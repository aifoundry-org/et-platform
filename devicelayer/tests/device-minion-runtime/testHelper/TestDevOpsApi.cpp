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

void TestDevOpsApi::initTestHelperSysEmu(const emu::SysEmuOptions& options) {
  devLayer_ = dev::IDeviceLayer::createSysEmuDeviceLayer(options);
  EXPECT_NE(devLayer_, nullptr) << "Unable to instantiate devLayer!" << std::endl;

  devices_.clear();
  auto deviceCount = getDevicesCount();
  for (unsigned long i = 0; i < deviceCount; i++) {
    devices_.emplace_back(new DeviceInfo);
    devices_[i]->dmaWriteAddr_ = devices_[i]->dmaReadAddr_ = devLayer_->getDramBaseAddress();
    devices_[i]->sqBitmap_ = ~0ULL;
  }
}

void TestDevOpsApi::initTestHelperPcie() {
  devLayer_ = dev::IDeviceLayer::createPcieDeviceLayer(true, false);
  EXPECT_NE(devLayer_, nullptr) << "Unable to instantiate devLayer!" << std::endl;

  devices_.clear();
  auto deviceCount = getDevicesCount();
  for (unsigned long i = 0; i < deviceCount; i++) {
    devices_.emplace_back(new DeviceInfo);
    devices_[i]->dmaWriteAddr_ = devices_[i]->dmaReadAddr_ = devLayer_->getDramBaseAddress();
    devices_[i]->sqBitmap_ = ~0ULL;
  }
}

void TestDevOpsApi::fExecutor(int deviceIdx, int queueIdx) {
  TEST_VLOG(1) << "Device [" << deviceIdx << "], Queue [" << queueIdx << "] started.";
  auto start = Clock::now();
  auto end = start + execTimeout_;

  size_t cmdIdx = 0;
  if (deviceIdx >= static_cast<int>(devices_.size())) {
    EXPECT_TRUE(false) << "Error: No device[" << deviceIdx << "] found, fExecutor can't be run!" << std::endl;
    return;
  }
  auto& deviceInfo = devices_[static_cast<unsigned long>(deviceIdx)];

  while (cmdIdx < streams_[key(deviceIdx, queueIdx)].size()) {
    if (Clock::now() > end) {
      EXPECT_TRUE(false) << "Error: fExecutor timed out!" << std::endl;
      return;
    }
    try {
      if (!pushCmd(deviceIdx, queueIdx, streams_[key(deviceIdx, queueIdx)][cmdIdx])) {
        {
          std::unique_lock<std::mutex> lk(deviceInfo->sqBitmapMtx_);
          deviceInfo->sqBitmapCondVar_.wait(
            lk, [this, &deviceInfo, queueIdx]() -> bool { return deviceInfo->sqBitmap_ & (0x1U << queueIdx); });
          deviceInfo->sqBitmap_ &= ~(0x1U << queueIdx);
        }
      } else {
        cmdIdx++;
      }
    } catch (const std::exception& e) {
      TEST_VLOG(0) << "Exception: " << e.what();
      assert(false);
    }
  }
}

void TestDevOpsApi::fListener(int deviceIdx) {
  TEST_VLOG(1) << "Device [" << deviceIdx << "] started.";
  auto start = Clock::now();
  auto end = start + execTimeout_;

  if (deviceIdx >= static_cast<int>(devices_.size())) {
    EXPECT_TRUE(false) << "Error: No device[" << deviceIdx << "] found, fListener can't be run!" << std::endl;
    return;
  }
  auto& deviceInfo = devices_[static_cast<unsigned long>(deviceIdx)];

  auto queueCount = getSqCount(deviceIdx);
  uint64_t sq_bitmap = (0x1U << queueCount) - 1;
  bool cq_available = true;

  ssize_t rspsToReceive = 0;
  for (int queueIdx = 0; queueIdx < queueCount; queueIdx++) {
    auto streamIt = streams_.find(key(deviceIdx, queueIdx));
    if (streamIt != streams_.end()) {
      rspsToReceive += streamIt->second.size();
    }
  }

  bool isPendingEvent = true;
  while (rspsToReceive > 0) {
    if (Clock::now() > end) {
      EXPECT_TRUE(false) << "Error: fListener timed out! responses left: " << rspsToReceive << std::endl;
      // wake all executor threads to exit after timeout
      deviceInfo->sqBitmap_ = (0x1U << queueCount) - 1;
      deviceInfo->sqBitmapCondVar_.notify_all();
      return;
    }
    try {
      if (isPendingEvent) {
        isPendingEvent = false;
      } else {
        devLayer_->waitForEpollEventsMasterMinion(deviceIdx, sq_bitmap, cq_available);
      }
      if (cq_available) {
        while (popRsp(deviceIdx)) {
          rspsToReceive--;
        }
      }
      if (sq_bitmap > 0) {
        {
          std::lock_guard<std::mutex> lk(deviceInfo->sqBitmapMtx_);
          deviceInfo->sqBitmap_ |= sq_bitmap;
        }
        deviceInfo->sqBitmapCondVar_.notify_all();
      }
    } catch (const std::exception& e) {
      TEST_VLOG(0) << "Exception: " << e.what();
      assert(false);
    }
  }
}

