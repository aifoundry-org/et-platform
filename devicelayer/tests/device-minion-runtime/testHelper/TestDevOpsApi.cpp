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
namespace fs = std::experimental::filesystem;
using namespace std::chrono_literals;
auto kPollingInterval = 10ms;
} // namespace

DEFINE_uint32(exec_timeout, 100, "Internal execution timeout");
DEFINE_string(kernels_dir, "", "Directory where different kernel ELF files are located");
DEFINE_string(trace_logfile, "TestHelper.log", "File where the MM and CM buffers will be dumped");
DEFINE_bool(enable_trace_dump, true,
            "Enable device trace dump to file specified by flag: trace_logfile, otherwise on UART");
DEFINE_bool(loopback_driver, false, "Run on loopback driver");
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
      redirectTraceLogging(deviceIdx, true /* to trace buffer */);
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
    redirectTraceLogging(deviceIdx, false /* to UART */);
  }
}

void TestDevOpsApi::executeSyncPerDevicePerQueue(int deviceIdx, int queueIdx,
                                                 std::vector<std::unique_ptr<IDevOpsApiCmd>>& stream) {
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
  // save kernel command context
  if (devOpsApiCmd->whoAmI() == IDevOpsApiCmd::CmdType::KERNEL_LAUNCH_CMD) {
    KernelLaunchCmd* cmd = static_cast<KernelLaunchCmd*>(devOpsApiCmd.get());
    uint16_t flags = cmd->getCmdFlags();
    addkernelCmdContext(
      devOpsApiCmd->getCmdTagId(), cmd->getKernelName(), cmd->getShireMask(),
      ((flags & device_ops_api::CMD_FLAGS_BARRIER_ENABLE) == device_ops_api::CMD_FLAGS_BARRIER_ENABLE),
      ((flags & device_ops_api::CMD_FLAGS_KERNEL_LAUNCH_FLUSH_L3) == device_ops_api::CMD_FLAGS_KERNEL_LAUNCH_FLUSH_L3));
  }
  auto res = devLayer_->sendCommandMasterMinion(deviceIdx, queueIdx, devOpsApiCmd->getCmdPtr(),
                                                devOpsApiCmd->getCmdSize(), devOpsApiCmd->isDma());
  if (res) {
    bytesSent_ += devOpsApiCmd->getCmdSize();
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
  auto rsp_size = response_header->rsp_hdr.size;
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
    TEST_VLOG(1) << "      - Command Start time: " << response->device_cmd_start_ts;
    TEST_VLOG(1) << "      - Command Wait time: " << response->device_cmd_wait_dur;
    TEST_VLOG(1) << "      - Command Execution time: " << response->device_cmd_execute_dur;
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
    TEST_VLOG(1) << "      - Command Start time: " << response->device_cmd_start_ts;
    TEST_VLOG(1) << "      - Command Wait time: " << response->device_cmd_wait_dur;
    TEST_VLOG(1) << "      - Command Execution time: " << response->device_cmd_execute_dur;
    if (response->status == IDevOpsApiCmd::getExpectedRsp(rsp_tag_id)) {
      status = CmdStatus::CMD_SUCCESSFUL;
    } else if (response->status == (device_ops_api::DEV_OPS_API_DMA_RESPONSE_TIMEOUT_IDLE_CHANNEL_UNAVAILABLE ||
                                    device_ops_api::DEV_OPS_API_DMA_RESPONSE_TIMEOUT_HANG)) {
      status = CmdStatus::CMD_TIMED_OUT;
    } else {
      status = CmdStatus::CMD_FAILED;
    }

  } else if (rsp_msg_id == device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_DMA_READLIST_RSP) {
    auto response = reinterpret_cast<device_ops_api::device_ops_dma_readlist_rsp_t*>(message.data());
    TEST_VLOG(1) << "=====> DMA readlist command response received (tag_id: " << std::hex << rsp_tag_id << ") <====";
    TEST_VLOG(1) << "     => Total measured latencies (in cycles) ";
    TEST_VLOG(1) << "      - Command Start time: " << response->device_cmd_start_ts;
    TEST_VLOG(1) << "      - Command Wait time: " << response->device_cmd_wait_dur;
    TEST_VLOG(1) << "      - Command Execution time: " << response->device_cmd_execute_dur;
    if (response->status == IDevOpsApiCmd::getExpectedRsp(rsp_tag_id)) {
      status = CmdStatus::CMD_SUCCESSFUL;
    } else if (response->status == (device_ops_api::DEV_OPS_API_DMA_RESPONSE_TIMEOUT_IDLE_CHANNEL_UNAVAILABLE ||
                                    device_ops_api::DEV_OPS_API_DMA_RESPONSE_TIMEOUT_HANG)) {
      status = CmdStatus::CMD_TIMED_OUT;
    } else {
      status = CmdStatus::CMD_FAILED;
    }

  } else if (rsp_msg_id == device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_DMA_WRITELIST_RSP) {
    auto response = reinterpret_cast<device_ops_api::device_ops_dma_writelist_rsp_t*>(message.data());
    TEST_VLOG(1) << "=====> DMA writelist command response received (tag_id: " << std::hex << rsp_tag_id << ") <====";
    TEST_VLOG(1) << "     => Total measured latencies (in cycles)";
    TEST_VLOG(1) << "      - Command Start time: " << response->device_cmd_start_ts;
    TEST_VLOG(1) << "      - Command Wait time: " << response->device_cmd_wait_dur;
    TEST_VLOG(1) << "      - Command Execution time: " << response->device_cmd_execute_dur;
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
    TEST_VLOG(1) << "      - Command Start time: " << response->device_cmd_start_ts;
    TEST_VLOG(1) << "      - Command Wait time: " << response->device_cmd_wait_dur;
    TEST_VLOG(1) << "      - Command Execution time: " << response->device_cmd_execute_dur;
    if (response->status == IDevOpsApiCmd::getExpectedRsp(rsp_tag_id)) {
      status = CmdStatus::CMD_SUCCESSFUL;
    } else if (response->status == device_ops_api::DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_TIMEOUT_HANG) {
      status = CmdStatus::CMD_TIMED_OUT;
    } else {
      status = CmdStatus::CMD_FAILED;
    }
    addkernelRspContext(rsp_tag_id, response->device_cmd_start_ts, response->device_cmd_wait_dur,
                        response->device_cmd_execute_dur);

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

  bytesReceived_ += rsp_size;

  return res;
}

bool TestDevOpsApi::addkernelCmdContext(device_ops_api::tag_id_t tagId, std::string kernelName, uint64_t shireMask,
                                        bool barrier, bool flushL3) {
  std::unique_lock<std::recursive_mutex> lock(kernelRtContextMtx_);
  auto it = kernelRtContext_.find(tagId);
  if (it != kernelRtContext_.end()) {
    return false;
  }
  kernelRuntimeContext context;
  context.kernelName = kernelName;
  context.shireMask = shireMask;
  context.cmdBarrier = barrier;
  context.flushL3 = flushL3;
  kernelRtContext_.emplace(tagId, context);
  return true;
}

bool TestDevOpsApi::addkernelRspContext(device_ops_api::tag_id_t tagId, uint64_t startTime, uint32_t waitDuration,
                                        uint32_t execDuration) {
  std::unique_lock<std::recursive_mutex> lock(kernelRtContextMtx_);
  auto it = kernelRtContext_.find(tagId);
  if (it == kernelRtContext_.end()) {
    return false;
  }
  it->second.startCycles = startTime;
  it->second.waitDuration = waitDuration;
  it->second.executionDuration = execDuration;
  return true;
}

bool TestDevOpsApi::printKernelRtContext(device_ops_api::tag_id_t tagId, std::stringstream& logs) {
  std::unique_lock<std::recursive_mutex> lock(kernelRtContextMtx_);
  auto it = kernelRtContext_.find(tagId);
  if (it == kernelRtContext_.end()) {
    return false;
  }
  logs << "* Tag ID: 0x" << std::hex << tagId << std::endl;
  logs << "* Kernel Name: " << it->second.kernelName << std::endl;
  logs << "* Shire Mask: 0x" << std::hex << it->second.shireMask << std::endl;
  logs << "* Barrier: " << it->second.cmdBarrier << std::endl;
  logs << "* Flush L3: " << it->second.flushL3 << std::endl;
  logs << "* Kernel Start cycles: " << it->second.startCycles << std::endl;
  logs << "* Kernel Wait duration: " << it->second.waitDuration << std::endl;
  logs << "* Kernel Execution duration: " << it->second.executionDuration << std::endl;
  return true;
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
  TEST_VLOG(0) << "  ====> Commands Sent / second: " << std::ceil(cmdResults_.size() / cmdsTotalDuration.count());
  TEST_VLOG(0) << "  ====> Bytes Sent / second: " << std::setprecision(10)
               << std::ceil(bytesSent_ / cmdsTotalDuration.count());
  TEST_VLOG(0) << "  ====> Responses Received / second: " << std::ceil(cmdResults_.size() / rspsTotalDuration.count());
  TEST_VLOG(0) << "  ====> Bytes Received / second: " << std::setprecision(10)
               << std::ceil(bytesReceived_ / rspsTotalDuration.count());
  TEST_VLOG(0) << "================================================";

  EXPECT_EQ(failedCmdTags.size(), 0);
  EXPECT_EQ(missingRspCmdTags.size(), 0);
  EXPECT_EQ(duplicateRspCmdTags.size(), 0);
  EXPECT_EQ(timeoutCmdTags.size(), 0);
}

void TestDevOpsApi::printErrorContext(int queueId, void* buffer, uint64_t shireMask, device_ops_api::tag_id_t tagId) {
  std::stringstream logs;
  std::ofstream logfile;
  if (FLAGS_enable_trace_dump) {
    logfile.open(FLAGS_trace_logfile, std::ios_base::app);
  }

  hartExecutionContext* context = reinterpret_cast<hartExecutionContext*>(buffer);
  logs << "\n*** Error Context Start (Queue ID: " << queueId << ")***" << std::endl;
  logs << "  * RUNTIME CONTEXT *" << std::endl;
  printKernelRtContext(tagId, logs);
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

void TestDevOpsApi::deleteCmdResults() {
  std::unique_lock<std::recursive_mutex> lock(cmdResultsMtx_);
  cmdResults_.clear();
}

bool TestDevOpsApi::printMMTraceStringData(unsigned char* traceBuf, size_t size) const {
  std::stringstream logs;
  std::ofstream logfile;
  if (FLAGS_enable_trace_dump) {
    logfile.open(FLAGS_trace_logfile, std::ios_base::app);
  }

  auto dataPtr = reinterpret_cast<trace_string_t*>(traceBuf);
  auto start = dataPtr;
  bool validStringEventFound = false;
  while ((dataPtr - start) < size) {
    if ((dataPtr->header.type == TRACE_TYPE_STRING) && (dataPtr->dataString[0] != '\0')) {
      logs << "H:" << dataPtr->header.hart_id << ":" << dataPtr->dataString;
      dataPtr++;
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

bool TestDevOpsApi::printCMTraceStringData(unsigned char* traceBuf, size_t size) const {
  std::stringstream logs;
  std::ofstream logfile;
  if (FLAGS_enable_trace_dump) {
    logfile.open(FLAGS_trace_logfile, std::ios_base::app);
  }

  auto hartDataPtr = traceBuf;
  bool validStringEventFound = false;
  for (int i = 0; i < WORKER_HART_COUNT; ++i) {
    auto dataPtr = reinterpret_cast<trace_string_t*>(hartDataPtr);
    auto start = dataPtr;
    while ((dataPtr - start) < size) {
      if ((dataPtr->header.type == TRACE_TYPE_STRING) && (dataPtr->dataString[0] != '\0')) {
        logs << "H:" << dataPtr->header.hart_id << ":" << dataPtr->dataString;
        dataPtr++;
        validStringEventFound = true;
      } else {
        break;
      }
    }
    hartDataPtr += CM_SIZE_PER_HART;
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
  const int kBufCount = 2;
  const std::array<uint32_t, kBufCount> bufSizes = {1024 * 1024, CM_SIZE_PER_HART * WORKER_HART_COUNT};
  auto totalSize = std::accumulate(bufSizes.begin(), bufSizes.end(), 0);

  auto rdBufMem = allocDmaBuffer(deviceIdx, totalSize, false /* read buffer */);

  std::array<device_ops_api::dma_read_node, kBufCount> rdList;
  auto rdBufPtr = static_cast<uint8_t*>(rdBufMem);
  for (int nodeIdx = 0; nodeIdx < kBufCount; nodeIdx++) {
    rdList[nodeIdx] = {.dst_host_virt_addr = reinterpret_cast<uint64_t>(rdBufPtr),
                       .dst_host_phy_addr = reinterpret_cast<uint64_t>(rdBufPtr),
                       .src_device_phy_addr = 0,
                       .size = bufSizes[nodeIdx]};
    rdBufPtr += bufSizes[nodeIdx];
  }
  std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;
  stream.push_back(IDevOpsApiCmd::createDmaReadListCmd(device_ops_api::CMD_FLAGS_BARRIER_DISABLE, rdList.data(),
                                                       kBufCount, device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
  executeSyncPerDevicePerQueue(deviceIdx, 0 /* queueIdx */, stream);

  printMMTraceStringData(static_cast<unsigned char*>(rdBufMem), bufSizes[0]);
  printCMTraceStringData(static_cast<unsigned char*>(rdBufMem) + bufSizes[0], bufSizes[1]);

  freeDmaBuffer(rdBufMem);

  cmdResults_.clear();
}

void TestDevOpsApi::redirectTraceLogging(int deviceIdx, bool toTraceBuf) {
  std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;

  // redirect MM trace logging to TraceBuf/UART
  stream.push_back(IDevOpsApiCmd::createTraceRtControlCmd(device_ops_api::CMD_FLAGS_BARRIER_DISABLE, 0x1,
                                                          toTraceBuf ? 0x3 : 0x0,
                                                          device_ops_api::DEV_OPS_TRACE_RT_CONTROL_RESPONSE_SUCCESS));
  // redirect CM trace logging to TraceBuf/UART
  stream.push_back(IDevOpsApiCmd::createTraceRtControlCmd(device_ops_api::CMD_FLAGS_BARRIER_DISABLE, 0x2,
                                                          toTraceBuf ? 0x3 : 0x0,
                                                          device_ops_api::DEV_OPS_TRACE_RT_CONTROL_RESPONSE_SUCCESS));

  executeSyncPerDevicePerQueue(deviceIdx, 0 /* queueIdx */, stream);

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
