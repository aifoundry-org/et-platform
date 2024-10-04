/*-------------------------------------------------------------------------
 * Copyright (C) 2021, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/
#pragma once
#include <cstddef>
#include <functional>
#include <hostUtils/debug/StackException.h>
#include <memory>
#include <optional>
#include <vector>

/// \defgroup runtime_types Runtime Types
/// These are custom types declared by Runtime

namespace rt {
/// \brief Forward declaration of \ref DmaBuffer
class IDmaBuffer;

/// \brief Forward declaration of \ref IProfiler
class IProfiler;

/// \brief Forward declaration of \ref IProfilerRecorder
namespace profiling {
class IProfilerRecorder;
}

/// \brief Event Handler
enum class EventId : uint16_t {};

/// \brief Stream Handler
enum class StreamId : int {};

/// \brief Device Handler
enum class DeviceId : int {};

/// \brief KernelId Handler
enum class KernelId : int {};

/// \brief This struct will hold parametrization options for Runtime instantiation
struct Options {
  bool checkMemcpyDeviceOperations_; /// < if set, the runtime will inspect all memcpy operations and throw an
                                     /// exception if invalid device address/size
  bool checkDeviceApiVersion_;
};

/// \brief Returns the default options. See \ref Options
constexpr auto getDefaultOptions() {
  return Options{true, true};
}

/// \brief RuntimePtr is an alias for a pointer to a Runtime instantation
using RuntimePtr = std::unique_ptr<class IRuntime>;

/// \brief The error handling in the runtime is trough exceptions. This is the
/// only exception kind that the runtime can throw
class Exception : public dbg::StackException {
  using dbg::StackException::StackException;
};
/// \brief This struct will hold a number of memcpy operations.
struct MemcpyList {

  void addOp(std::byte* src, std::byte* dst, size_t size) {
    operations_.emplace_back(Op{src, dst, size});
  }

  struct Op {
    std::byte* src_;
    std::byte* dst_;
    size_t size_;
  };
  std::vector<Op> operations_;
};

/// \brief This is the device kernel error context which is a result of a running kernel terminating on a abnormal state
/// (exception / abort)
struct __attribute__((aligned(64))) ErrorContext {
  uint64_t type_;  ///< 0: compute hang, 1: U-mode exception, 2: system abort, 3: self abort, 4: kernel execution error,
                   ///< 5: kernel execution tensor error
  uint64_t cycle_; ///< The cycle as sampled from the system counters at the point where the event type has happened.
                   ///< Its accurate to within 100 cycles.
  uint64_t hartId_;              ///< The Hart thread ID which took the error event.
  uint64_t mepc_;                ///< Device exception program counter
  uint64_t mstatus_;             ///< Device status register
  uint64_t mtval_;               ///< Device bad address or instruction
  uint64_t mcause_;              ///< Device trap cause
  int64_t userDefinedError_;     ///< User defined error (Depending on the Type)
  std::array<uint64_t, 31> gpr_; ///< RiscV ABI, corresponds to x1-x31 registers
};

enum class DeviceErrorCode {
  KernelLaunchUnexpectedError,
  KernelLaunchException,
  KernelLaunchShiresNotReady,
  KernelLaunchHostAborted,
  KernelLaunchInvalidAddress,
  KernelLaunchTimeoutHang,
  KernelLaunchInvalidArgsPayloadSize,
  KernelLaunchCmIfaceMulticastFailed,
  KernelLaunchCmIfaceUnicastFailed,
  KernelLaunchSpIfaceResetFailed,
  KernelLaunchCwMinionsBootFailed,
  KernelLaunchInvalidArgsInvalidShireMask,
  KernelLaunchResponseUserError,

  KernelAbortError,
  KernelAbortInvalidTagId,
  KernelAbortTimeoutHang,
  KernelAbortHostAborted,

  AbortUnexpectedError,
  AbortInvalidTagId,

  DmaUnexpectedError,
  DmaHostAborted,
  DmaErrorAborted,
  DmaInvalidAddress,
  DmaInvalidSize,
  DmaCmIfaceMulticastFailed,
  DmaDriverDataConfigFailed,
  DmaDriverLinkConfigFailed,
  DmaDriverChanStartFailed,
  DmaDriverAbortFailed,

  TraceConfigUnexpectedError,
  TraceConfigBadShireMask,
  TraceConfigBadThreadMask,
  TraceConfigBadEventMask,
  TraceConfigBadFilterMask,
  TraceConfigHostAborted,
  TraceConfigCmFailed,
  TraceConfigMmFailed,
  TraceConfigInvalidConfig,

  TraceControlUnexpectedError,
  TraceControlBadRtType,
  TraceControlBadControlMask,
  TraceControlComputeMinionRtCtrlError,
  TraceControlMasterMinionRtCtrlError,
  TraceControlHostAborted,
  TraceControlCmIfaceMulticastFailed,

  ApiCompatibilityUnexpectedError,
  ApiCompatibilityIncompatibleMajor,
  ApiCompatibilityIncompatibleMinor,
  ApiCompatibilityIncompatiblePatch,
  ApiCompatibilityBadFirmwareType,
  ApiCompatibilityHostAborted,

  FirmwareVersionUnexpectedError,
  FirmwareVersionBadFwType,
  FirmwareVersionNotAvailable,
  FirmwareVersionHostAborted,

  EchoHostAborted,

  CmResetUnexpectedError,
  CmResetInvalidShireMask,
  CmResetFailed,

  ErrorTypeUnsupportedCommand,
  ErrorTypeCmSmodeRtException,
  ErrorTypeCmSmodeRtHang,

  Unknown
};

/// \brief This struct contains the errorCode given by de device when some command fail and the associated
/// \ref rt::ErrorContext (if any)
struct StreamError {
  explicit StreamError(DeviceErrorCode errorCode, DeviceId device = DeviceId{-1})
    : errorCode_(errorCode)
    , device_(device) {
  }
  StreamError() = default;
  std::string getString() const; /// < returns a string representation of the StreamError
  DeviceErrorCode errorCode_ = DeviceErrorCode::Unknown;
  DeviceId device_;                     /// < device where the error originated
  std::optional<StreamId> stream_;      /// < indicates the stream where the error happened.
  std::optional<uint64_t> cmShireMask_; /// < only available in some kernel errors. Contains offending shiremask
  std::optional<std::vector<ErrorContext>> errorContext_;
};
/// \brief This callback can be optionally set to automatically retrieve stream errors when produced
using StreamErrorCallback = std::function<void(EventId, const StreamError&)>;
/// \brief Constants
constexpr auto kCacheLineSize = 64U; // TODO This should not be here, it should be
                                     // in a header with project-wide constants

/// \brief This callback will be called when a kernel is aborted. Be sure to call freeResources() once you are done with
/// the context, in order to release the resources.
/// event will contain the eventId which is associated to the aborted kernel
/// context will contain a pointer to device memory and the size which will be sizeof(ErrorContext) * num of contexts
/// freeResources must be called inside the callback code in order to release the resources, after you are done with
/// context.
/// This callback will be executed in a DETACHED thread,so be sure the execution of this task won't survive runtime
/// instance.
using KernelAbortedCallback =
  std::function<void(EventId event, std::byte* context, size_t size, std::function<void()> freeResources)>;

/// \brief This struct will contain the results of calling a loadCode function
struct LoadCodeResult {
  EventId event_;          /// < event to wait for the loadCode is complete
  KernelId kernel_;        /// < kernelId associated to the loadCode, to be used in later kernelLaunch(es)
  std::byte* loadAddress_; /// < this is the device physical address where the kernel was loaded (decided by runtime)
};

/// \brief This struct encodes device properties
struct DeviceProperties {
  enum class FormFactor { PCIE, M2 };
  enum class ArchRevision { ETSOC1, PANTERO, GEPARDO, UNKNOWN = -1 };
  uint32_t frequency_;                  ///< minion boot frequency in Mhz
  uint32_t availableShires_;            ///< device shire count
  uint32_t memoryBandwidth_;            ///< device memory bandwidth in MBytes/second
  uint64_t memorySize_;                 ///< device memory size in bytes
  uint16_t l3Size_;                     ///< device L3 size in bytes
  uint16_t l2shireSize_;                ///< device L2 shire size in bytes
  uint16_t l2scratchpadSize_;           ///< device L2 scratchpad size in bytes
  uint16_t cacheLineSize_;              ///< device caceh line size in bytes
  uint16_t l2CacheBanks_;               ///< number of banks in the L2
  uint32_t computeMinionShireMask_;     ///< mask which indicates what are the compute minion shires
  uint16_t spareComputeMinionoShireId_; ///< spare compute minion Shire ID
  ArchRevision deviceArch_;             ///< device architecture revision
  FormFactor formFactor_;               ///< device form factor
  uint8_t tdp_;                         ///< TDP in Watts
  uint64_t p2pBitmap_ = 0;              ///< indicates p2p capabilities between this device and others
  uint32_t availableChiplets_ = 1;      ///< device chiplet count
};

// NOTE: this is copied directly from device firmware "encoder.h"; we need to find a proper solution. So this will be in
// a experimental status until everything is properly designed.
struct UserTrace {
  uint64_t buffer_;      /*!< Base address for Trace buffer. */
  uint32_t buffer_size_; /*!< Total size of the Trace buffer. */
  uint32_t threshold_;   /*!< Threshold for free memory in the buffer for each hart. */
  uint64_t shireMask_;   /*!< Bit Mask of Shire to enable Trace Capture. */
  uint64_t threadMask_;  /*!< Bit Mask of Thread within a Shire to enable Trace Capture. */
  uint32_t eventMask_;   /*!< This is a bit mask, each bit corresponds to a specific Event to trace. */
  uint32_t filterMask_;  /*!< This is a bit mask representing a list of filters for a given event to trace. */
};

