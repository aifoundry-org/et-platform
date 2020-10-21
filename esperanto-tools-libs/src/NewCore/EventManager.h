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

#include "runtime/IRuntime.h"
#include <condition_variable>
#include <deque>
#include <unordered_map>
namespace rt {

struct EventSequence {
  using Type = std::underlying_type_t<EventId>;

  explicit EventSequence(EventId event, Type count = 1)
    : start_(static_cast<Type>(event))
    , count_(count) {
  }

  bool contains(EventId event) const {
    auto evt = static_cast<Type>(event);
    return evt >= start_ && evt < start_ + count_;
  }
  enum class Merged { False, Begin, End };
  Merged tryToMerge(const EventSequence& other) {
    if (other.start_ + other.count_ == start_) {
      start_ = other.start_;
      count_ += other.count_;
      return Merged::Begin;
    }
    if (start_ + count_ == other.start_) {
      count_ += other.count_;
      return Merged::End;
    }
    return Merged::False;
  }

  bool operator<(const EventSequence& other) const {
    return start_ < other.start_;
  }

  Type start_;
  Type count_;
};

class EventManager {
public:
  EventId getNextId();
  void dispatch(EventId event);
  bool isDispatched(EventId event) const;
  void blockUntilDispatched(EventId event);

private:
  void awakeBlockedThreads(EventId event);

  std::deque<EventSequence> dispatched_;
  std::mutex condVarMutex_;
  std::unordered_map<EventId, std::unique_ptr<std::condition_variable>> blockedThreads_;
  std::underlying_type_t<EventId> nextEventId_;
};
} // namespace rt