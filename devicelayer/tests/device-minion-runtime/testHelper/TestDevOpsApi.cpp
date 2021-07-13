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
#include <iostream>
#include <numeric>

namespace {
using namespace ELFIO;
using namespace std::chrono_literals;
using namespace dev::dl_tests;
namespace fs = std::experimental::filesystem;
auto kPollingInterval = 10ms;
} // namespace

DEFINE_uint32(exec_timeout, 100, "Internal execution timeout");
DEFINE_string(kernels_dir, "", "Directory where different kernel ELF files are located");
DEFINE_string(trace_logfile, "DeviceFwTrace.log", "File where the MM and CM buffers will be dumped");
DEFINE_bool(loopback_driver, false, "Run on loopback driver");
DEFINE_bool(enable_trace_dump, FLAGS_loopback_driver ? false : true,
            "Enable device trace dump to file specified by flag: trace_logfile, otherwise on UART");
DEFINE_bool(use_epoll, true, "Use EPOLL if true, otherwise use interval based polling");

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

void TestDevOpsApi::waitForSqAvailability(int deviceIdx, int queueIdx) {
  auto& deviceInfo = devices_[static_cast<unsigned long>(deviceIdx)];

  std::unique_lock lk(deviceInfo->asyncEpollMtx_);
  deviceInfo->asyncEpollCondVar_.wait(
    lk, [&deviceInfo, queueIdx]() { return deviceInfo->sqBitmap_ & (0x1U << queueIdx) || deviceInfo->abort_; });
  deviceInfo->sqBitmap_ &= ~(0x1U << queueIdx);
}

void TestDevOpsApi::dispatchStreamAsync(const std::shared_ptr<Stream>& stream) {
  const auto& deviceInfo = devices_[static_cast<unsigned long>(stream->deviceIdx_)];

  int cmdIdx = 0;
  while (!deviceInfo->abort_ && cmdIdx < stream->cmds_.size()) {
    try {
      if (!pushCmd(stream->deviceIdx_, stream->queueIdx_, stream->cmds_[cmdIdx])) {
        waitForSqAvailability(stream->deviceIdx_, stream->queueIdx_);
      } else {
        cmdIdx++;
      }
    } catch (const std::exception& e) {
      TEST_VLOG(0) << "Exception: " << e.what();
      assert(false);
    }
  }
}

void TestDevOpsApi::fExecutor(int deviceIdx, int queueIdx) {
  TEST_VLOG(1) << "Device [" << deviceIdx << "], Queue [" << queueIdx << "] started.";
  auto start = Clock::now();
  auto end = start + execTimeout_;
  auto& deviceInfo = devices_[static_cast<unsigned long>(deviceIdx)];

  auto streamIt = streams_.begin();
  while (!deviceInfo->abort_) {
    if (streamIt != streams_.end()) {
      if ((*streamIt)->deviceIdx_ == deviceIdx && (*streamIt)->queueIdx_ == queueIdx) {
        dispatchStreamAsync(*streamIt);
      }
      streamIt++;
    } else {
      waitForSqAvailability(deviceIdx, queueIdx);
    }
  }
}

void TestDevOpsApi::waitForCqAvailability(int deviceIdx, TimeDuration timeout) {
  auto end = Clock::now() + timeout;
  auto& deviceInfo = devices_[static_cast<unsigned long>(deviceIdx)];
  auto queueCount = getSqCount(deviceIdx);

  uint64_t sqBitmap = 0;
  bool cqAvailable = false;
  while (end > Clock::now() && !cqAvailable) {
    if (FLAGS_use_epoll) {
      devLayer_->waitForEpollEventsMasterMinion(deviceIdx, sqBitmap, cqAvailable);
    } else {
      std::this_thread::sleep_for(kPollingInterval);
      sqBitmap = (0x1U << queueCount) - 1;
      cqAvailable = true;
    }
    if (sqBitmap > 0) {
      std::scoped_lock lk(deviceInfo->asyncEpollMtx_);
      deviceInfo->sqBitmap_ |= sqBitmap;
      deviceInfo->asyncEpollCondVar_.notify_all();
    }
  }
}

void TestDevOpsApi::fListener(int deviceIdx) {
  TEST_VLOG(1) << "Device [" << deviceIdx << "] started.";
  auto end = Clock::now() + execTimeout_;

  auto& deviceInfo = devices_[static_cast<unsigned long>(deviceIdx)];

  auto rspsToReceive = std::accumulate(streams_.begin(), streams_.end(), 0ULL, [deviceIdx](auto& a, auto& b) {
    return b->deviceIdx_ == deviceIdx ? a + b->cmds_.size() : a;
  });

  while (end > Clock::now() && rspsToReceive > 0) {
    try {
      auto res = popRsp(deviceIdx);
      if (!res) {
        waitForCqAvailability(deviceIdx, end - Clock::now());
      } else {
        rspsToReceive--;
        rspsToReceive += handleStreamReTransmission(res.tagId_);
      }
    } catch (const std::exception& e) {
      TEST_VLOG(0) << "Exception: " << e.what();
      assert(false);
    }
  }

  std::scoped_lock lk(deviceInfo->asyncEpollMtx_);
  deviceInfo->abort_ = true;
  deviceInfo->sqBitmap_ = ~0ULL;
  deviceInfo->asyncEpollCondVar_.notify_all();
}