// These two structs (DmaInfo and DeviceConfig) are directly copied from IDeviceLayer.h; these needs to be republished
// by runtime since runtime consumers dont necessary need to know about DeviceLayer component once runtime multiprocess
// is released
struct DmaInfo {
  uint64_t maxElementSize_;  ///< maximum amount of memory that can be transfer per each DMA command entry
  uint64_t maxElementCount_; ///< max number of DMA entries per DMA command
};

/// These are related to DMA transfers, intended for internal use only

enum class CmaCopyType { TO_CMA, FROM_CMA }; // type of CMA
using CmaCopyFunction = std::function<void(const std::byte* src, std::byte* dst, size_t size, CmaCopyType type)>;
static constexpr auto defaultCmaCopyFunction = [](const std::byte* src, std::byte* dst, size_t size, CmaCopyType) {
  std::copy(src, src + size, dst);
};

// Forward declaration
struct KernelLaunchOptionsImp;

class KernelLaunchOptions {

  friend class RuntimeImp;

  void setIfImpIsNull(void);

  KernelLaunchOptions(const rt::KernelLaunchOptionsImp& kOptImp);

public:
  KernelLaunchOptions();
  virtual ~KernelLaunchOptions();

  /// \brief Set in what shires the kernel will be executed, by default it gets the max shires availables
  /// depending on device type.
  void setShireMask(uint64_t shireMask);
  /// \brief Set if the kernel execution should be postponed till all previous works issued into this stream finish (a
  /// barrier). Usually the kernel launch must be postponed till some previous memory operations end, hence the default
  /// value is true.
  void setBarrier(bool barrier);
  /// \brief Set if the L3 should be flushed before the kernel execution starts, by default is false.
  void setFlushL3(bool flushL3);

