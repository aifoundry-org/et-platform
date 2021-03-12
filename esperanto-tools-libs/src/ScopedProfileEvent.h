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
#include <optional>
#include <tuple>
#include <atomic>

namespace rt {
namespace profiling {
class ScopedProfileEvent {
public:
  explicit ScopedProfileEvent(Class cls, ProfilerImp& profiler, StreamId streamId,
                              std::unordered_map<std::string, uint64_t> extra = {})
    : profiler_(profiler)
    , event_{Type::Start, cls} {
    event_.extra_ = std::move(extra);
    event_.setStream(streamId);
    init();
  }
  explicit ScopedProfileEvent(Class cls, ProfilerImp& profiler, EventId eventId)
    : profiler_(profiler)
    , event_{Type::Start, cls} {
    event_.setEvent(eventId);
    init();
  }
  explicit ScopedProfileEvent(Class cls, ProfilerImp& profiler, StreamId streamId, EventId eventId)
    : profiler_(profiler)
    , event_{Type::Start, cls, streamId, eventId} {
  }
  explicit ScopedProfileEvent(Class cls, ProfilerImp& profiler)
    : profiler_(profiler)
    , event_{Type::Start, cls} {
      init();
  }
  void init() {
    event_.addExtra("pair_id", nextPairId_++);
    profiler_.record(event_);
  }

  void setEventId(EventId event) {
    event_.setEvent(event);
  }

  ~ScopedProfileEvent() {
    event_.setTimeStamp();
    event_.type_ = Type::End;
    profiler_.record(event_);
  }

private:
  inline static std::atomic<int> nextPairId_ = 0;
  ProfilerImp& profiler_;
  ProfileEvent event_;
};
} // namespace profiling
} // namespace rt