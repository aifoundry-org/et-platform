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
#include "ProfilerImp.h"
#include "runtime/IProfileEvent.h"

#include <atomic>
#include <optional>
#include <tuple>

namespace rt::profiling {

class ScopedProfileEvent {
public:
  explicit ScopedProfileEvent(Class cls, IProfiler& profiler, StreamId streamId, KernelId kernelId,
                              uint64_t loadAddress)
    : profiler_(profiler)
    , event_{Type::Complete, cls} {
    event_.setStream(streamId);
    event_.setKernelId(kernelId);
    event_.setLoadAddress(loadAddress);
    init();
  }
  explicit ScopedProfileEvent(Class cls, IProfiler& profiler, KernelId kernelId)
    : profiler_(profiler)
    , event_{Type::Complete, cls} {
    event_.setKernelId(kernelId);
    init();
  }
  explicit ScopedProfileEvent(Class cls, IProfiler& profiler, DeviceId deviceId)
    : profiler_(profiler)
    , event_{Type::Complete, cls} {
    event_.setDeviceId(deviceId);
    init();
  }
  explicit ScopedProfileEvent(Class cls, IProfiler& profiler, StreamId streamId)
    : profiler_(profiler)
    , event_{Type::Complete, cls} {
    event_.setStream(streamId);
    init();
  }
  explicit ScopedProfileEvent(Class cls, IProfiler& profiler, EventId eventId)
    : profiler_(profiler)
    , event_{Type::Complete, cls} {
    event_.setEvent(eventId);
    init();
  }
  explicit ScopedProfileEvent(Class cls, IProfiler& profiler)
    : profiler_(profiler)
    , event_{Type::Complete, cls} {
    init();
  }
  void init() {
    event_.setTimeStamp();
  }

  void setEventId(EventId event) {
    event_.setEvent(event);
  }

  void setStream(StreamId stream) {
    event_.setStream(stream);
  }

  void setLoadAddress(uint64_t address) {
    event_.setLoadAddress(address);
  }

  void setKernelId(KernelId kernelId) {
    event_.setKernelId(std::move(kernelId));
  }

  void setDeviceProperties(DeviceProperties props) {
    event_.setDeviceProperties(std::move(props));
  }

  void setParentId(EventId parent) {
    event_.setParentId(parent);
  }

  void recordNow() {
    if (!isRecorded_) {
      isRecorded_ = true;
      event_.setDuration(ProfileEvent::Clock::now() - event_.getTimeStamp());
      static_cast<IProfilerRecorder&>(profiler_).record(event_);
    }
  }

  ~ScopedProfileEvent() {
    recordNow();
  }

  ScopedProfileEvent(const ScopedProfileEvent&) = delete;
  ScopedProfileEvent& operator=(const ScopedProfileEvent&) = delete;

  ScopedProfileEvent(ScopedProfileEvent&&) noexcept = delete;
  ScopedProfileEvent& operator=(ScopedProfileEvent&&) = delete;

private:
  IProfiler& profiler_;
  ProfileEvent event_;
  bool isRecorded_ = false;
};

} // namespace rt::profiling