  /// \brief Set user tracing parameters
  /// \param buffer Base address for the device trace buffer (previously allocated with \ref mallocDevice). If not null,
  /// the firmware will use it to store user trace data. Must be 4KB * num_harts (4KB*2080) big
  /// \param bufferSize Total size of the trace buffer
  /// \param threshold Threshold for free memory in the buffer for each hart
  /// \param shireMask Bit Mask of Shire to enable Trace Capture
  /// \param threadMask Bit Mask of Thread within a Shire to enable Trace Capture
  /// \param eventMask This is a bit mask, each bit corresponds to a specific Event to trace
  /// \param filterMask This is a bit mask representing a list of filters for a given event to trace
  void setUserTracing(uint64_t buffer, uint32_t bufferSize, uint32_t threshold, uint64_t shireMask, uint64_t threadMask,
                      uint32_t eventMask, uint32_t filterMask);

  /// \brief Set the stack size
  /// \param baseAddress Device base address of the stacks. Must be aligned to 4KB.
  /// \param size Size of desired stack to be distribute between the all harts. Must be a multiple of 4KB.
  /// \note Throws an exception if the alignment or multiplicity constraints are not met
  void setStackConfig(std::byte* baseAddress, uint64_t size);

  /// \brief Set the path of the file that will contain a core dump if the execution throws an exception
  void setCoreDumpFilePath(const std::string& coreDumpFilePath);

  std::unique_ptr<KernelLaunchOptionsImp> imp_;
};

} // namespace rt

namespace std {
string to_string(rt::DeviceErrorCode);
inline string to_string(const rt::StreamError& e) {
  return e.getString();
}
} // namespace std
