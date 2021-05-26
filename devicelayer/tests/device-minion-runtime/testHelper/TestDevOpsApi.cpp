//******************************************************************************
// Copyright (C) 2020 Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "TestDevOpsApi.h"
#include "Autogen.h"
#include <cmath>
#include <experimental/filesystem>

using namespace ELFIO;
namespace fs = std::experimental::filesystem;

DEFINE_string(kernels_dir, "", "Directory where different kernel ELF files are located");
DEFINE_uint32(exec_timeout, 100, "Internal execution timeout");
DEFINE_bool(loopback_driver, false, "Run on loopback driver");

void TestDevOpsApi::fExecutor(uint8_t queueId) {
  auto start = Clock::now();
  auto end = start + execTimeout_;

  size_t cmdIdx = 0;
  while (cmdIdx < streams_[queueId].size()) {
    if (Clock::now() > end) {
      EXPECT_TRUE(false) << "Error: fExecutor timed out!" << std::endl;
      return;
    }
    try {
      if (!pushCmd(queueId, streams_[queueId][cmdIdx])) {
        {
          std::unique_lock<std::mutex> lk(sqBitmapMtx_);
          sqBitmapCondVar_.wait(lk, [this, queueId]() -> bool { return sqBitmap_ & (0x1U << queueId); });
          sqBitmap_ &= ~(0x1U << queueId);
        }
      } else {
        cmdIdx++;
      }
    } catch (const std::exception& e) {
      TEST_VLOG(0) << "Exception: " << e.what() << std::endl;
      assert(false);
    }
  }
}

void TestDevOpsApi::fListener() {
  auto start = Clock::now();
  auto end = start + execTimeout_;

  auto queueCount = devLayer_->getSubmissionQueuesCount(kIDevice);
  uint64_t sq_bitmap = (0x1U << queueCount) - 1;
  bool cq_available = true;

  ssize_t rspsToReceive = 0;
  for (auto& it : streams_) {
    rspsToReceive += it.second.size();
  }

  bool isPendingEvent = true;
  while (rspsToReceive > 0) {
    if (Clock::now() > end) {
      EXPECT_TRUE(false) << "Error: fListener timed out! responses left: " << rspsToReceive << std::endl;
      // wake all executor threads to exit after timeout
      sqBitmap_ = (0x1U << queueCount) - 1;
      sqBitmapCondVar_.notify_all();
      return;
    }
    try {
      if (isPendingEvent) {
        isPendingEvent = false;
      } else {
        devLayer_->waitForEpollEventsMasterMinion(kIDevice, sq_bitmap, cq_available);
      }
      if (cq_available) {
        while (popRsp()) {
          rspsToReceive--;
        }
      }
      if (sq_bitmap > 0) {
        {
          std::lock_guard<std::mutex> lk(sqBitmapMtx_);
          sqBitmap_ |= sq_bitmap;
        }
        sqBitmapCondVar_.notify_all();
      }
    } catch (const std::exception& e) {
      TEST_VLOG(0) << "Exception: " << e.what() << std::endl;
      assert(false);
    }
  }
}

void TestDevOpsApi::executeAsync() {
  std::vector<std::thread> threadVector;
  auto queueCount = devLayer_->getSubmissionQueuesCount(kIDevice);

  TEST_VLOG(0) << "Test execution started..." << std::endl;

  // Create one listener thread
  threadVector.push_back(std::thread([this]() { this->fListener(); }));

  // Create submission threads
  for (int queueId = 0; queueId < queueCount; queueId++) {
    if (streams_.find(queueId) == streams_.end()) {
      continue;
    }
    threadVector.push_back(std::thread([this, queueId]() { this->fExecutor(queueId); }));
  }

  for (auto& t : threadVector) {
    if (t.joinable()) {
      t.join();
    }
  }

  // Print and validate command results
  printCmdExecutionSummary();

  cleanUpExecution();
}

