/*-------------------------------------------------------------------------
 * Copyright (C) 2021, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/
#include <hostUtils/debug/StackException.h>
#include <memory>

/// \defgroup runtime_types Runtime Types
/// These are custom types declared by Runtime

namespace rt {
/// \brief Forward declaration of \ref DmaBuffer
class DmaBuffer;

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

/// \brief Stream Status
enum class StreamStatus : int {
  Ok,     ///< Stream do not have any error
  Error,  ///< Stream is in an erroneous state, at least one command reported an error in the response
  Timeout ///< The wait operation timed-out
};

/// \brief RuntimePtr is an alias for a pointer to a Runtime instantation
using RuntimePtr = std::unique_ptr<class IRuntime>;

/// \brief The error handling in the runtime is trough exceptions. This is the
/// only exception kind that the runtime can throw
class Exception : public dbg::StackException {
  using dbg::StackException::StackException;
};

/// \brief Constants
constexpr auto kCacheLineSize = 64U; // TODO This should not be here, it should be
                                     // in a header with project-wide constants
} // namespace rt