void TestDevOpsApi::executeAsync() {
  std::vector<std::thread> threadVector;
  int deviceCount = getDevicesCount();
  TEST_VLOG(0) << "Test execution started...";

  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    int executorThreadCount = 0;
    auto queueCount = getSqCount(deviceIdx);

    // Create submission threads
    for (int queueIdx = 0; queueIdx < queueCount; queueIdx++) {
      if (streams_.find(key(deviceIdx, queueIdx)) == streams_.end()) {
        continue;
      }
      executorThreadCount++;
      threadVector.push_back(std::thread([this, deviceIdx, queueIdx]() { this->fExecutor(deviceIdx, queueIdx); }));
    }

    if (executorThreadCount == 0) {
      continue;
    }

    // Create one listener thread
    threadVector.push_back(std::thread([this, deviceIdx]() { this->fListener(deviceIdx); }));
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

void TestDevOpsApi::executeSyncPerDevice(int deviceIdx) {
  auto start = Clock::now();
  auto end = start + execTimeout_;
  auto queueCount = getSqCount(deviceIdx);
  uint64_t sqBitmap = 0;
  bool cqAvailable = false;
  for (int queueIdx = 0; queueIdx < queueCount; queueIdx++) {
    if (streams_.find(key(deviceIdx, queueIdx)) == streams_.end()) {
      continue;
    }
    for (size_t cmdIdx = 0; cmdIdx < streams_[key(deviceIdx, queueIdx)].size(); cmdIdx++) {
      auto execStart = Clock::now();
      try {
        while (!pushCmd(deviceIdx, queueIdx, streams_[key(deviceIdx, queueIdx)][cmdIdx])) {
          do {
            ASSERT_TRUE(end > Clock::now()) << "\nexecuteSync timed out!\n" << std::endl;
            devLayer_->waitForEpollEventsMasterMinion(deviceIdx, sqBitmap, cqAvailable);
          } while (!(sqBitmap & (1ULL << queueIdx)));
        }
        while (!popRsp(deviceIdx)) {
          do {
            ASSERT_TRUE(end > Clock::now()) << "\nexecuteSync timed out!\n" << std::endl;
            devLayer_->waitForEpollEventsMasterMinion(deviceIdx, sqBitmap, cqAvailable);
          } while (!cqAvailable);
        }
      } catch (const std::exception& e) {
        TEST_VLOG(0) << "Exception: " << e.what();
        assert(false);
      }
      std::chrono::duration<double> execTime = Clock::now() - execStart;
      TEST_VLOG(1) << " - Command sent to response receive time: " << (execTime.count() * 1000) << "ms";
    }
  }
}

void TestDevOpsApi::executeSync() {
  std::vector<std::thread> threadVector;
  int deviceCount = getDevicesCount();

  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    TEST_VLOG(0) << "Device[" << deviceIdx << "]: test execution started...";
    threadVector.push_back(std::thread([this, deviceIdx]() { this->executeSyncPerDevice(deviceIdx); }));
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

void TestDevOpsApi::cleanUpExecution() {
  int deviceCount = getDevicesCount();
  std::unordered_map<int, std::vector<device_ops_api::tag_id_t>> kernelTagIdsToAbort;
  int abortCmdCount = 0;
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    auto queueCount = getSqCount(deviceIdx);
    std::vector<device_ops_api::tag_id_t> kernelTagIdsToAbortPerDev;

    for (int queueIdx = 0; queueIdx < queueCount; queueIdx++) {
      if (streams_.find(key(deviceIdx, queueIdx)) == streams_.end()) {
        continue;
      }

      for (auto& it : streams_[key(deviceIdx, queueIdx)]) {
        if (it->whoAmI() == IDevOpsApiCmd::CmdType::KERNEL_LAUNCH_CMD &&
            getCmdResult(it->getCmdTagId()) == CmdStatus::CMD_RSP_NOT_RECEIVED) {
          kernelTagIdsToAbortPerDev.push_back(it->getCmdTagId());
          ++abortCmdCount;
        }
      }
      kernelTagIdsToAbort.emplace(deviceIdx, std::move(kernelTagIdsToAbortPerDev));
    }
  }

  streams_.clear();

  if (abortCmdCount > 0) {
    // Cleaning up command results except for the ones whose responses are not received
    for (auto it = cmdResults_.begin(); it != cmdResults_.end();) {
      if (it->second != CmdStatus::CMD_RSP_NOT_RECEIVED) {
        it = cmdResults_.erase(it);
      } else {
        ++it;
      }
    }

    std::vector<std::thread> threadVector;
    for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
      if (kernelTagIdsToAbort.find(deviceIdx) == kernelTagIdsToAbort.end()) {
        continue;
      }
      TEST_VLOG(0) << "Sending abort command(s) for incomplete kernel(s)";

      // Make a stream of abort kernel commands
      std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;
      for (auto it : kernelTagIdsToAbort[deviceIdx]) {
        stream.push_back(
          IDevOpsApiCmd::createKernelAbortCmd(false, it, device_ops_api::DEV_OPS_API_KERNEL_ABORT_RESPONSE_SUCCESS));
      }
      // using SQ_0 to abort kernels
      streams_.emplace(key(deviceIdx, 0), std::move(stream));

      // Execute the abort commands
      execTimeout_ = std::chrono::seconds(20);
      threadVector.push_back(std::thread([this, deviceIdx]() { this->fListener(deviceIdx); }));
      threadVector.push_back(std::thread([this, deviceIdx]() { this->fExecutor(deviceIdx, 0); }));
    }

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

void TestDevOpsApi::resetMemPooltoDefault(int deviceIdx) {
  if (deviceIdx >= static_cast<int>(devices_.size())) {
    TEST_VLOG(0) << "Device[" << deviceIdx << "] not found.";
    assert(false);
  }
  auto& deviceInfo = devices_[static_cast<unsigned long>(deviceIdx)];
  deviceInfo->dmaWriteAddr_ = deviceInfo->dmaReadAddr_ = devLayer_->getDramBaseAddress();
}

uint64_t TestDevOpsApi::getDmaWriteAddr(int deviceIdx, size_t bufSize) {
  if (deviceIdx >= static_cast<int>(devices_.size())) {
    TEST_VLOG(0) << "Device[" << deviceIdx << "] not found.";
    assert(false);
  }
  auto& deviceInfo = devices_[static_cast<unsigned long>(deviceIdx)];
  std::lock_guard<std::mutex> lock(deviceInfo->dmaWriteAddrMtx_);
  auto dramEnd = devLayer_->getDramBaseAddress() + devLayer_->getDramSize();
  if (deviceInfo->dmaWriteAddr_ + bufSize < dramEnd) {
    auto currentDmaPtr = deviceInfo->dmaWriteAddr_;
    deviceInfo->dmaWriteAddr_ += bufSize;
    return currentDmaPtr;
  }
  TEST_VLOG(0) << "DMA write ptr reached DRAM end";
  assert(false);
  return 0;
}

uint64_t TestDevOpsApi::getDmaReadAddr(int deviceIdx, size_t bufSize) {
  if (deviceIdx >= static_cast<int>(devices_.size())) {
    TEST_VLOG(0) << "Device[" << deviceIdx << "] not found.";
    assert(false);
  }
  auto& deviceInfo = devices_[static_cast<unsigned long>(deviceIdx)];
  std::lock_guard<std::mutex> lock(deviceInfo->dmaReadAddrMtx_);
  auto dramEnd = devLayer_->getDramBaseAddress() + devLayer_->getDramSize();
  if (deviceInfo->dmaReadAddr_ + bufSize < dramEnd) {
    auto currentDmaPtr = deviceInfo->dmaReadAddr_;
    deviceInfo->dmaReadAddr_ += bufSize;
    return currentDmaPtr;
  }
  TEST_VLOG(0) << "DMA read ptr reached DRAM end";
  assert(false);
  return 0;
}

bool TestDevOpsApi::pushCmd(int deviceIdx, int queueIdx, std::unique_ptr<IDevOpsApiCmd>& devOpsApiCmd) {
  addCmdResultEntry(devOpsApiCmd->getCmdTagId(), CmdStatus::CMD_RSP_NOT_RECEIVED);
  auto res = devLayer_->sendCommandMasterMinion(deviceIdx, queueIdx, devOpsApiCmd->getCmdPtr(),
                                                devOpsApiCmd->getCmdSize(), devOpsApiCmd->isDma());
  if (res) {
    TEST_VLOG(1) << "=====> Command Sent (tag_id: " << std::hex << devOpsApiCmd->getCmdTagId() << ") <====";
  } else {
    deleteCmdResultEntry(devOpsApiCmd->getCmdTagId());
  }

  return res;
}

bool TestDevOpsApi::popRsp(int deviceIdx) {
  std::vector<std::byte> message;
  auto res = devLayer_->receiveResponseMasterMinion(deviceIdx, message);
  if (!res) {
    return res;
  }

  auto response_header = reinterpret_cast<device_ops_api::rsp_header_t*>(message.data());
  auto rsp_msg_id = response_header->rsp_hdr.msg_id;
  auto rsp_tag_id = response_header->rsp_hdr.tag_id;
  CmdStatus status;

  if (rsp_msg_id == device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_ECHO_RSP) {
    auto response = reinterpret_cast<device_ops_api::device_ops_echo_rsp_t*>(message.data());
    TEST_VLOG(1) << "=====> Echo response received (tag_id: " << std::hex << rsp_tag_id << ") <====";
    if (response->echo_payload == kEchoPayload) {
      status = CmdStatus::CMD_SUCCESSFUL;
    } else {
      status = CmdStatus::CMD_FAILED;
    }

  } else if (rsp_msg_id == device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_API_COMPATIBILITY_RSP) {
    auto response = reinterpret_cast<device_ops_api::device_ops_api_compatibility_rsp_t*>(message.data());
    TEST_VLOG(1) << "=====> API_Compatibilty response received (tag_id: " << std::hex << rsp_tag_id << ") <====";
    if (response->major == kDevFWMajor && response->minor == kDevFWMinor && response->patch == kDevFWPatch) {
      status = CmdStatus::CMD_SUCCESSFUL;
    } else {
      status = CmdStatus::CMD_FAILED;
    }

  } else if (rsp_msg_id == device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_FW_VERSION_RSP) {
    auto response = reinterpret_cast<device_ops_api::device_ops_fw_version_rsp_t*>(message.data());
    TEST_VLOG(1) << "=====> FW_Version response received (tag_id: " << std::hex << rsp_tag_id << ") <====";
    if (response->major == 1 && response->minor == 0 && response->patch == 0) {
      status = CmdStatus::CMD_SUCCESSFUL;
    } else {
      status = CmdStatus::CMD_FAILED;
    }

  } else if (rsp_msg_id == device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_DATA_READ_RSP) {
    auto response = reinterpret_cast<device_ops_api::device_ops_data_read_rsp_t*>(message.data());
    TEST_VLOG(1) << "=====> DMA data read command response received (tag_id: " << std::hex << rsp_tag_id << ") <====";
    TEST_VLOG(1) << "     => Total measured latencies (in cycles) ";
    TEST_VLOG(1) << "      - Command Wait time: " << response->cmd_wait_time;
    TEST_VLOG(1) << "      - Command Execution time: " << response->cmd_execution_time;
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
    TEST_VLOG(1) << "=====> DMA data write command response received (tag_id: " << std::hex << rsp_tag_id << ") <====";
    TEST_VLOG(1) << "     => Total measured latencies (in cycles)";
    TEST_VLOG(1) << "      - Command Wait time: " << response->cmd_wait_time;
    TEST_VLOG(1) << "      - Command Execution time: " << response->cmd_execution_time;
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
    TEST_VLOG(1) << "=====> Kernel Launch command response received (tag_id: " << std::hex << rsp_tag_id << ") <====";
    TEST_VLOG(1) << "     => Total measured latencies (in cycles)";
    TEST_VLOG(1) << "      - Command Wait time: " << response->cmd_wait_time;
    TEST_VLOG(1) << "      - Command Execution time: " << response->cmd_execution_time;
    if (response->status == IDevOpsApiCmd::getExpectedRsp(rsp_tag_id)) {
      status = CmdStatus::CMD_SUCCESSFUL;
    } else if (response->status == device_ops_api::DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_TIMEOUT_HANG) {
      status = CmdStatus::CMD_TIMED_OUT;
    } else {
      status = CmdStatus::CMD_FAILED;
    }

  } else if (rsp_msg_id == device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_KERNEL_ABORT_RSP) {
    auto response = reinterpret_cast<device_ops_api::device_ops_kernel_abort_rsp_t*>(message.data());
    TEST_VLOG(1) << "=====> Kernel Abort command response received (tag_id: " << std::hex << rsp_tag_id << ") <====";
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
    return popRsp(deviceIdx);
  }

  // verify the response status for duplication
  if (getCmdResult(rsp_tag_id) != CmdStatus::CMD_RSP_NOT_RECEIVED) {
    updateCmdResult(rsp_tag_id, CmdStatus::CMD_RSP_DUPLICATE);
    return popRsp(deviceIdx);
  }

  if (!updateCmdResult(rsp_tag_id, status)) {
    TEST_VLOG(0) << "Command (tagId: " << std::hex << rsp_tag_id
                 << ") wasn't sent under this scope, discarding this response!";
    return popRsp(deviceIdx);
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

  TEST_VLOG(0) << "=============== TEST SUMMARY ===================";
  TEST_VLOG(0) << "====> Total Commands: " << cmdResults_.size();
  TEST_VLOG(0) << "  ====> Commands successful: " << successfulCmdTags.size();
  TEST_VLOG(0) << "  ====> Commands failed: " << failedCmdTags.size();
  if (failedCmdTags.size() > 0) {
    std::stringstream tagIdsStream;
    for (auto it : failedCmdTags) {
      tagIdsStream << "0x" << std::hex << it << " ";
    }
    TEST_VLOG(0) << "    ----> Tag IDs: " << tagIdsStream.str();
  }
  TEST_VLOG(0) << "  ====> Commands timed out: " << timeoutCmdTags.size();
  if (timeoutCmdTags.size() > 0) {
    std::stringstream tagIdsStream;
    for (auto it : timeoutCmdTags) {
      tagIdsStream << "0x" << std::hex << it << " ";
    }
    TEST_VLOG(0) << "    ----> Tag IDs: " << tagIdsStream.str();
  }
  TEST_VLOG(0) << "  ====> Commands missing responses: " << missingRspCmdTags.size();
  if (missingRspCmdTags.size() > 0) {
    std::stringstream tagIdsStream;
    for (auto it : missingRspCmdTags) {
      tagIdsStream << "0x" << std::hex << it << " ";
    }
    TEST_VLOG(0) << "    ----> Tag IDs: " << tagIdsStream.str();
  }
  TEST_VLOG(0) << "  ====> Commands duplicate responses: " << duplicateRspCmdTags.size();
  if (duplicateRspCmdTags.size() > 0) {
    std::stringstream tagIdsStream;
    for (auto it : duplicateRspCmdTags) {
      tagIdsStream << "0x" << std::hex << it << " ";
    }
    TEST_VLOG(0) << "    ----> Tag IDs: " << tagIdsStream.str();
  }
  std::chrono::duration<double> cmdsTotalDuration = lastCmdTimepoint_ - firstCmdTimepoint_;
  std::chrono::duration<double> rspsTotalDuration = lastRspTimepoint_ - firstRspTimepoint_;
  TEST_VLOG(0) << "  ====> Commands sent per second: " << std::ceil(cmdResults_.size() / cmdsTotalDuration.count());
  TEST_VLOG(0) << "  ====> Responses received per second: "
               << std::ceil(cmdResults_.size() / rspsTotalDuration.count());
  TEST_VLOG(0) << "================================================";

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
        TEST_VLOG(0) << "* gpr[x" << (reg + 3) << "] : a" << i << "    : 0x" << std::hex
                     << context[minionHartID].gpr[reg];
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

void TestDevOpsApi::loadElfToDevice(int deviceIdx, ELFIO::elfio& reader, const std::string& path,
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

  TEST_VLOG(1) << std::endl << "Loading ELF: " << path;
  TEST_VLOG(1) << "  ELF Entry: 0x" << std::hex << entry;
  TEST_VLOG(1) << "  Segment [0]: "
               << "vAddr: 0x" << std::hex << vAddr << ", pAddr: 0x" << std::hex << pAddr << ", Mem Size: 0x" << memSize
               << ", File Size: 0x" << fileSize;

  // Allocate device buffer for the ELF segment
  uint64_t deviceElfSegment0Buffer = getDmaWriteAddr(deviceIdx, ALIGN(memSize, 0x1000));
  kernelEntryAddr = deviceElfSegment0Buffer + (entry - vAddr);

  TEST_VLOG(1) << " Allocated buffer at device address: 0x" << deviceElfSegment0Buffer << " for segment 0";
  TEST_VLOG(1) << " Kernel entry at device address: 0x" << kernelEntryAddr;

  // Create DMA write command
  auto hostVirtAddr = reinterpret_cast<uint64_t>(segment0->get_data());
  auto hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
  stream.push_back(IDevOpsApiCmd::createDataWriteCmd(false, deviceElfSegment0Buffer, hostVirtAddr, hostPhysAddr,
                                                     fileSize, device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
}
