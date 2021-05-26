//******************************************************************************
// Copyright (C) 2021 Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#ifndef TEST_DEV_OPS_API_H
#define TEST_DEV_OPS_API_H

#include "IDevOpsApiCmd.h"
#include "deviceLayer/IDeviceLayer.h"
#include <atomic>
#include <hostUtils/logging/Logger.h>
#include <condition_variable>
#include <elfio/elfio.hpp>
#include <gflags/gflags.h>
#include <gtest/gtest.h>
#include <mutex>
#include <thread>
#include <unordered_map>

using Clock = std::chrono::system_clock;
using Timepoint = Clock::time_point;
using TimeDuration = Clock::duration;

DECLARE_string(kernels_dir);
DECLARE_uint32(exec_timeout);
DECLARE_bool(loopback_driver);

namespace {
constexpr int kIDevice = 0;
constexpr int32_t kEchoPayload = 0xDEADBEEF;
const uint8_t kDevFWMajor = DEVICE_OPS_API_MAJOR;
const uint8_t kDevFWMinor = DEVICE_OPS_API_MINOR;
const uint8_t kDevFWPatch = DEVICE_OPS_API_PATCH;
const uint64_t kCacheLineSize = 64;
} // namespace

enum class CmdStatus { CMD_RSP_NOT_RECEIVED, CMD_TIMED_OUT, CMD_FAILED, CMD_RSP_DUPLICATE, CMD_SUCCESSFUL };

struct __attribute__((packed, aligned(64))) hart_execution_context {
  uint64_t kernel_pending_shires;
  uint64_t cycles;
  uint64_t hart_id;
  uint64_t sepc;
  uint64_t sstatus;
  uint64_t stval;
  uint64_t scause;
  uint64_t gpr[29];
};

class TestDevOpsApi : public ::testing::Test {
protected:
  void fExecutor(uint8_t queueId);
  void fListener();
  void executeAsync();
  void executeSync();

  void resetMemPooltoDefault();
  uint64_t getDmaWriteAddr(size_t bufSize);
  uint64_t getDmaReadAddr(size_t bufSize);

  void initTagId(device_ops_api::tag_id_t value);
  device_ops_api::tag_id_t getNextTagId();

  void loadElfToDevice(ELFIO::elfio& reader, const std::string& path,
                       std::vector<std::unique_ptr<IDevOpsApiCmd>>& stream, uint64_t& kernelEntryAddr);

  bool pushCmd(uint16_t queueId, std::unique_ptr<IDevOpsApiCmd>& devOpsApiCmd);
  bool popRsp(void);

  bool addCmdResultEntry(device_ops_api::tag_id_t tagId, CmdStatus status);
  CmdStatus getCmdResult(device_ops_api::tag_id_t tagId);
  bool updateCmdResult(device_ops_api::tag_id_t tagId, CmdStatus status);
  void deleteCmdResultEntry(device_ops_api::tag_id_t tagId);
  void deleteCmdResults();
  void printCmdExecutionSummary();
  void printErrorContext(void *buffer, uint64_t shireMask);
  void cleanUpExecution();

  logging::LoggerDefault logger_;
  std::unique_ptr<dev::IDeviceLayer> devLayer_;
  std::atomic<device_ops_api::tag_id_t> tagId_;

  uint64_t dmaWriteAddr_;
  std::mutex dmaWriteAddrMtx_;
  uint64_t dmaReadAddr_;
  std::mutex dmaReadAddrMtx_;

  uint64_t sqBitmap_ = 0xffffffffffffffff;
  std::condition_variable sqBitmapCondVar_;
  std::mutex sqBitmapMtx_;

  std::unordered_map<int, std::vector<std::unique_ptr<IDevOpsApiCmd>>> streams_;
  TimeDuration execTimeout_;

  std::unordered_map<device_ops_api::tag_id_t, CmdStatus> cmdResults_;
  std::recursive_mutex cmdResultsMtx_;

  Timepoint firstCmdTimepoint_;
  Timepoint lastCmdTimepoint_;
  Timepoint firstRspTimepoint_;
  Timepoint lastRspTimepoint_;
};

#endif // TEST_DEV_OPS_API_H