void TestDevOpsApi::executeAsync() {
  std::vector<std::thread> threadVector;
  int deviceCount = getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    auto& deviceInfo = devices_[static_cast<unsigned long>(deviceIdx)];
    deviceInfo->abort_ = false;

    int executorThreadCount = 0;
    auto queueCount = getSqCount(deviceIdx);

    // Create submission threads
    for (int queueIdx = 0; queueIdx < queueCount; queueIdx++) {
      if (std::count_if(streams_.begin(), streams_.end(), [deviceIdx, queueIdx](auto& stream) {
            return stream->deviceIdx_ == deviceIdx && stream->queueIdx_ == queueIdx;
          }) <= 0) {
        continue;
      }
      executorThreadCount++;
      threadVector.push_back(std::thread([this, deviceIdx, queueIdx]() { this->fExecutor(deviceIdx, queueIdx); }));
    }

    if (executorThreadCount != 0) {
      // Create one listener thread per device
      threadVector.push_back(std::thread([this, deviceIdx]() { this->fListener(deviceIdx); }));
    }
  }

  for (auto& t : threadVector) {
    if (t.joinable()) {
      t.join();
    }
  }
}

void TestDevOpsApi::dispatchStreamSync(const std::shared_ptr<Stream>& stream, TimeDuration timeout) {
  auto start = Clock::now();
  auto end = start + timeout;
  for (const auto& cmd : stream->cmds_) {
    try {
      uint64_t sqBitmap = 0x1U << stream->queueIdx_;
      bool cqAvailable;
      while (!(sqBitmap & (1ULL << stream->queueIdx_)) || !pushCmd(stream->deviceIdx_, stream->queueIdx_, cmd)) {
        ASSERT_TRUE(end > Clock::now()) << "\nexecuteSync timed out!\n" << std::endl;
        devLayer_->waitForEpollEventsMasterMinion(stream->deviceIdx_, sqBitmap, cqAvailable);
      }
      cqAvailable = true;
      while (!cqAvailable || !popRsp(stream->deviceIdx_)) {
        ASSERT_TRUE(end > Clock::now()) << "\nexecuteSync timed out!\n" << std::endl;
        devLayer_->waitForEpollEventsMasterMinion(stream->deviceIdx_, sqBitmap, cqAvailable);
      }
    } catch (const std::exception& e) {
      TEST_VLOG(0) << "Exception: " << e.what();
      assert(false);
    }
  }
}

void TestDevOpsApi::executeSyncPerDevice(int deviceIdx) {
  auto start = Clock::now();
  auto end = start + execTimeout_;

  for (auto& stream : streams_) {
    if (stream->deviceIdx_ != deviceIdx) {
      continue;
    }
    dispatchStreamSync(stream, end - Clock::now());
  }
}

void TestDevOpsApi::executeSync() {
  int deviceCount = getDevicesCount();
  std::vector<std::thread> threadVector;
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    auto queueCount = getSqCount(deviceIdx);
    if (std::count_if(streams_.begin(), streams_.end(),
                      [deviceIdx](auto& stream) { return stream->deviceIdx_ == deviceIdx; }) <= 0) {
      continue;
    }
    threadVector.push_back(std::thread([this, deviceIdx]() { this->executeSyncPerDevice(deviceIdx); }));
  }

  for (auto& t : threadVector) {
    if (t.joinable()) {
      t.join();
    }
  }
}

void TestDevOpsApi::execute(bool isAsync) {
  int deviceCount = getDevicesCount();

  std::ofstream logfile;
  if (FLAGS_enable_trace_dump) {
    TEST_VLOG(1) << "Trace data is available at " << fs::current_path() / FLAGS_trace_logfile;
    logfile.open(FLAGS_trace_logfile, std::ios_base::app);
    logfile << "\n\n"
            << ::testing::UnitTest::GetInstance()->current_test_info()->test_case_name() << "."
            << ::testing::UnitTest::GetInstance()->current_test_info()->name() << std::endl;
    for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
      controlTraceLogging(deviceIdx, true /* to trace buffer */, true /* Reset trace buffer. */);
    }
    logfile.close();
  }

  invalidRsps_.clear();

  TEST_VLOG(0) << "Test execution started...";
  isAsync ? executeAsync() : executeSync();

  // Print and validate command results
  printCmdExecutionSummary();

  cleanUpExecution();

  for (int deviceIdx = 0; FLAGS_enable_trace_dump && deviceIdx < deviceCount; deviceIdx++) {
    extractAndPrintTraceData(deviceIdx);
  }
}

