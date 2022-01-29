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

/// \brief Event Handler
enum class EventId : uint16_t {};

/// \brief Stream Handler
enum class StreamId : int {};

/// \brief Device Handler
enum class DeviceId : int {};

/// \brief KernelId Handler
enum class KernelId : int {};

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
struct __attribute__((packed, aligned(64))) ErrorContext {
  uint64_t type_;   ///< 0: compute hang, 1: U-mode exception, 2: system abort, 3: self abort, 4: kernel execution error
  uint64_t cycle_;  ///< The cycle as sampled from the system counters at the point where the event type has happened.
                    ///< Its accurate to within 100 cycles.
  uint64_t hartId_; ///< The Hart thread ID which took the error event.
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
/// \ref ErrorContext (if any)
struct StreamError {
  explicit StreamError(DeviceErrorCode errorCode)
    : errorCode_(errorCode) {
  }
  std::string getString() const; /// < returns a string representation of the StreamError
  DeviceErrorCode errorCode_;
  std::optional<std::vector<ErrorContext>> errorContext_;
};
/// \brief This callback can be optionally set to automatically retrieve stream errors when produced
using StreamErrorCallback = std::function<void(EventId, const StreamError&)>;
/// \brief Constants
constexpr auto kCacheLineSize = 64U; // TODO This should not be here, it should be
                                     // in a header with project-wide constants

/// \brief This struct will contain the results of calling a loadCode function
struct LoadCodeResult {
  EventId event_;          /// < event to wait for the loadCode is complete
  KernelId kernel_;        /// < kernelId associated to the loadCode, to be used in later kernelLaunch(es)
  std::byte* loadAddress_; /// < this is the device physical address where the kernel was loaded (decided by runtime)
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
} // namespace rt

namespace std {
string to_string(rt::DeviceErrorCode);
inline string to_string(const rt::StreamError& e) {
  return e.getString();
}
} // namespace std
