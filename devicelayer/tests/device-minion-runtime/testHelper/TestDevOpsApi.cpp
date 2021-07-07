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
  uint64_t sqBitmap = (0x1U << queueCount) - 1;
  bool cqAvailable = true;

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
        if (FLAGS_use_epoll) {
          devLayer_->waitForEpollEventsMasterMinion(deviceIdx, sqBitmap, cqAvailable);
        } else {
          std::this_thread::sleep_for(kPollingInterval);
          sqBitmap = (0x1U << queueCount) - 1;
          cqAvailable = true;
        }
      }
      if (cqAvailable) {
        while (popRsp(deviceIdx)) {
          rspsToReceive--;
        }
      }
      if (sqBitmap > 0) {
        {
          std::lock_guard<std::mutex> lk(deviceInfo->sqBitmapMtx_);
          deviceInfo->sqBitmap_ |= sqBitmap;
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

  for (int deviceIdx = 0; FLAGS_enable_trace_dump && deviceIdx < deviceCount; deviceIdx++) {
    extractAndPrintTraceData(deviceIdx);
    controlTraceLogging(deviceIdx, false /* to UART */, false /* don't reset Trace buffer*/);
  }
}

void TestDevOpsApi::executeSyncPerDevicePerQueue(int deviceIdx, int queueIdx, const std::vector<CmdTag>& stream) {
  auto start = Clock::now();
  auto end = start + execTimeout_;
  uint64_t sqBitmap = 0x1U << queueIdx;
  bool cqAvailable = true;
  for (auto& cmd : stream) {
    try {
      while (!(sqBitmap & (1ULL << queueIdx)) || !pushCmd(deviceIdx, queueIdx, cmd)) {
        ASSERT_TRUE(end > Clock::now()) << "\nexecuteSync timed out!\n" << std::endl;
        devLayer_->waitForEpollEventsMasterMinion(deviceIdx, sqBitmap, cqAvailable);
      }
      while (!cqAvailable || !popRsp(deviceIdx)) {
        ASSERT_TRUE(end > Clock::now()) << "\nexecuteSync timed out!\n" << std::endl;
        devLayer_->waitForEpollEventsMasterMinion(deviceIdx, sqBitmap, cqAvailable);
      }
    } catch (const std::exception& e) {
      TEST_VLOG(0) << "Exception: " << e.what();
      assert(false);
    }
  }
}

void TestDevOpsApi::executeSync() {
  std::vector<std::thread> threadVector;
  int deviceCount = getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    auto queueCount = getSqCount(deviceIdx);
    for (int queueIdx = 0; queueIdx < queueCount; queueIdx++) {
      if (streams_.find(key(deviceIdx, queueIdx)) == streams_.end()) {
        continue;
      }
      TEST_VLOG(0) << "Device[" << deviceIdx << "], Queue[" << queueIdx << "]: test execution started...";
      threadVector.push_back(std::thread([this, deviceIdx, queueIdx]() {
        this->executeSyncPerDevicePerQueue(deviceIdx, queueIdx, streams_[key(deviceIdx, queueIdx)]);
      }));
    }
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
  std::unordered_map<int, std::vector<CmdTag>> kernelTagIdsToAbort;
  int abortCmdCount = 0;
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    auto queueCount = getSqCount(deviceIdx);
    std::vector<CmdTag> kernelTagIdsToAbortPerDev;

    for (int queueIdx = 0; queueIdx < queueCount; queueIdx++) {
      if (streams_.find(key(deviceIdx, queueIdx)) == streams_.end()) {
        continue;
      }

      kernelTagIdsToAbortPerDev.resize(streams_[key(deviceIdx, queueIdx)].size());
      auto it =
        std::copy_if(streams_[key(deviceIdx, queueIdx)].begin(), streams_[key(deviceIdx, queueIdx)].end(),
                     kernelTagIdsToAbortPerDev.begin(), [](auto tagId) {
                       return IDevOpsApiCmd::getDevOpsApiCmd(tagId)->whoAmI() == CmdType::KERNEL_LAUNCH_CMD &&
                              IDevOpsApiCmd::getDevOpsApiCmd(tagId)->getCmdStatus() == CmdStatus::CMD_RSP_NOT_RECEIVED;
                     });
      kernelTagIdsToAbortPerDev.resize(std::distance(kernelTagIdsToAbortPerDev.begin(), it));
      abortCmdCount += kernelTagIdsToAbortPerDev.size();
      kernelTagIdsToAbort.emplace(deviceIdx, std::move(kernelTagIdsToAbortPerDev));
    }
  }

  if (abortCmdCount > 0) {
    TEST_VLOG(0) << "Sending abort command(s) for incomplete kernel(s)";
    std::vector<std::thread> threadVector;
    for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
      if (kernelTagIdsToAbort.find(deviceIdx) == kernelTagIdsToAbort.end()) {
        continue;
      }

      // Make a stream of abort kernel commands
      std::vector<CmdTag> stream;
      for (auto it : kernelTagIdsToAbort[deviceIdx]) {
        stream.push_back(IDevOpsApiCmd::createCmd<KernelAbortCmd>(
          false, it, device_ops_api::DEV_OPS_API_KERNEL_ABORT_RESPONSE_SUCCESS));
      }

      // Execute the abort commands using SQ_0
      execTimeout_ = std::chrono::seconds(20);
      executeSyncPerDevicePerQueue(deviceIdx, 0 /* queueIdx */, stream);
      stream.clear();
    }

    int missingRspCmdsCount = 0;
    for (auto& [devIdx, tagIdsVec] : kernelTagIdsToAbort) {
      missingRspCmdsCount += std::count_if(tagIdsVec.begin(), tagIdsVec.end(), [](auto tagId) {
        return IDevOpsApiCmd::getDevOpsApiCmd(tagId)->getCmdStatus() == CmdStatus::CMD_RSP_NOT_RECEIVED;
      });
    }
    EXPECT_EQ(missingRspCmdsCount, 0) << "Failed to abort kernel launches!";
  }
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