void TestDevOpsApi::executeSync() {
  auto start = Clock::now();
  auto end = start + execTimeout_;
  auto queueCount = devLayer_->getSubmissionQueuesCount(kIDevice);
  uint64_t sqBitmap = 0;
  bool cqAvailable = false;

  TEST_VLOG(0) << "Test execution started..." << std::endl;
  for (int queueId = 0; queueId < queueCount; queueId++) {
    if (streams_.find(queueId) == streams_.end()) {
      continue;
    }
    for (size_t cmdIdx = 0; cmdIdx < streams_[queueId].size(); cmdIdx++) {
      auto execStart = Clock::now();
      try {
        while (!pushCmd(queueId, streams_[queueId][cmdIdx])) {
          do {
            ASSERT_TRUE(end > Clock::now()) << "\nexecuteSync timed out!\n" << std::endl;
            devLayer_->waitForEpollEventsMasterMinion(kIDevice, sqBitmap, cqAvailable);
          } while (!(sqBitmap & (1ULL << queueId)));
        }
        while (!popRsp()) {
          do {
            ASSERT_TRUE(end > Clock::now()) << "\nexecuteSync timed out!\n" << std::endl;
            devLayer_->waitForEpollEventsMasterMinion(kIDevice, sqBitmap, cqAvailable);
          } while (!cqAvailable);
        }
      } catch (const std::exception& e) {
        TEST_VLOG(0) << "Exception: " << e.what() << std::endl;
        assert(false);
      }
      std::chrono::duration<double> execTime = Clock::now() - execStart;
      TEST_VLOG(1) << " - Command sent to response receive time: " << (execTime.count() * 1000) << "ms" << std::endl
                   << std::endl;
    }
  }

  // Print and validate command results
  printCmdExecutionSummary();

  cleanUpExecution();
}

void TestDevOpsApi::cleanUpExecution() {
  std::vector<device_ops_api::tag_id_t> kernelTagIdsToAbort;
  auto queueCount = devLayer_->getSubmissionQueuesCount(kIDevice);

  for (int queueId = 0; queueId < queueCount; queueId++) {
    if (streams_.find(queueId) == streams_.end()) {
      continue;
    }

    for (auto& it : streams_[queueId]) {
      if (it->whoAmI() == IDevOpsApiCmd::CmdType::KERNEL_LAUNCH_CMD &&
          getCmdResult(it->getCmdTagId()) == CmdStatus::CMD_RSP_NOT_RECEIVED) {
        kernelTagIdsToAbort.push_back(it->getCmdTagId());
      }
    }
  }

  streams_.clear();

  if (kernelTagIdsToAbort.size() > 0) {
    TEST_VLOG(0) << "Sending abort command(s) for incomplete kernel(s)" << std::endl;

    // Cleaning up command results except for the ones whose responses are not received
    for (auto it = cmdResults_.begin(); it != cmdResults_.end();) {
      if (it->second != CmdStatus::CMD_RSP_NOT_RECEIVED) {
        it = cmdResults_.erase(it);
      } else {
        ++it;
      }
    }

    // Make a stream of abort kernel commands
    std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;
    for (auto it : kernelTagIdsToAbort) {
      stream.push_back(IDevOpsApiCmd::createKernelAbortCmd(getNextTagId(), false, it,
                                                           device_ops_api::DEV_OPS_API_KERNEL_ABORT_RESPONSE_SUCCESS));
    }
    // using SQ_0 to abort kernels
    streams_.emplace(0, std::move(stream));

    // Execute the abort commands
    std::vector<std::thread> threadVector;
    execTimeout_ = std::chrono::seconds(20);
    threadVector.push_back(std::thread([this]() { this->fListener(); }));
    threadVector.push_back(std::thread([this]() { this->fExecutor(0); }));

    for (auto& t : threadVector) {
      if (t.joinable()) {
        t.join();
      }
    }

    auto missingRspCmdsCount = std::count_if(cmdResults_.begin(), cmdResults_.end(),
                                             [](auto& e) { return std::get<1>(e) == CmdStatus::CMD_RSP_NOT_RECEIVED; });
    EXPECT_EQ(missingRspCmdsCount, 0) << "Failed to abort kernel launches!" << std::endl;

    streams_.clear();
  }

  cmdResults_.clear();
}

void TestDevOpsApi::resetMemPooltoDefault() {
  dmaWriteAddr_ = dmaReadAddr_ = devLayer_->getDramBaseAddress();
}

