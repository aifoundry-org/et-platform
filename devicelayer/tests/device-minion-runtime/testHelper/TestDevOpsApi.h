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
#include <esperanto/et-trace/layout.h>
#include <esperanto/et-trace/decoder.h>
#include <mutex>
#include <thread>
#include <unordered_map>

DECLARE_uint32(exec_timeout);
DECLARE_string(kernels_dir);
DECLARE_string(trace_logfile);
DECLARE_bool(enable_trace_dump);
DECLARE_bool(loopback_driver);
DECLARE_bool(use_epoll);

#define CATCH_EXCEPTION                               \
      catch (const Exception &ex) {                   \
          TEST_VLOG(0) << "Exception: " << ex.what(); \
          std::exit(EXIT_FAILURE);                    \
      }

namespace dev::dl_tests {

/*! \def ALIGN(x, a)
    \brief Alignemnt checking macro
*/
#define ALIGN(x, a) (((x) + ((a) - 1)) & ~((a) - 1))

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
constexpr uint64_t MM_SHIRE_MASK = (0x1ULL << 32);
constexpr uint64_t HART_ID = 0x1;
constexpr uint32_t TRACE_STRING_FILTER = 0x1;
constexpr uint32_t TRACE_STRING_LOG_INFO = 0x3;

extern "C" {

/*! \def GET_SHIRE_MASK
 *  \brief Get the Shire mask form Hart ID using Harts per Shire information.
 */
#define GET_SHIRE_MASK(hart_id)     (1UL << ((hart_id) / 64U))

/*! \def GET_HART_MASK
 * \brief Get Hart mask form Hart ID using Harts per Shire information.
 */
#define GET_HART_MASK(hart_id)      (1UL << ((hart_id) % 64U))


/*! \def CHECK_CM_HART_TRACE_ENABLED
 * \brief Check if CM Hart tracing is enabled for given hart and mask.
 */
#define CHECK_CM_HART_TRACE_ENABLED(hart_id, shire_mask, thread_mask)  \
              ((shire_mask & GET_SHIRE_MASK(hart_id)) &&               \
               (thread_mask & GET_HART_MASK(hart_id)))

struct trace_entry_header_mm_t {
  uint64_t cycle;   // Current cycle
  uint32_t hart_id; // Hart ID of the Hart which is logging Trace
  uint16_t type;    // One of enum trace_type_e
  uint8_t pad[2];
} __attribute__((packed));

struct cm_trace_string_t {
  struct trace_entry_header_t header;
  char dataString[64];
} __attribute__((packed));

struct mm_trace_string_t {
  struct trace_entry_header_mm_t mm_header;
  char dataString[64];
} __attribute__((packed));

struct mm_trace_cmd_status_t {
  struct trace_entry_header_mm_t mm_header;
  struct trace_event_cmd_status_t cmd;
} __attribute__((packed));

struct cm_trace_cmd_status_t {
  struct trace_entry_header_t header;
  struct trace_event_cmd_status_t cmd;
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

  void controlTraceLogging(int deviceIdx, bool toTraceBuf, bool resetTraceBuf,
                           TimeDuration timeout = std::chrono::seconds(10));
  bool printMMTraceData(unsigned char* traceBuf, size_t bufSize) const;
  bool printCMTraceData(unsigned char* traceBuf, size_t bufSize, uint64_t shire_mask, uint64_t hart_mask) const;
  bool printCMTraceSingleHartData(unsigned char* hartDataPtr, uint32_t cmHartID, size_t dataSize) const;
  void extractAndPrintTraceData(int deviceIdx, TimeDuration timeout = std::chrono::seconds(10));

  inline int getDevicesCount() {
    return devLayer_->getDevicesCount();
  }

  inline int getSqCount(int deviceIdx) {
    try {
      return devLayer_->getSubmissionQueuesCount(deviceIdx);
    } CATCH_EXCEPTION
  }

  inline void* allocDmaBuffer(int deviceIdx, size_t sizeInBytes, bool writeable) {
    try {
      return devLayer_->allocDmaBuffer(deviceIdx, sizeInBytes, writeable);
    } CATCH_EXCEPTION
  }

  inline void freeDmaBuffer(void* dmaBuffer) {
    try {
      return devLayer_->freeDmaBuffer(dmaBuffer);
    } CATCH_EXCEPTION
  }

  inline size_t key(int deviceIdx, int queueIdx) {
    return (size_t)deviceIdx << 32 | (size_t)queueIdx;
  }

  void insertStream(int deviceIdx, int queueIdx, std::vector<CmdTag> cmds, unsigned int retryCount = 0);
  void deleteStreams();

  TimeDuration execTimeout_;

  inline void skipIfMultiDevMultiSqNotAvailable(bool singleDevice, bool singleQueue) {
  if (!singleDevice && getDevicesCount() == 1) {
    TEST_VLOG(0) << "Skipping Test: " << ::testing::UnitTest::GetInstance()->current_test_info()->test_case_name()
                 << "." << ::testing::UnitTest::GetInstance()->current_test_info()->name()
                 << " because multiple devices are not available.";
    std::exit(EXIT_SUCCESS);
  }
  if (!singleQueue && getSqCount(0) == 1) {
    TEST_VLOG(0) << "Skipping Test: " << ::testing::UnitTest::GetInstance()->current_test_info()->test_case_name()
                 << "." << ::testing::UnitTest::GetInstance()->current_test_info()->name()
                 << " because multiple SQs are not available.";
    std::exit(EXIT_SUCCESS);
  }
}

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

  inline bool readLastTestStatus() const {
    bool devicesAlive = true;

    // Read last test status
    std::ifstream lastTestStatus;
    lastTestStatus.open("lastTestStatus.txt");
    std::string line;
    while (std::getline(lastTestStatus, line)) {
      if (line.find("DEVICES_NOT_RESPONDING") != std::string::npos) {
        devicesAlive = false;
        break;
      }
    }
    lastTestStatus.close();
    return devicesAlive;
  }

  inline void writeCurrentTestStatus(bool devicesRunning) const {
    std::ofstream lastTestStatus;
    lastTestStatus.open("lastTestStatus.txt");
    devicesRunning ? lastTestStatus << "DEVICES_RUNNING" : lastTestStatus << "DEVICES_NOT_RESPONDING";
    lastTestStatus.close();
  }

  void waitForSqAvailability(int deviceIdx, int queueIdx);
  void waitForCqAvailability(int deviceIdx, TimeDuration timeout);
  void dispatchStreamAsync(const std::shared_ptr<Stream>& stream);
  void fExecutor(int deviceIdx, int queueIdx);
  void fListener(int deviceIdx);
  void executeAsync();

  void dispatchStreamSync(const std::shared_ptr<Stream>& stream, TimeDuration timeout);
  void executeSyncPerDevice(int deviceIdx);
  void executeSync();

  bool isDeviceAlive(int deviceIdx, TimeDuration timeout);
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