void TestDevOpsApi::cleanUpExecution() {
  int deviceCount = getDevicesCount();
  std::unordered_map<int, std::vector<CmdTag>> kernelTagIdsToAbort;
  int abortCmdCount = 0;
  for (const auto& stream : streams_) {
    for (const auto& cmd : stream->cmds_) {
      if (kernelTagIdsToAbort.count(stream->deviceIdx_) == 0) {
        kernelTagIdsToAbort[stream->deviceIdx_] = std::vector<CmdTag>();
      }
      if (IDevOpsApiCmd::getDevOpsApiCmd(cmd)->whoAmI() == CmdType::KERNEL_LAUNCH_CMD &&
          IDevOpsApiCmd::getDevOpsApiCmd(cmd)->getCmdStatus() == CmdStatus::CMD_RSP_NOT_RECEIVED) {
        kernelTagIdsToAbort[stream->deviceIdx_].push_back(cmd);
        abortCmdCount++;
      }
    }
  }

  if (abortCmdCount <= 0) {
    return;
  }

  TEST_VLOG(0) << "Sending abort command(s) for incomplete kernel(s)";
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    if (kernelTagIdsToAbort.find(deviceIdx) == kernelTagIdsToAbort.end()) {
      continue;
    }

    // Make a stream of abort kernel commands
    std::vector<CmdTag> cmds;
    for (auto it : kernelTagIdsToAbort[deviceIdx]) {
      cmds.push_back(
        IDevOpsApiCmd::createCmd<KernelAbortCmd>(false, it, device_ops_api::DEV_OPS_API_KERNEL_ABORT_RESPONSE_SUCCESS));
    }

    // Execute the abort commands using SQ_0
    auto stream = std::make_shared<Stream>(deviceIdx, 0 /* queueIdx */, std::move(cmds));
    dispatchStreamSync(stream, std::chrono::seconds(20));
    cmds.clear();

    // Extract and discard any pending responses
    while (popRsp(deviceIdx))
      ;
  }

  int missingRspCmdsCount = 0;
  for (auto& [devIdx, tagIdsVec] : kernelTagIdsToAbort) {
    missingRspCmdsCount += std::count_if(tagIdsVec.begin(), tagIdsVec.end(), [](auto tagId) {
      return IDevOpsApiCmd::getDevOpsApiCmd(tagId)->getCmdStatus() == CmdStatus::CMD_RSP_NOT_RECEIVED;
    });
  }
  EXPECT_EQ(missingRspCmdsCount, 0) << "Failed to abort kernel launches!";
}