uint64_t TestDevOpsApi::getDmaWriteAddr(size_t bufSize) {
  std::lock_guard<std::mutex> lock(dmaWriteAddrMtx_);
  auto dramEnd = devLayer_->getDramBaseAddress() + devLayer_->getDramSize();
  if (dmaWriteAddr_ + bufSize < dramEnd) {
    auto currentDmaPtr = dmaWriteAddr_;
    dmaWriteAddr_ += bufSize;
    return currentDmaPtr;
  }
  TEST_VLOG(0) << "DMA write ptr reached DRAM end" << std::endl;
  assert(false);
  return 0;
}

uint64_t TestDevOpsApi::getDmaReadAddr(size_t bufSize) {
  std::lock_guard<std::mutex> lock(dmaReadAddrMtx_);
  auto dramEnd = devLayer_->getDramBaseAddress() + devLayer_->getDramSize();
  if (dmaReadAddr_ + bufSize < dramEnd) {
    auto currentDmaPtr = dmaReadAddr_;
    dmaReadAddr_ += bufSize;
    return currentDmaPtr;
  }
  TEST_VLOG(0) << "DMA read ptr reached DRAM end" << std::endl;
  assert(false);
  return 0;
}

device_ops_api::tag_id_t TestDevOpsApi::getNextTagId() {
  return tagId_++;
}

void TestDevOpsApi::initTagId(device_ops_api::tag_id_t value) {
  tagId_ = value;
}

bool TestDevOpsApi::pushCmd(uint16_t queueId, std::unique_ptr<IDevOpsApiCmd>& devOpsApiCmd) {
  addCmdResultEntry(devOpsApiCmd->getCmdTagId(), CmdStatus::CMD_RSP_NOT_RECEIVED);
  auto res = devLayer_->sendCommandMasterMinion(kIDevice, queueId, devOpsApiCmd->getCmdPtr(),
                                                devOpsApiCmd->getCmdSize(), devOpsApiCmd->isDma());
  if (res) {
    TEST_VLOG(1) << "=====> Command Sent (tag_id: " << std::hex << devOpsApiCmd->getCmdTagId()
                 << ") <====" << std::endl;
  } else {
    deleteCmdResultEntry(devOpsApiCmd->getCmdTagId());
  }

  return res;
}

