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
#include <atomic>
#include <condition_variable>
#include <unordered_map>
#include <set>
namespace rt {

class EventManager {
public:
  EventId getNextId();
  void dispatch(EventId event);
  void blockUntilDispatched(EventId event);
  std::set<EventId> getOnflyEvents() const;

private:

  // need a semaphore to deal with spurious wakeups, a simple cond_variable is not enough
  struct Semaphore {
    void notifyAll() {
      ready_ = true;
      condVar_.notify_all();
    }
    void wait(std::unique_lock<std::mutex>& lock) {
      count_++;
      condVar_.wait(lock, [this]() { return ready_; });
      count_--;
    }
    bool isAnyThreadBlocked() const {
      return count_ > 0;
    }

    std::condition_variable condVar_;
    int count_ = 0;
    bool ready_ = false;
  };

  bool isDispatched(EventId event) const;

  mutable std::mutex mutex_;
  std::set<EventId> onflyEvents_;
  std::unordered_map<EventId, std::unique_ptr<Semaphore>> blockedThreads_;
  std::underlying_type_t<EventId> nextEventId_ = 0;
};
} // namespace rt