void TestDevOpsApi::resetMemPooltoDefault(int deviceIdx) {
  if (deviceIdx >= static_cast<int>(devices_.size())) {
    throw Exception("deviceIdx: " + std::to_string(deviceIdx) + " does not exist!");
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
  std::scoped_lock lock(deviceInfo->dmaWriteAddrMtx_);
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
  std::scoped_lock lock(deviceInfo->dmaReadAddrMtx_);
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

bool TestDevOpsApi::pushCmd(int deviceIdx, int queueIdx, CmdTag tagId) {
  auto devOpsApiCmd = IDevOpsApiCmd::getDevOpsApiCmd(tagId);
  if (!devOpsApiCmd) {
    throw Exception("Invalid CmdTag: " + std::to_string(tagId));
  }

  auto res = devLayer_->sendCommandMasterMinion(deviceIdx, queueIdx, devOpsApiCmd->getCmdPtr(),
                                                devOpsApiCmd->getCmdSize(), devOpsApiCmd->isDma());
  if (!res) {
    return res;
  }

  TEST_VLOG(1) << "=====> Command Sent (tag_id: " << devOpsApiCmd->getCmdTagId() << ") <====";

  if (bytesSent_ == 0) {
    firstCmdTimepoint_ = Clock::now();
  } else {
    lastCmdTimepoint_ = Clock::now();
  }
  bytesSent_ += devOpsApiCmd->getCmdSize();
  return res;
}

TestDevOpsApi::PopRspResult TestDevOpsApi::popRsp(int deviceIdx) {
  std::vector<std::byte> rspMem;
  auto res = devLayer_->receiveResponseMasterMinion(deviceIdx, rspMem);
  if (!res) {
    return {res};
  }

  auto rspHdr = templ::bit_cast<device_ops_api::rsp_header_t*>(rspMem.data());
  if (bytesReceived_ == 0) {
    firstRspTimepoint_ = Clock::now();
  } else {
    lastRspTimepoint_ = Clock::now();
  }
  bytesReceived_ += rspHdr->rsp_hdr.size;
  auto rspTagId = rspHdr->rsp_hdr.tag_id;

  auto devOpsApiCmd = IDevOpsApiCmd::getDevOpsApiCmd(rspTagId);
  if (!devOpsApiCmd || devOpsApiCmd->setRsp(rspMem) == CmdStatus::CMD_RSP_NOT_RECEIVED) {
    invalidRsps_.push_back(rspTagId);
    return popRsp(deviceIdx);
  }

  if (devOpsApiCmd->getCmdStatus() == CmdStatus::CMD_RSP_DUPLICATE) {
    return popRsp(deviceIdx);
  }

  TEST_VLOG(1) << devOpsApiCmd->printSummary();

  return {rspTagId};
}

size_t TestDevOpsApi::handleStreamReTransmission(CmdTag tagId) {
  auto devOpsApiCmd = IDevOpsApiCmd::getDevOpsApiCmd(tagId);
  if (!devOpsApiCmd) {
    return 0;
  }

  if (devOpsApiCmd->getCmdStatus() == CmdStatus::CMD_SUCCESSFUL) {
    return 0;
  }

  std::shared_ptr<Stream> stream = nullptr;
  for (auto& it : streams_) {
    if (std::find(it->cmds_.begin(), it->cmds_.end(), tagId) != it->cmds_.end()) {
      // found the stream corresponding to defaulter tagId
      stream = it;
      break;
    }
  }

  if (!stream || !stream->retryCount_) {
    return 0;
  }

  if (auto newStream = stream->getReTransmissionStream()) {
    streams_.push_back(std::move(newStream));
    return streams_.back()->cmds_.size();
  }
  return 0;
}

void TestDevOpsApi::printCmdExecutionSummary() {
  int reTransmittedStreams = 0;
  std::vector<CmdTag> allCmdTags;
  for (const auto& stream : streams_) {
    if (stream->reTransmitted_) {
      reTransmittedStreams++;
      continue;
    }
    allCmdTags.insert(allCmdTags.end(), stream->cmds_.begin(), stream->cmds_.end());
  }

  std::vector<CmdTag> successfulCmdTags(allCmdTags.size());
  auto it = std::copy_if(allCmdTags.begin(), allCmdTags.end(), successfulCmdTags.begin(), [](auto tagId) {
    return IDevOpsApiCmd::getDevOpsApiCmd(tagId)->getCmdStatus() == CmdStatus::CMD_SUCCESSFUL;
  });
  successfulCmdTags.resize(std::distance(successfulCmdTags.begin(), it));

  std::vector<CmdTag> failedCmdTags(allCmdTags.size());
  it = std::copy_if(allCmdTags.begin(), allCmdTags.end(), failedCmdTags.begin(), [](auto tagId) {
    return IDevOpsApiCmd::getDevOpsApiCmd(tagId)->getCmdStatus() == CmdStatus::CMD_FAILED;
  });
  failedCmdTags.resize(std::distance(failedCmdTags.begin(), it));

  std::vector<CmdTag> timeoutCmdTags(allCmdTags.size());
  it = std::copy_if(allCmdTags.begin(), allCmdTags.end(), timeoutCmdTags.begin(), [](auto tagId) {
    return IDevOpsApiCmd::getDevOpsApiCmd(tagId)->getCmdStatus() == CmdStatus::CMD_TIMED_OUT;
  });
  timeoutCmdTags.resize(std::distance(timeoutCmdTags.begin(), it));

  std::vector<CmdTag> missingRspCmdTags(allCmdTags.size());
  it = std::copy_if(allCmdTags.begin(), allCmdTags.end(), missingRspCmdTags.begin(), [](auto tagId) {
    return IDevOpsApiCmd::getDevOpsApiCmd(tagId)->getCmdStatus() == CmdStatus::CMD_RSP_NOT_RECEIVED;
  });
  missingRspCmdTags.resize(std::distance(missingRspCmdTags.begin(), it));

  std::vector<CmdTag> duplicateRspCmdTags(allCmdTags.size());
  it = std::copy_if(allCmdTags.begin(), allCmdTags.end(), duplicateRspCmdTags.begin(), [](auto tagId) {
    return IDevOpsApiCmd::getDevOpsApiCmd(tagId)->getCmdStatus() == CmdStatus::CMD_RSP_DUPLICATE;
  });
  duplicateRspCmdTags.resize(std::distance(duplicateRspCmdTags.begin(), it));

  std::stringstream summary;
  summary << "\n=============== TEST SUMMARY ==================="
          << "\n====> Total Commands: " << allCmdTags.size()
          << "\n\t====> Commands successful: " << successfulCmdTags.size()
          << "\n\t====> Commands failed: " << failedCmdTags.size();
  if (!failedCmdTags.empty()) {
    summary << "\n\t\t----> Tag IDs: ";
    for (auto tagIdIt : failedCmdTags) {
      summary << "0x" << std::hex << tagIdIt << " ";
    }
  }

  summary << "\n\t====> Commands timed out: " << timeoutCmdTags.size();
  if (!timeoutCmdTags.empty()) {
    summary << "\n\t\t----> Tag IDs: ";
    for (auto tagIdIt : timeoutCmdTags) {
      summary << "0x" << std::hex << tagIdIt << " ";
    }
  }
  summary << "\n\t====> Commands missing responses: " << missingRspCmdTags.size();
  if (!missingRspCmdTags.empty()) {
    summary << "\n\t\t----> Tag IDs: ";
    for (auto tagIdIt : missingRspCmdTags) {
      summary << "0x" << std::hex << tagIdIt << " ";
    }
  }
  summary << "\n\t====> Commands duplicate responses: " << duplicateRspCmdTags.size();
  if (!duplicateRspCmdTags.empty()) {
    summary << "\n\t\t----> Tag IDs: ";
    for (auto tagIdIt : duplicateRspCmdTags) {
      summary << "0x" << std::hex << tagIdIt << " ";
    }
  }
  summary << "\n\t====> Corrupted/old responses: " << invalidRsps_.size();
  if (!invalidRsps_.empty()) {
    summary << "\n\t\t----> Tag IDs: ";
    for (auto tagIdIt : invalidRsps_) {
      summary << "0x" << std::hex << tagIdIt << " ";
    }
  }
  summary << "\n\t====> Retransmitted Streams: " << reTransmittedStreams;
  std::chrono::duration<double> cmdsTotalDuration = lastCmdTimepoint_ - firstCmdTimepoint_;
  std::chrono::duration<double> rspsTotalDuration = lastRspTimepoint_ - firstRspTimepoint_;
  summary << "\n\t====> Commands Sent / second: "
          << std::ceil(static_cast<double>(allCmdTags.size()) / cmdsTotalDuration.count())
          << "\n\t====> Bytes Sent / second: " << std::setprecision(10)
          << std::ceil(static_cast<double>(bytesSent_) / cmdsTotalDuration.count())
          << "\n\t====> Responses Received / second: "
          << std::ceil(static_cast<double>(allCmdTags.size()) / rspsTotalDuration.count())
          << "\n\t====> Bytes Received / second: " << std::setprecision(10)
          << std::ceil(static_cast<double>(bytesReceived_) / rspsTotalDuration.count())
          << "\n================================================";

  TEST_VLOG(0) << summary.str();

  EXPECT_EQ(failedCmdTags.size(), 0);
  EXPECT_EQ(missingRspCmdTags.size(), 0);
  EXPECT_EQ(duplicateRspCmdTags.size(), 0);
  EXPECT_EQ(timeoutCmdTags.size(), 0);
}

void TestDevOpsApi::printErrorContext(int queueId, const std::byte* buffer, uint64_t shireMask, CmdTag tagId) const {
  std::stringstream logs;
  std::ofstream logfile;
  if (FLAGS_enable_trace_dump) {
    logfile.open(FLAGS_trace_logfile, std::ios_base::app);
  }

  auto context = templ::bit_cast<const hartExecutionContext*>(buffer);
  logs << "\n*** Error Context Start (Queue ID: " << queueId << ")***" << std::endl;
  logs << "  * RUNTIME CONTEXT *" << std::endl;
  auto devOpsApiCmd = IDevOpsApiCmd::getDevOpsApiCmd(tagId);
  if (!devOpsApiCmd) {
    throw Exception("Command with tagId: " + std::to_string(tagId) + " does not exist!");
  }
  logs << devOpsApiCmd->printSummary();
  logs << "--------------------------" << std::endl;
  for (int shireId = 0; shireId < 32; shireId++) {
    if (!(shireMask & (1 << shireId))) {
      continue;
    }

    // print the context of the first hart in a shire only
    auto minionHartID = shireId * 64;
    logs << "  * DEVICE CONTEXT *" << std::endl;
    logs << "* HartID: " << context[minionHartID].hart_id << std::endl;
    switch (context[minionHartID].type) {
    case CM_CONTEXT_TYPE_USER_KERNEL_ERROR:
      logs << "* Type: User Kernel Error" << std::endl;
      logs << "* Error cycles: " << context[minionHartID].cycles << std::endl;
      logs << "* Error code: " << context[minionHartID].user_error << std::endl;
      break;
    case CM_CONTEXT_TYPE_HANG:
      logs << "* Type: Compute Minion Hang" << std::endl;
      break;
    case CM_CONTEXT_TYPE_UMODE_EXCEPTION:
      logs << "* Type: U-mode Exception" << std::endl;
      break;
    case CM_CONTEXT_TYPE_SMODE_EXCEPTION:
      logs << "* Type: S-mode Exception" << std::endl;
      break;
    case CM_CONTEXT_TYPE_SYSTEM_ABORT:
      logs << "* Type: System Abort" << std::endl;
      break;
    case CM_CONTEXT_TYPE_SELF_ABORT:
      logs << "* Type: Kernel Self Abort" << std::endl;
      break;
    default:
      logs << "* Type: Undefined" << std::endl;
    }
    if (context[minionHartID].type != CM_CONTEXT_TYPE_USER_KERNEL_ERROR) {
      std::array<std::string, 31> arr = {"ra", "sp", "gp", "tp", "t0", "t1", "t2", "s0/fp", "s1", "a0", "a1",
                                         "a2", "a3", "a4", "a5", "a6", "a7", "s2", "s3",    "s4", "s5", "s6",
                                         "s7", "s8", "s9", "sa", "sb", "t3", "t4", "t5",    "t6"};
      logs << "* Error cycles: " << context[minionHartID].cycles << std::endl;
      logs << "* epc: 0x" << std::hex << context[minionHartID].sepc << std::endl;
      logs << "* status: 0x" << std::hex << context[minionHartID].sstatus << std::endl;
      logs << "* tval: 0x" << std::hex << context[minionHartID].stval << std::endl;
      logs << "* cause: 0x" << std::hex << context[minionHartID].scause << std::endl;
      for (auto i = 0; i < 31; i++) {
        logs << "* gpr[x" << std::hex << i + 1 << "] :" << arr[i] << ": 0x" << std::hex << context[minionHartID].gpr[i]
             << std::endl;
      }
    }
    logs << "--------------------------" << std::endl;
  }
  logs << "*** Error Context End ***" << std::endl;

  if (FLAGS_enable_trace_dump) {
    logfile << logs.str();
    logfile.close();
  } else {
    TEST_VLOG(0) << logs.str();
  }
}

// Trace Command type packet decoding string messages
const std::array<std::string, 5> mmCmdTypeString = {"",
  "MM_DMA_READ_CMD",
  "MM_DMA_WRITE_CMD",
  "MM_KERNEL_LAUNCH_CMD",
  "MM_KERNEL_ABORT_CMD"};

// Trace Command status packet decoding string messages
const std::array<std::string, 7> cmdStatusString = {"",
  "CMD_STATUS_WAIT_BARRIER",
  "CMD_STATUS_RECEIVED",
  "CMD_STATUS_EXECUTING",
  "CMD_STATUS_FAILED",
  "CMD_STATUS_ABORTED",
  "CMD_STATUS_SUCCEEDED"};

bool TestDevOpsApi::printMMTraceStringData(unsigned char* traceBuf, size_t bufSize) const {
  std::stringstream logs;
  std::ofstream logfile;
  if (FLAGS_enable_trace_dump) {
    logfile.open(FLAGS_trace_logfile, std::ios_base::app);
  }

  // Get Trace buffer header
  auto traceHeader = templ::bit_cast<trace_buffer_std_header_t*>(traceBuf);
  size_t dataSize = traceHeader->data_size - sizeof(struct trace_buffer_std_header_t);

  // Check if it is valid MM Trace buffer.
  if ((traceHeader->magic_header != TRACE_MAGIC_HEADER) || (traceHeader->type != TRACE_MM_BUFFER) ||
      ((dataSize + sizeof(struct trace_buffer_std_header_t)) > bufSize)) {
    TEST_VLOG(0) << "Invalid MM Trace Buffer!";
    return false;
  }

  char stringLog[TRACE_STRING_MAX_SIZE + 1]; //NOSONAR For Device Trace string processing.
  // Get size from Trace buffer header
  traceBuf = traceBuf + sizeof(struct trace_buffer_std_header_t);
  auto packetHeader = templ::bit_cast<trace_entry_header_mm_t*>(traceBuf);
  const mm_trace_string_t* tracePacketString;
  const mm_trace_cmd_status_t* tracePacketCmdStatus;

  bool validStringEventFound = false;
  size_t dataPopped = 0;

  while (dataPopped < dataSize) {
    if (packetHeader->type == TRACE_TYPE_STRING) {
      tracePacketString = templ::bit_cast<mm_trace_string_t*>(traceBuf + dataPopped);
      strncpy(&stringLog[0], tracePacketString->dataString, TRACE_STRING_MAX_SIZE);
      stringLog[TRACE_STRING_MAX_SIZE] = '\0';
      logs << "Timestamp:" << tracePacketString->mm_header.cycle                \
        << " H:" << tracePacketString->mm_header.hart_id << " :" << stringLog << std::endl;
      dataPopped += sizeof(struct mm_trace_string_t);
      validStringEventFound = true;
    }
    else if (packetHeader->type == TRACE_TYPE_CMD_STATUS) {
      tracePacketCmdStatus = templ::bit_cast<mm_trace_cmd_status_t*>(traceBuf + dataPopped);
      logs << "Timestamp:" << tracePacketCmdStatus->mm_header.cycle             \
       << "\tCMD:" << mmCmdTypeString[tracePacketCmdStatus->cmd.cmd_type]       \
       << "\tSTATUS:" << cmdStatusString[tracePacketCmdStatus->cmd.cmd_status]  \
       << "\tSQ_ID:" << (uint16_t)tracePacketCmdStatus->cmd.queue_slot_id       \
       << "\tTAG_ID:" << tracePacketCmdStatus->cmd.trans_id << std::endl;
      dataPopped += sizeof(struct mm_trace_cmd_status_t);
    } else {
      break;
    }
    packetHeader = templ::bit_cast<trace_entry_header_mm_t*>(traceBuf + dataPopped);
  }
  if (FLAGS_enable_trace_dump) {
    logfile << logs.str();
    logfile.close();
  } else {
    TEST_VLOG(0) << logs.str();
  }

  return validStringEventFound;
}

bool TestDevOpsApi::printCMTraceStringData(unsigned char* traceBuf, size_t bufSize) const {
  std::stringstream logs;
  std::ofstream logfile;
  if (FLAGS_enable_trace_dump) {
    logfile.open(FLAGS_trace_logfile, std::ios_base::app);
  }

  // Get size from Trace buffer header
  auto traceHeader = templ::bit_cast<trace_buffer_std_header_t*>(traceBuf);

  // Check if it is valid CM Trace buffer.
  if ((traceHeader->magic_header != TRACE_MAGIC_HEADER) || (traceHeader->type != TRACE_CM_BUFFER)) {
    TEST_VLOG(0) << "Invalid CM Trace Buffer!";
    return false;
  }

  size_t headerSize = sizeof(struct trace_buffer_std_header_t);
  size_t dataSize = traceHeader->data_size - headerSize;
  auto perHartBufSize = bufSize / WORKER_HART_COUNT;
  uint32_t cmHartID;
  size_t dataPopped;
  char stringLog[TRACE_STRING_MAX_SIZE + 1]; //NOSONAR For Device Trace string processing.
  auto hartDataPtr = traceBuf;
  bool validStringEventFound = false;

  for (int i = 0; i < WORKER_HART_COUNT; ++i) {
    // For CM buffers (except first HART's buffer) Header is just size of data in buffer.
    if (i != 0) {
      headerSize = sizeof(struct trace_buffer_size_header_t);
      auto size_header = templ::bit_cast<struct trace_buffer_size_header_t*>(hartDataPtr);
      dataSize = size_header->data_size - sizeof(struct trace_buffer_size_header_t);
    }

    // Last 32 harts of MM Shire are working as Compute worker, adjust HART ID based on that.
    cmHartID = i < MM_BASE_ID ? i : i + 32;

    if ((dataSize + headerSize) > perHartBufSize) {
      // This buffer is not valid, no need parse this. Move to next Hart's buffer
      hartDataPtr += CM_SIZE_PER_HART;
      continue;
    }

    // Move the pointer on top of trace buffer header.
    hartDataPtr = hartDataPtr + headerSize;
    const cm_trace_string_t* tracePacketString;
    const cm_trace_cmd_status_t* tracePacketCmdStatus;
    auto packetHeader = templ::bit_cast<trace_entry_header_t*>(hartDataPtr);
    dataPopped = 0;

    while (dataPopped < dataSize) {
      if (packetHeader->type == TRACE_TYPE_STRING) {
        tracePacketString = templ::bit_cast<cm_trace_string_t*>(hartDataPtr + dataPopped);
        strncpy(&stringLog[0], tracePacketString->dataString, TRACE_STRING_MAX_SIZE);
        stringLog[TRACE_STRING_MAX_SIZE] = '\0';
        logs << "Timestamp:" << tracePacketString->header.cycle << "H:"           \
          << cmHartID << ":" << stringLog << std::endl;
        dataPopped += sizeof(struct cm_trace_string_t);
        validStringEventFound = true;
      }
      else if (packetHeader->type == TRACE_TYPE_CMD_STATUS) {
        tracePacketCmdStatus = templ::bit_cast<cm_trace_cmd_status_t*>(hartDataPtr + dataPopped);
        logs << "Timestamp:" << tracePacketCmdStatus->header.cycle                \
          << "\tCMD:" << mmCmdTypeString[tracePacketCmdStatus->cmd.cmd_type]      \
          << "\tSTATUS:" << cmdStatusString[tracePacketCmdStatus->cmd.cmd_status] \
          << "\tSQ_ID:" << (uint16_t)tracePacketCmdStatus->cmd.queue_slot_id      \
          << "\tTAD_ID:" << tracePacketCmdStatus->cmd.trans_id << std::endl;
        dataPopped += sizeof(struct cm_trace_cmd_status_t);
      } else {
        break;
      }
      packetHeader = templ::bit_cast<trace_entry_header_t*>(hartDataPtr + dataPopped);
    }
    hartDataPtr += (CM_SIZE_PER_HART - headerSize);
  }
  if (FLAGS_enable_trace_dump) {
    logfile << logs.str();
    logfile.close();
  } else {
    TEST_VLOG(0) << logs.str();
  }

  return validStringEventFound;
}

void TestDevOpsApi::extractAndPrintTraceData(int deviceIdx) {
  const size_t mmBufSize = 1024 * 1024;
  const size_t cmBufSize = CM_SIZE_PER_HART * WORKER_HART_COUNT;
  auto rdBufMem = allocDmaBuffer(deviceIdx, mmBufSize + cmBufSize, false /* read buffer */);
  device_ops_api::dma_read_node rdNode = {.dst_host_virt_addr = templ::bit_cast<uint64_t>(rdBufMem),
                                          .dst_host_phy_addr = 0,
                                          .src_device_phy_addr = 0,
                                          .size = static_cast<uint32_t>(mmBufSize + cmBufSize)};
  std::vector<CmdTag> cmds;
  cmds.push_back(IDevOpsApiCmd::createCmd<DmaReadListCmd>(
    device_ops_api::CMD_FLAGS_BARRIER_DISABLE | device_ops_api::CMD_FLAGS_MMFW_TRACEBUF |
      device_ops_api::CMD_FLAGS_CMFW_TRACEBUF,
    &rdNode, 1 /* single node */, device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));

  auto stream = std::make_shared<Stream>(deviceIdx, 0 /* queueIdx */, std::move(cmds));
  dispatchStreamSync(stream, std::chrono::seconds(10));

  if (std::count_if(stream->cmds_.begin(), stream->cmds_.end(), [](auto tagId) {
        return IDevOpsApiCmd::getDevOpsApiCmd(tagId)->getCmdStatus() == CmdStatus::CMD_SUCCESSFUL;
      }) == stream->cmds_.size()) {
    printMMTraceStringData(static_cast<unsigned char*>(rdBufMem), cmBufSize);
    printCMTraceStringData(static_cast<unsigned char*>(rdBufMem) + mmBufSize, mmBufSize);
  } else {
    TEST_VLOG(0) << "Failed to pull DMA trace buffers!";
  }

  freeDmaBuffer(rdBufMem);
}

void TestDevOpsApi::controlTraceLogging(int deviceIdx, bool toTraceBuf, bool resetTraceBuf) {
  std::vector<CmdTag> cmds;

  uint32_t control = device_ops_api::TRACE_RT_CONTROL_ENABLE_TRACE;
  // Redirect logs to Trace or UART.
  control |= toTraceBuf ? device_ops_api::TRACE_RT_CONTROL_LOG_TO_TRACE : device_ops_api::TRACE_RT_CONTROL_LOG_TO_UART;
  // Reset Trace buffer.
  control |= resetTraceBuf ? device_ops_api::TRACE_RT_CONTROL_RESET_TRACEBUF : 0x0;

  // redirect MM and CM trace logging to TraceBuf/UART
  cmds.push_back(IDevOpsApiCmd::createCmd<TraceRtControlCmd>(
    device_ops_api::CMD_FLAGS_BARRIER_DISABLE, device_ops_api::TRACE_RT_TYPE_MM | device_ops_api::TRACE_RT_TYPE_CM,
    control, device_ops_api::DEV_OPS_TRACE_RT_CONTROL_RESPONSE_SUCCESS));

  auto stream = std::make_shared<Stream>(deviceIdx, 0 /* queueIdx */, std::move(cmds));
  dispatchStreamSync(stream, std::chrono::seconds(10));

  if (std::count_if(stream->cmds_.begin(), stream->cmds_.end(), [](auto tagId) {
        return IDevOpsApiCmd::getDevOpsApiCmd(tagId)->getCmdStatus() == CmdStatus::CMD_SUCCESSFUL;
      }) != stream->cmds_.size()) {
    TEST_VLOG(0) << "Failed to redirect tracing of buffer, disabling trace dump!";
    FLAGS_enable_trace_dump = false;
  }
}

void TestDevOpsApi::loadElfToDevice(int deviceIdx, ELFIO::elfio& reader, const std::string& path,
                                    std::vector<CmdTag>& cmds, uint64_t& kernelEntryAddr) {
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
  auto hostVirtAddr = templ::bit_cast<uint64_t>(segment0->get_data());
  cmds.push_back(IDevOpsApiCmd::createCmd<DataWriteCmd>(false, deviceElfSegment0Buffer, hostVirtAddr,
                                                        fileSize, device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
}

void TestDevOpsApi::insertStream(int deviceIdx, int queueIdx, std::vector<CmdTag> cmds, unsigned int retryCount) {
  streams_.push_back(std::make_shared<Stream>(deviceIdx, queueIdx, std::move(cmds), retryCount));
}

void TestDevOpsApi::deleteStreams() {
  streams_.clear();
  IDevOpsApiCmd::deleteDevOpsApiCmds();
}