bool TestDevOpsApi::popRsp(void) {
  std::vector<std::byte> message;
  auto res = devLayer_->receiveResponseMasterMinion(kIDevice, message);
  if (!res) {
    return res;
  }

  auto response_header = reinterpret_cast<device_ops_api::rsp_header_t*>(message.data());
  auto rsp_msg_id = response_header->rsp_hdr.msg_id;
  auto rsp_tag_id = response_header->rsp_hdr.tag_id;
  CmdStatus status;

  if (rsp_msg_id == device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_ECHO_RSP) {
    auto response = reinterpret_cast<device_ops_api::device_ops_echo_rsp_t*>(message.data());
    TEST_VLOG(1) << "=====> Echo response received (tag_id: " << std::hex << rsp_tag_id << ") <====" << std::endl;
    if (response->echo_payload == kEchoPayload) {
      status = CmdStatus::CMD_SUCCESSFUL;
    } else {
      status = CmdStatus::CMD_FAILED;
    }

  } else if (rsp_msg_id == device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_API_COMPATIBILITY_RSP) {
    auto response = reinterpret_cast<device_ops_api::device_ops_api_compatibility_rsp_t*>(message.data());
    TEST_VLOG(1) << "=====> API_Compatibilty response received (tag_id: " << std::hex << rsp_tag_id
                 << ") <====" << std::endl;
    if (response->major == kDevFWMajor && response->minor == kDevFWMinor && response->patch == kDevFWPatch) {
      status = CmdStatus::CMD_SUCCESSFUL;
    } else {
      status = CmdStatus::CMD_FAILED;
    }

  } else if (rsp_msg_id == device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_FW_VERSION_RSP) {
    auto response = reinterpret_cast<device_ops_api::device_ops_fw_version_rsp_t*>(message.data());
    TEST_VLOG(1) << "=====> FW_Version response received (tag_id: " << std::hex << rsp_tag_id << ") <====" << std::endl;
    if (response->major == 1 && response->minor == 0 && response->patch == 0) {
      status = CmdStatus::CMD_SUCCESSFUL;
    } else {
      status = CmdStatus::CMD_FAILED;
    }

  } else if (rsp_msg_id == device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_DATA_READ_RSP) {
    auto response = reinterpret_cast<device_ops_api::device_ops_data_read_rsp_t*>(message.data());
    TEST_VLOG(1) << "=====> DMA data read command response received (tag_id: " << std::hex << rsp_tag_id
                 << ") <====" << std::endl;
    TEST_VLOG(1) << "     => Total measured latencies (in cycles) " << std::endl;
    TEST_VLOG(1) << "      - Command Wait time: " << response->cmd_wait_time << std::endl;
    TEST_VLOG(1) << "      - Command Execution time: " << response->cmd_execution_time << std::endl << std::endl;
    if (response->status == IDevOpsApiCmd::getExpectedRsp(rsp_tag_id)) {
      status = CmdStatus::CMD_SUCCESSFUL;
    } else if (response->status == (device_ops_api::DEV_OPS_API_DMA_RESPONSE_TIMEOUT_IDLE_CHANNEL_UNAVAILABLE ||
                                    device_ops_api::DEV_OPS_API_DMA_RESPONSE_TIMEOUT_HANG)) {
      status = CmdStatus::CMD_TIMED_OUT;
    } else {
      status = CmdStatus::CMD_FAILED;
    }

  } else if (rsp_msg_id == device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_DATA_WRITE_RSP) {
    auto response = reinterpret_cast<device_ops_api::device_ops_data_write_rsp_t*>(message.data());
    TEST_VLOG(1) << "=====> DMA data write command response received (tag_id: " << std::hex << rsp_tag_id
                 << ") <====" << std::endl;
    TEST_VLOG(1) << "     => Total measured latencies (in cycles)" << std::endl;
    TEST_VLOG(1) << "      - Command Wait time: " << response->cmd_wait_time << std::endl;
    TEST_VLOG(1) << "      - Command Execution time: " << response->cmd_execution_time << std::endl << std::endl;
    if (response->status == IDevOpsApiCmd::getExpectedRsp(rsp_tag_id)) {
      status = CmdStatus::CMD_SUCCESSFUL;
    } else if (response->status == (device_ops_api::DEV_OPS_API_DMA_RESPONSE_TIMEOUT_IDLE_CHANNEL_UNAVAILABLE ||
                                    device_ops_api::DEV_OPS_API_DMA_RESPONSE_TIMEOUT_HANG)) {
      status = CmdStatus::CMD_TIMED_OUT;
    } else {
      status = CmdStatus::CMD_FAILED;
    }

  } else if (rsp_msg_id == device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_KERNEL_LAUNCH_RSP) {
    auto response = reinterpret_cast<device_ops_api::device_ops_kernel_launch_rsp_t*>(message.data());
    TEST_VLOG(1) << "=====> Kernel Launch command response received (tag_id: " << std::hex << rsp_tag_id
                 << ") <====" << std::endl;
    TEST_VLOG(1) << "     => Total measured latencies (in cycles)" << std::endl;
    TEST_VLOG(1) << "      - Command Wait time: " << response->cmd_wait_time << std::endl;
    TEST_VLOG(1) << "      - Command Execution time: " << response->cmd_execution_time << std::endl << std::endl;
    if (response->status == IDevOpsApiCmd::getExpectedRsp(rsp_tag_id)) {
      status = CmdStatus::CMD_SUCCESSFUL;
    } else if (response->status == device_ops_api::DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_TIMEOUT_HANG) {
      status = CmdStatus::CMD_TIMED_OUT;
    } else {
      status = CmdStatus::CMD_FAILED;
    }

  } else if (rsp_msg_id == device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_KERNEL_ABORT_RSP) {
    auto response = reinterpret_cast<device_ops_api::device_ops_kernel_abort_rsp_t*>(message.data());
    TEST_VLOG(1) << "=====> Kernel Abort command response received (tag_id: " << std::hex << rsp_tag_id
                 << ") <====" << std::endl;
    if (response->status == IDevOpsApiCmd::getExpectedRsp(rsp_tag_id)) {
      status = CmdStatus::CMD_SUCCESSFUL;
    } else {
      status = CmdStatus::CMD_FAILED;
    }

  } else if (rsp_msg_id == device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_TRACE_RT_CONTROL_RSP) {
    auto response = reinterpret_cast<device_ops_api::device_ops_trace_rt_control_rsp_t*>(message.data());
    TEST_VLOG(1) << "=====> Trace RT config command response received (tag_id: " << std::hex << rsp_tag_id
                 << ") <====" << std::endl;
    if (response->status == IDevOpsApiCmd::getExpectedRsp(rsp_tag_id)) {
      status = CmdStatus::CMD_SUCCESSFUL;
    } else {
      status = CmdStatus::CMD_FAILED;
    }

  } else {
    EXPECT_TRUE(false) << "ERROR: Unknown response!" << std::endl;
    return popRsp();
  }

  // verify the response status for duplication
  if (getCmdResult(rsp_tag_id) != CmdStatus::CMD_RSP_NOT_RECEIVED) {
    updateCmdResult(rsp_tag_id, CmdStatus::CMD_RSP_DUPLICATE);
    return popRsp();
  }

  if (!updateCmdResult(rsp_tag_id, status)) {
    TEST_VLOG(0) << "Command (tagId: " << std::hex << rsp_tag_id
                 << ") wasn't sent under this scope, discarding this response!" << std::endl;
    return popRsp();
  }
  return res;
}

