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
#include <condition_variable>
#include <elfio/elfio.hpp>
#include <gflags/gflags.h>
#include <gtest/gtest.h>
#include <hostUtils/logging/Logger.h>
#include <mutex>
#include <thread>
#include <unordered_map>

using Clock = std::chrono::system_clock;
using Timepoint = Clock::time_point;
using TimeDuration = Clock::duration;

DECLARE_uint32(exec_timeout);
DECLARE_string(kernels_dir);
DECLARE_string(trace_logfile);
DECLARE_bool(enable_trace_dump);
DECLARE_bool(loopback_driver);
DECLARE_bool(use_epoll);

namespace {
constexpr int32_t kEchoPayload = 0xDEADBEEF;
const uint8_t kDevFWMajor = DEVICE_OPS_API_MAJOR;
const uint8_t kDevFWMinor = DEVICE_OPS_API_MINOR;
const uint8_t kDevFWPatch = DEVICE_OPS_API_PATCH;
const uint64_t kCacheLineSize = 64;

/* TODO: All trace packet information should be in common files
         for both Host and Device usage. */
constexpr uint32_t CM_SIZE_PER_HART = 4096;
constexpr uint32_t WORKER_HART_COUNT = 2080;

extern "C" {
enum trace_type_e {
  TRACE_TYPE_STRING,
  TRACE_TYPE_PMC_COUNTER,
  TRACE_TYPE_PMC_ALL_COUNTERS,
  TRACE_TYPE_VALUE_U64,
  TRACE_TYPE_VALUE_U32,
  TRACE_TYPE_VALUE_U16,
  TRACE_TYPE_VALUE_U8,
  TRACE_TYPE_VALUE_FLOAT
};

struct trace_entry_header_t {
  uint64_t cycle;   // Current cycle
  uint32_t hart_id; // Hart ID of the Hart which is logging Trace
  uint16_t type;    // One of enum trace_type_e
  uint8_t pad[2];
} __attribute__((packed));

struct trace_string_t {
  struct trace_entry_header_t header;
  char dataString[64];
} __attribute__((packed));
}

} // namespace

enum class CmdStatus { CMD_RSP_NOT_RECEIVED, CMD_TIMED_OUT, CMD_FAILED, CMD_RSP_DUPLICATE, CMD_SUCCESSFUL };

enum cm_context_type {
  CM_CONTEXT_TYPE_HANG = 0,
  CM_CONTEXT_TYPE_UMODE_EXCEPTION,
  CM_CONTEXT_TYPE_SMODE_EXCEPTION,
  CM_CONTEXT_TYPE_SYSTEM_ABORT,
  CM_CONTEXT_TYPE_SELF_ABORT,
  CM_CONTEXT_TYPE_USER_KERNEL_ERROR
};

struct kernelRuntimeContext {
  std::string kernelName;
  uint64_t shireMask;
  bool cmdBarrier;
  bool flushL3;
  uint64_t startCycles;
  uint32_t waitDuration;
  uint32_t executionDuration;
};

struct __attribute__((packed, aligned(64))) hartExecutionContext {
  uint64_t type;
  uint64_t cycles;
  uint64_t hart_id;
  uint64_t sepc;
  uint64_t sstatus;
  uint64_t stval;
  uint64_t scause;
  int64_t user_error;
  uint64_t gpr[31];
};

class TestDevOpsApi : public ::testing::Test {
protected:
  void initTestHelperSysEmu(const emu::SysEmuOptions& options);
  void initTestHelperPcie();
  void executeAsync();
  void executeSync();
  uint64_t getDmaWriteAddr(int deviceIdx, size_t bufSize);
  uint64_t getDmaReadAddr(int deviceIdx, size_t bufSize);
  void resetMemPooltoDefault(int deviceIdx);
  void loadElfToDevice(int deviceIdx, ELFIO::elfio& reader, const std::string& path,
                       std::vector<std::unique_ptr<IDevOpsApiCmd>>& stream, uint64_t& kernelEntryAddr);

