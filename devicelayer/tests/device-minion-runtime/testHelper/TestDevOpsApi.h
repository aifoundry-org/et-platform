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

DECLARE_uint32(exec_timeout);
DECLARE_string(kernels_dir);
DECLARE_string(trace_logfile);
DECLARE_bool(enable_trace_dump);
DECLARE_bool(loopback_driver);
DECLARE_bool(use_epoll);

namespace dev::dl_tests {

using Clock = std::chrono::system_clock;
using Timepoint = Clock::time_point;
using TimeDuration = Clock::duration;

const uint8_t kDevFWMajor = DEVICE_OPS_API_MAJOR;
const uint8_t kDevFWMinor = DEVICE_OPS_API_MINOR;
const uint8_t kDevFWPatch = DEVICE_OPS_API_PATCH;
// TODO: SW-8297: The FW versions are coming put to be zero
#ifdef TARGET_PCIE
const uint16_t kMachineFWVersionMajor = 0;
const uint16_t kMachineFWVersionMinor = 0;
const uint16_t kMachineFWVersionPatch = 0;
#else
const uint16_t kMachineFWVersionMajor = 0;
const uint16_t kMachineFWVersionMinor = 0;
const uint16_t kMachineFWVersionPatch = 0;
#endif
const uint64_t kCacheLineSize = 64;

/* TODO: All trace packet information should be in common files
         for both Host and Device usage. */
constexpr uint32_t CM_SIZE_PER_HART = 4096;
constexpr uint32_t WORKER_HART_COUNT = 2080;
constexpr uint32_t MM_BASE_ID = 2048;
constexpr uint32_t TRACE_STRING_MAX_SIZE = 64;
constexpr uint32_t TRACE_MAGIC_HEADER = 0x76543210;
constexpr uint64_t MM_SHIRE_MASK = (0x1ULL << 32);
constexpr uint64_t CM_SHIRE_MASK = 0x1;
constexpr uint64_t HART_ID = 0x1;
constexpr uint32_t TRACE_STRING_FILTER = 0x1;
constexpr uint32_t TRACE_STRING_LOG_INFO = 0x3;

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

enum trace_buffer_type_e { TRACE_MM_BUFFER, TRACE_CM_BUFFER, TRACE_SP_BUFFER };

struct trace_buffer_size_header_t {
  uint32_t data_size;
} __attribute__((packed));

struct trace_buffer_std_header_t {
  uint32_t magic_header;
  uint32_t data_size;
  uint16_t type;
  uint8_t pad[6];
} __attribute__((packed));

struct trace_entry_header_t {
  uint64_t cycle; // Current cycle
  uint16_t type;  // One of enum trace_type_e
} __attribute__((packed));

struct trace_entry_header_mm_t {
  uint64_t cycle;   // Current cycle
  uint32_t hart_id; // Hart ID of the Hart which is logging Trace
  uint16_t type;    // One of enum trace_type_e
  uint8_t pad[2];
} __attribute__((packed));

struct trace_string_t {
  struct trace_entry_header_t header;
  char dataString[64];
} __attribute__((packed));

struct trace_string_mm_t {
  struct trace_entry_header_mm_t mm_header;
  char dataString[64];
} __attribute__((packed));
}

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
  struct PopRspResult {
    CmdTag tagId_;
    explicit operator bool() const {
      return tagId_ > 0;
    }
  };

  void initTestHelperSysEmu(const emu::SysEmuOptions& options);
  void initTestHelperPcie();
  void execute(bool isAsync);
  uint64_t getDmaWriteAddr(int deviceIdx, size_t bufSize);
  uint64_t getDmaReadAddr(int deviceIdx, size_t bufSize);
  void resetMemPooltoDefault(int deviceIdx);
  void loadElfToDevice(int deviceIdx, ELFIO::elfio& reader, const std::string& path, std::vector<CmdTag>& stream,
                       uint64_t& kernelEntryAddr);

  bool pushCmd(int deviceIdx, int queueIdx, CmdTag tagId);
  PopRspResult popRsp(int deviceIdx);
  void printErrorContext(int queueId, const std::byte* buffer, uint64_t shireMask, CmdTag tagId) const;

  void controlTraceLogging(int deviceIdx, bool toTraceBuf, bool resetTraceBuf);
  bool printMMTraceStringData(unsigned char* traceBuf, size_t bufSize) const;
  bool printCMTraceStringData(unsigned char* traceBuf, size_t bufSize) const;
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

  void insertStream(int deviceIdx, int queueIdx, std::vector<CmdTag> cmds, unsigned int retryCount = 0);
  void deleteStreams();

  TimeDuration execTimeout_;

private:
  struct DeviceInfo {
    uint64_t dmaWriteAddr_;
    std::mutex dmaWriteAddrMtx_;
    uint64_t dmaReadAddr_;
    std::mutex dmaReadAddrMtx_;
    uint64_t sqBitmap_;
    std::condition_variable asyncEpollCondVar_;
    std::mutex asyncEpollMtx_;
    std::atomic<bool> abort_ = false;
  };

  struct Stream {
    Stream(int deviceIdx, int queueIdx, std::vector<CmdTag> cmds, unsigned int retryCount = 0)
      : deviceIdx_(deviceIdx)
      , queueIdx_(queueIdx)
      , retryCount_(retryCount) {
      if (cmds.empty()) {
        throw Exception("Stream must atleast contain one command!");
      }
      cmds_ = std::move(cmds);
    }
    std::shared_ptr<Stream> getReTransmissionStream() {
      if (retryCount_ == 0 || reTransmitted_) {
        return nullptr;
      }
      auto newStream = std::make_shared<Stream>(*this);
      newStream->retryCount_ = retryCount_ - 1;
      newStream->cmds_.clear();
      // Clone stream commands with new tagIds
      for (const auto& cmd : cmds_) {
        newStream->cmds_.push_back(IDevOpsApiCmd::cloneDevOpsApiCmd(cmd));
      }
      if (newStream->cmds_.empty()) {
        return nullptr;
      }
      retryCount_ = 0;
      reTransmitted_ = true;
      return newStream;
    }
    ~Stream() = default;
    const int deviceIdx_;
    const int queueIdx_;
    bool reTransmitted_ = false;
    unsigned int retryCount_;
    std::vector<CmdTag> cmds_;
  };

  void waitForSqAvailability(int deviceIdx, int queueIdx);
  void dispatchStreamAsync(const std::shared_ptr<Stream>& stream);
  void fExecutor(int deviceIdx, int queueIdx);
  void fListener(int deviceIdx);
  void executeAsync();

  void waitForCqAvailability(int deviceIdx, TimeDuration timeout);
  void dispatchStreamSync(const std::shared_ptr<Stream>& stream, TimeDuration timeout);
  void executeSyncPerDevice(int deviceIdx);
  void executeSync();

  size_t handleStreamReTransmission(CmdTag tagId);
  void printCmdExecutionSummary();
  void cleanUpExecution();

  logging::LoggerDefault logger_;
  std::vector<CmdTag> invalidRsps_;
  std::unique_ptr<dev::IDeviceLayer> devLayer_;
  std::vector<std::shared_ptr<Stream>> streams_;
  std::vector<std::unique_ptr<DeviceInfo>> devices_;

  Timepoint firstCmdTimepoint_;
  Timepoint lastCmdTimepoint_;
  Timepoint firstRspTimepoint_;
  Timepoint lastRspTimepoint_;

  size_t bytesSent_ = 0;
  size_t bytesReceived_ = 0;
};

} // namespace dev::dl_tests

#endif // TEST_DEV_OPS_API_H