bool TestDevOpsApi::addCmdResultEntry(device_ops_api::tag_id_t tagId, CmdStatus status) {
  std::unique_lock<std::recursive_mutex> lock(cmdResultsMtx_);
  auto it = cmdResults_.find(tagId);
  if (it != cmdResults_.end()) {
    return false;
  }

  if (cmdResults_.size() == 0) {
    firstCmdTimepoint_ = Clock::now();
    firstRspTimepoint_ = firstCmdTimepoint_;
  } else {
    lastCmdTimepoint_ = Clock::now();
  }

  cmdResults_.emplace(tagId, status);
  return true;
}

bool TestDevOpsApi::updateCmdResult(device_ops_api::tag_id_t tagId, CmdStatus status) {
  std::unique_lock<std::recursive_mutex> lock(cmdResultsMtx_);
  auto it = cmdResults_.find(tagId);
  if (it == cmdResults_.end()) {
    return false;
  }

  lastRspTimepoint_ = Clock::now();

  it->second = status;
  return true;
}

CmdStatus TestDevOpsApi::getCmdResult(device_ops_api::tag_id_t tagId) {
  std::unique_lock<std::recursive_mutex> lock(cmdResultsMtx_);
  auto it = cmdResults_.find(tagId);
  if (it == cmdResults_.end()) {
    return CmdStatus::CMD_RSP_NOT_RECEIVED; // Default response
  }

  return it->second;
}

void TestDevOpsApi::deleteCmdResultEntry(device_ops_api::tag_id_t tagId) {
  std::unique_lock<std::recursive_mutex> lock(cmdResultsMtx_);
  auto it = cmdResults_.find(tagId);
  if (it == cmdResults_.end()) {
    return;
  }
  cmdResults_.erase(it);
}