  void fExecutor(int deviceIdx, int queueIdx);
  void fListener(int deviceIdx);
  bool pushCmd(int deviceIdx, int queueIdx, std::unique_ptr<IDevOpsApiCmd>& devOpsApiCmd);
  bool popRsp(int deviceIdx);
  void printCmdExecutionSummary();
  void printErrorContext(int queueId, void* buffer, uint64_t shireMask, device_ops_api::tag_id_t tagId);
  void cleanUpExecution();
  void deleteCmdResults();
  void executeSyncPerDevicePerQueue(int deviceIdx, int queueIdx, std::vector<std::unique_ptr<IDevOpsApiCmd>>& stream);

  bool printMMTraceStringData(unsigned char* traceBuf, size_t size) const;
  bool printCMTraceStringData(unsigned char* traceBuf, size_t size) const;
  void extractAndPrintTraceData(int deviceIdx);

  inline int getDevicesCount() {
    return devLayer_->getDevicesCount();
  }

  inline int getSqCount(int deviceIdx) {
    return devLayer_->getSubmissionQueuesCount(deviceIdx);
  }

  inline void* allocDmaBuffer(int deviceIdx, size_t sizeInBytes, bool writeable) {
    return devLayer_->allocDmaBuffer(deviceIdx, sizeInBytes, writeable);
  }

  inline void freeDmaBuffer(void* dmaBuffer) {
    return devLayer_->freeDmaBuffer(dmaBuffer);
  }

  inline size_t key(int deviceIdx, int queueIdx) {
    return (size_t)deviceIdx << 32 | (size_t)queueIdx;
  }

  std::unordered_map<size_t, std::vector<std::unique_ptr<IDevOpsApiCmd>>> streams_;
  TimeDuration execTimeout_;

private:
  bool addkernelCmdContext(device_ops_api::tag_id_t tagId, std::string kernelName, uint64_t shireMask, bool barrier,
                           bool flushL3);
  bool addkernelRspContext(device_ops_api::tag_id_t tagId, uint64_t startTime, uint32_t waitDuration,
                           uint32_t execDuration);
  bool printKernelRtContext(device_ops_api::tag_id_t tagId, std::stringstream& logs);
  bool addCmdResultEntry(device_ops_api::tag_id_t tagId, CmdStatus status);
  CmdStatus getCmdResult(device_ops_api::tag_id_t tagId);
  bool updateCmdResult(device_ops_api::tag_id_t tagId, CmdStatus status);
  void deleteCmdResultEntry(device_ops_api::tag_id_t tagId);
  void redirectTraceLogging(int deviceIdx, bool toTraceBuf);

  struct DeviceInfo {
    uint64_t dmaWriteAddr_;
    std::mutex dmaWriteAddrMtx_;
    uint64_t dmaReadAddr_;
    std::mutex dmaReadAddrMtx_;
    uint64_t sqBitmap_;
    std::condition_variable sqBitmapCondVar_;
    std::mutex sqBitmapMtx_;
  };

  logging::LoggerDefault logger_;
  std::unique_ptr<dev::IDeviceLayer> devLayer_;
  std::vector<std::unique_ptr<DeviceInfo>> devices_;
  std::unordered_map<device_ops_api::tag_id_t, CmdStatus> cmdResults_;
  std::recursive_mutex cmdResultsMtx_;
  std::unordered_map<device_ops_api::tag_id_t, kernelRuntimeContext> kernelRtContext_;
  std::recursive_mutex kernelRtContextMtx_;

  Timepoint firstCmdTimepoint_;
  Timepoint lastCmdTimepoint_;
  Timepoint firstRspTimepoint_;
  Timepoint lastRspTimepoint_;

  size_t bytesSent_ = 0;
  size_t bytesReceived_ = 0;
};

#endif // TEST_DEV_OPS_API_H