bool TestDevOpsApi::pushCmd(int deviceIdx, int queueIdx, CmdTag tagId) {
  auto devOpsApiCmd = IDevOpsApiCmd::getDevOpsApiCmd(tagId);
  if (devOpsApiCmd == nullptr) {
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

bool TestDevOpsApi::popRsp(int deviceIdx) {
  std::vector<std::byte> rspMem;
  auto res = devLayer_->receiveResponseMasterMinion(deviceIdx, rspMem);
  if (!res) {
    return res;
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
  if (devOpsApiCmd == nullptr) {
    return false;
  }

  if (devOpsApiCmd->setRsp(rspMem) == CmdStatus::CMD_RSP_NOT_RECEIVED) {
    invalidRsps_.push_back(rspTagId);
    return popRsp(deviceIdx);
  }

  TEST_VLOG(1) << devOpsApiCmd->printSummary();

  return res;
}

void TestDevOpsApi::printCmdExecutionSummary() {
  std::vector<CmdTag> allCmdTags;

  for (auto& [tagId, stream] : streams_) {
    if (!stream.size()) {
      continue;
    }
    allCmdTags.insert(allCmdTags.end(), stream.begin(), stream.end());
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
  if (devOpsApiCmd == nullptr) {
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

  std::string stringLog(TRACE_STRING_MAX_SIZE + 1, '\0');
  // Get size from Trace buffer header
  traceBuf = traceBuf + sizeof(struct trace_buffer_std_header_t);
  auto dataPtr = templ::bit_cast<trace_string_mm_t*>(traceBuf);
  bool validStringEventFound = false;
  size_t dataPopped = 0;

  while (dataPopped < dataSize) {
    if ((dataPtr->mm_header.type == TRACE_TYPE_STRING) && (dataPtr->dataString[0] != '\0')) {
      strncpy(&stringLog[0], dataPtr->dataString, TRACE_STRING_MAX_SIZE);
      stringLog[TRACE_STRING_MAX_SIZE] = '\0';
      logs << "H:" << dataPtr->mm_header.hart_id << ":" << stringLog << std::endl;
      dataPtr++;
      dataPopped += sizeof(struct trace_string_mm_t);
      validStringEventFound = true;
    } else {
      break;
    }
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
  std::string stringLog(TRACE_STRING_MAX_SIZE + 1, '\0');
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
    auto dataPtr = templ::bit_cast<trace_string_t*>(hartDataPtr);
    dataPopped = 0;

    while (dataPopped < dataSize) {
      if ((dataPtr->header.type == TRACE_TYPE_STRING) && (dataPtr->dataString[0] != '\0')) {
        strncpy(&stringLog[0], dataPtr->dataString, TRACE_STRING_MAX_SIZE);
        stringLog[TRACE_STRING_MAX_SIZE] = '\0';
        logs << "H:" << cmHartID << ":" << stringLog << std::endl;
        dataPtr++;
        dataPopped += sizeof(struct trace_string_mm_t);
        validStringEventFound = true;
      } else {
        break;
      }
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
  std::vector<CmdTag> stream;
  stream.push_back(IDevOpsApiCmd::createCmd<DmaReadListCmd>(
    device_ops_api::CMD_FLAGS_BARRIER_DISABLE | device_ops_api::CMD_FLAGS_MMFW_TRACEBUF |
      device_ops_api::CMD_FLAGS_CMFW_TRACEBUF,
    &rdNode, 1 /* single node */, device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));

  executeSyncPerDevicePerQueue(deviceIdx, 0 /* queueIdx */, stream);

  if (std::count_if(stream.begin(), stream.end(), [](auto tagId) {
        return IDevOpsApiCmd::getDevOpsApiCmd(tagId)->getCmdStatus() == CmdStatus::CMD_SUCCESSFUL;
      }) == stream.size()) {
    printMMTraceStringData(static_cast<unsigned char*>(rdBufMem), cmBufSize);
    printCMTraceStringData(static_cast<unsigned char*>(rdBufMem) + mmBufSize, mmBufSize);
  } else {
    TEST_VLOG(0) << "Failed to pull DMA trace buffers!";
  }

  freeDmaBuffer(rdBufMem);
  stream.clear();
}

void TestDevOpsApi::controlTraceLogging(int deviceIdx, bool toTraceBuf, bool resetTraceBuf) {
  std::vector<CmdTag> stream;

  uint32_t control = device_ops_api::TRACE_RT_CONTROL_ENABLE_TRACE;
  // Redirect logs to Trace or UART.
  control |= toTraceBuf ? device_ops_api::TRACE_RT_CONTROL_LOG_TO_TRACE : device_ops_api::TRACE_RT_CONTROL_LOG_TO_UART;
  // Reset Trace buffer.
  control |= resetTraceBuf ? device_ops_api::TRACE_RT_CONTROL_RESET_TRACEBUF : 0x0;

  // redirect MM and CM trace logging to TraceBuf/UART
  stream.push_back(IDevOpsApiCmd::createCmd<TraceRtControlCmd>(
    device_ops_api::CMD_FLAGS_BARRIER_DISABLE, device_ops_api::TRACE_RT_TYPE_MM | device_ops_api::TRACE_RT_TYPE_CM,
    control, device_ops_api::DEV_OPS_TRACE_RT_CONTROL_RESPONSE_SUCCESS));

  executeSyncPerDevicePerQueue(deviceIdx, 0 /* queueIdx */, stream);

  if (std::count_if(stream.begin(), stream.end(), [](auto tagId) {
        return IDevOpsApiCmd::getDevOpsApiCmd(tagId)->getCmdStatus() == CmdStatus::CMD_SUCCESSFUL;
      }) != stream.size()) {
    TEST_VLOG(0) << "Failed to redirect tracing of buffer, disabling trace dump!";
    FLAGS_enable_trace_dump = false;
  }
  stream.clear();
}

void TestDevOpsApi::loadElfToDevice(int deviceIdx, ELFIO::elfio& reader, const std::string& path,
                                    std::vector<CmdTag>& stream, uint64_t& kernelEntryAddr) {
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
  stream.push_back(IDevOpsApiCmd::createCmd<DataWriteCmd>(false, deviceElfSegment0Buffer, hostVirtAddr,
                                                          fileSize, device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
}

void TestDevOpsApi::insertStream(int deviceIdx, int queueIdx, std::vector<CmdTag> stream) {
  streams_.insert_or_assign(key(deviceIdx, queueIdx), std::move(stream));
}

void TestDevOpsApi::deleteStream(int deviceIdx, int queueIdx) {
  auto it = streams_.find(key(deviceIdx, queueIdx));
  if (it != streams_.end()) {
    streams_.erase(it);
  }
}

void TestDevOpsApi::deleteStreams() {
  streams_.clear();
  IDevOpsApiCmd::deleteDevOpsApiCmds();
}
