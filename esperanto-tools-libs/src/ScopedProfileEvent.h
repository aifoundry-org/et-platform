/*-------------------------------------------------------------------------
 * Copyright (c) 2025 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
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
  explicit ScopedProfileEvent(Class cls, IProfiler& profiler, DeviceId deviceId, size_t size)
    : profiler_(profiler)
    , event_{Type::Complete, cls} {
    event_.setDeviceId(deviceId);
    event_.setSize(size);
    init();
  }
  explicit ScopedProfileEvent(Class cls, IProfiler& profiler, DeviceId deviceId, StreamId streamId, bool barrier,
                              const std::byte* src, const std::byte* dst, size_t size)
    : profiler_(profiler)
    , event_{Type::Complete, cls} {
    event_.setDeviceId(deviceId);
    event_.setStream(streamId);
    event_.setBarrier(barrier);
    event_.setAddressSrc(reinterpret_cast<uint64_t>(src));
    event_.setAddressDst(reinterpret_cast<uint64_t>(dst));
    event_.setSize(size);
    init();
  }
  explicit ScopedProfileEvent(Class cls, IProfiler& profiler, StreamId streamId)
    : profiler_(profiler)
    , event_{Type::Complete, cls} {
    event_.setStream(streamId);
    init();
  }
  explicit ScopedProfileEvent(Class cls, IProfiler& profiler, StreamId streamId, bool barrier)
    : profiler_(profiler)
    , event_{Type::Complete, cls} {
    event_.setStream(streamId);
    event_.setBarrier(barrier);
    init();
  }
  explicit ScopedProfileEvent(Class cls, IProfiler& profiler, StreamId streamId, bool barrier, const std::byte* src,
                              const std::byte* dst, size_t size)
    : profiler_(profiler)
    , event_{Type::Complete, cls} {
    event_.setStream(streamId);
    event_.setBarrier(barrier);
    event_.setAddressSrc(reinterpret_cast<uint64_t>(src));
    event_.setAddressDst(reinterpret_cast<uint64_t>(dst));
    event_.setSize(size);
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

  void setAddress(std::byte* address) {
    event_.setAddress(reinterpret_cast<uint64_t>(address));
  }

  void setAlignment(uint32_t alignment) {
    event_.setAlignment(alignment);
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