void TestDevOpsApi::printCmdExecutionSummary() {
  std::vector<device_ops_api::tag_id_t> successfulCmdTags, failedCmdTags, timeoutCmdTags, missingRspCmdTags,
    duplicateRspCmdTags;

  for (auto it : cmdResults_) {
    switch (it.second) {
    case CmdStatus::CMD_SUCCESSFUL:
      successfulCmdTags.push_back(it.first);
      break;
    case CmdStatus::CMD_FAILED:
      failedCmdTags.push_back(it.first);
      break;
    case CmdStatus::CMD_TIMED_OUT:
      timeoutCmdTags.push_back(it.first);
      break;
    case CmdStatus::CMD_RSP_NOT_RECEIVED:
      missingRspCmdTags.push_back(it.first);
      break;
    case CmdStatus::CMD_RSP_DUPLICATE:
      duplicateRspCmdTags.push_back(it.first);
    }
  }

  TEST_VLOG(0) << "=============== TEST SUMMARY ===================" << std::endl;
  TEST_VLOG(0) << "====> Total Commands: " << cmdResults_.size() << std::endl;
  TEST_VLOG(0) << "  ====> Commands successful: " << successfulCmdTags.size() << std::endl;
  TEST_VLOG(0) << "  ====> Commands failed: " << failedCmdTags.size() << std::endl;
  if (failedCmdTags.size() > 0) {
    std::stringstream tagIdsStream;
    for (auto it : failedCmdTags) {
      tagIdsStream << "0x" << std::hex << it << " ";
    }
    TEST_VLOG(0) << "    ----> Tag IDs: " << tagIdsStream.str() << std::endl;
  }
  TEST_VLOG(0) << "  ====> Commands timed out: " << timeoutCmdTags.size() << std::endl;
  if (timeoutCmdTags.size() > 0) {
    std::stringstream tagIdsStream;
    for (auto it : timeoutCmdTags) {
      tagIdsStream << "0x" << std::hex << it << " ";
    }
    TEST_VLOG(0) << "    ----> Tag IDs: " << tagIdsStream.str() << std::endl;
  }
  TEST_VLOG(0) << "  ====> Commands missing responses: " << missingRspCmdTags.size() << std::endl;
  if (missingRspCmdTags.size() > 0) {
    std::stringstream tagIdsStream;
    for (auto it : missingRspCmdTags) {
      tagIdsStream << "0x" << std::hex << it << " ";
    }
    TEST_VLOG(0) << "    ----> Tag IDs: " << tagIdsStream.str() << std::endl;
  }
  TEST_VLOG(0) << "  ====> Commands duplicate responses: " << duplicateRspCmdTags.size() << std::endl;
  if (duplicateRspCmdTags.size() > 0) {
    std::stringstream tagIdsStream;
    for (auto it : duplicateRspCmdTags) {
      tagIdsStream << "0x" << std::hex << it << " ";
    }
    TEST_VLOG(0) << "    ----> Tag IDs: " << tagIdsStream.str() << std::endl;
  }
  std::chrono::duration<double> cmdsTotalDuration = lastCmdTimepoint_ - firstCmdTimepoint_;
  std::chrono::duration<double> rspsTotalDuration = lastRspTimepoint_ - firstRspTimepoint_;
  TEST_VLOG(0) << "  ====> Commands sent per second: " << std::ceil(cmdResults_.size() / cmdsTotalDuration.count())
               << std::endl;
  TEST_VLOG(0) << "  ====> Responses received per second: " << std::ceil(cmdResults_.size() / rspsTotalDuration.count())
               << std::endl;
  TEST_VLOG(0) << "================================================" << std::endl;

  EXPECT_EQ(failedCmdTags.size(), 0);
  EXPECT_EQ(missingRspCmdTags.size(), 0);
  EXPECT_EQ(duplicateRspCmdTags.size(), 0);
  EXPECT_EQ(timeoutCmdTags.size(), 0);
}

