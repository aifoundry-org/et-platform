/*-------------------------------------------------------------------------
 * Copyright (C) 2020, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/

#pragma once
#include <ostream>

/// \defgroup runtime_profiler_api Runtime Profiler API
///
/// The runtime profiler API allows to gather profiling data from runtime execution.
/// This API expects to receive an output stream where all profiling events will be dumped, the user can explicitely
/// indicate when to start pofiling and when to stop profiling.
/// @{
namespace rt {

/// \brief IProfiler interface declaration, all IProfiler interactions should be made using this interface.
///
class IProfiler {
public:
  /// \brief Profiler user can choose what profiling output generate, Json format or Binary format
  enum class OutputType { Json, Binary };

  /// \brief Virtual Destructor to enable polymorphic release of IProfiler
  /// instances
  virtual ~IProfiler() = default;

  /// \brief Indicates the start of the profiler. The given stream will be accessed freely from the runtime, don't make
  /// concurrent writes to this stream or data races could happen.
  ///
  /// @param[in] outputStream this is the stream where the profiler will dump the data. Make sure its opened in binary
  /// mode.
  /// @param[in] outputType to choose between different formatting options for the outputstream \ref OutputType
  ///
  virtual void start(std::ostream& outputStream, OutputType outputType) = 0;
  ///
  /// \brief This function will stop the profiler from gathering data.
  ///
  virtual void stop() = 0;
};

namespace profiling {
/// \brief Forward declaration of ProfileEvent
class ProfileEvent;

/// \brief IProfilerRecorder interface declaration, its a simple added method to IProfiler interface for recording
class IProfilerRecorder : public IProfiler {
public:
  virtual void record(const ProfileEvent& event) = 0;

  virtual bool isDummy() const {
    return false;
  }

  // Records an event now if profiling has started, or delays it until it starts
  virtual void recordNowOrAtStart(const ProfileEvent& event) = 0;

  // Set up the name of the current thread
  static void setCurrentThreadName(std::string&& threadName) {
    threadName_ = "Host Runtime" + std::move(threadName);
  }

protected:
  static thread_local std::string threadName_;
};

} // namespace profiling
} // namespace rt

/// @}
// End of runtime_profiler_api