void TestDevOpsApi::printErrorContext(void* buffer, uint64_t shireMask) {
  hart_execution_context* context = reinterpret_cast<hart_execution_context*>(buffer);

  TEST_VLOG(0) << "*** Error Context Start ***";
  for (int shireID = 0; shireID < 32; shireID++) {
    if (shireMask & (1 << shireID)) {
      // print the context of the first hart in a shire only
      auto minionHartID = shireID * 64;
      TEST_VLOG(0) << "* HartID: " << context[minionHartID].hart_id;
      TEST_VLOG(0) << "* Hart cycles: " << context[minionHartID].cycles;
      TEST_VLOG(0) << "* epc: 0x" << std::hex << context[minionHartID].sepc;
      TEST_VLOG(0) << "* status: 0x" << std::hex << context[minionHartID].sstatus;
      TEST_VLOG(0) << "* tval: 0x" << std::hex << context[minionHartID].stval;
      TEST_VLOG(0) << "* cause: 0x" << std::hex << context[minionHartID].scause;
      TEST_VLOG(0) << "* gpr[x1]  : ra    : 0x" << std::hex << context[minionHartID].gpr[0];
      TEST_VLOG(0) << "* gpr[x3]  : gp    : 0x" << std::hex << context[minionHartID].gpr[1];
      TEST_VLOG(0) << "* gpr[x5]  : t0    : 0x" << std::hex << context[minionHartID].gpr[2];
      TEST_VLOG(0) << "* gpr[x6]  : t1    : 0x" << std::hex << context[minionHartID].gpr[3];
      TEST_VLOG(0) << "* gpr[x7]  : t2    : 0x" << std::hex << context[minionHartID].gpr[4];
      TEST_VLOG(0) << "* gpr[x8]  : s0/fp : 0x" << std::hex << context[minionHartID].gpr[5];
      TEST_VLOG(0) << "* gpr[x9]  : s1    : 0x" << std::hex << context[minionHartID].gpr[6];
      for (auto i = 0; i < 8; i++) {
        auto reg = i + 7;
        TEST_VLOG(0) << "* gpr[x" << (reg + 3) << "] : a" << i << "    : 0x" << std::hex << context[minionHartID].gpr[reg];
      }
      for (auto i = 0; i < 10; i++) {
        auto reg = i + 15;
        if (i < 8) {
          TEST_VLOG(0) << "* gpr[x" << (reg + 3) << "] : s" << (i + 2) << "    : 0x" << std::hex
                       << context[minionHartID].gpr[reg];
        } else {
          TEST_VLOG(0) << "* gpr[x" << (reg + 3) << "] : s" << (i + 2) << "   : 0x" << std::hex
                       << context[minionHartID].gpr[reg];
        }
      }
      TEST_VLOG(0) << "* gpr[x28] : t3    : 0x" << std::hex << context[minionHartID].gpr[25];
      TEST_VLOG(0) << "* gpr[x29] : t4    : 0x" << std::hex << context[minionHartID].gpr[26];
      TEST_VLOG(0) << "* gpr[x30] : t5    : 0x" << std::hex << context[minionHartID].gpr[27];
      TEST_VLOG(0) << "* gpr[x31] : t6    : 0x" << std::hex << context[minionHartID].gpr[28];
      TEST_VLOG(0) << "--------------------------";
    }
  }
  TEST_VLOG(0) << "*** Error Context End ***" << std::endl;
}

void TestDevOpsApi::deleteCmdResults() {
  std::unique_lock<std::recursive_mutex> lock(cmdResultsMtx_);
  cmdResults_.clear();
}

void TestDevOpsApi::loadElfToDevice(ELFIO::elfio& reader, const std::string& path,
                                    std::vector<std::unique_ptr<IDevOpsApiCmd>>& stream, uint64_t& kernelEntryAddr) {
  bool elfLoadSuccess = reader.load(path);
  ASSERT_TRUE(elfLoadSuccess);

  // We only allow ELFs with 1 segment, and the code should be relocatable.
  // TODO: Properly generate static PIE ELFs
  ASSERT_EQ(reader.segments.size(), 1);

  const segment* segment0 = reader.segments[0];
  ASSERT_TRUE(segment0->get_type() & PT_LOAD);

  // Copy segment to device memory
  auto entry = reader.get_entry();
  auto vAddr = segment0->get_virtual_address();
  auto pAddr = segment0->get_physical_address();
  auto fileSize = segment0->get_file_size();
  auto memSize = segment0->get_memory_size();

  TEST_VLOG(1) << std::endl << "Loading ELF: " << path << std::endl;
  TEST_VLOG(1) << "  ELF Entry: 0x" << std::hex << entry << std::endl;
  TEST_VLOG(1) << "  Segment [0]: "
               << "vAddr: 0x" << std::hex << vAddr << ", pAddr: 0x" << std::hex << pAddr << ", Mem Size: 0x" << memSize
               << ", File Size: 0x" << fileSize << std::endl;

  // Allocate device buffer for the ELF segment
  uint64_t deviceElfSegment0Buffer = getDmaWriteAddr(ALIGN(memSize, 0x1000));
  kernelEntryAddr = deviceElfSegment0Buffer + (entry - vAddr);

  TEST_VLOG(1) << " Allocated buffer at device address: 0x" << deviceElfSegment0Buffer << " for segment 0" << std::endl;
  TEST_VLOG(1) << " Kernel entry at device address: 0x" << kernelEntryAddr << std::endl;

  // Create DMA write command
  auto hostVirtAddr = reinterpret_cast<uint64_t>(segment0->get_data());
  auto hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
  stream.push_back(IDevOpsApiCmd::createDataWriteCmd(getNextTagId(), false, deviceElfSegment0Buffer, hostVirtAddr,
                                                     hostPhysAddr, fileSize,
                                                     device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
}
