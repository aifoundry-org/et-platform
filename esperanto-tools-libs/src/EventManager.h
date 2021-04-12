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
#include "utils.h"
#include <atomic>
#include <condition_variable>
#include <set>
#include <unordered_map>
namespace rt {

class EventManager {
public:
  EventId getNextId();
  void dispatch(EventId event);
  // returns false if the timeout is reached; true otherwise
  bool blockUntilDispatched(EventId event, std::chrono::milliseconds timeout);
  std::set<EventId> getOnflyEvents() const;

private:

  // need a semaphore to deal with spurious wakeups, a simple cond_variable is not enough
  struct Semaphore {
    void notifyAll() {
      ready_ = true;
      condVar_.notify_all();
    }
    // returns false if the timeout is reached; true otherwise
    bool wait(std::unique_lock<std::mutex>& lock, std::chrono::milliseconds timeout) {
      count_++;
      LOG_IF(FATAL, timeout.count() == 0) << "Count can't be zero!";
      RT_DLOG(INFO) << "Blocking thread for a max of " << timeout.count() << " milliseconds.";
      auto res = condVar_.wait_for(lock, timeout, [this]() { return ready_; });
      RT_DLOG(INFO) << "Thread unblocked ready value: " << ready_ << " wait_for result: " << res;
      count_--;
      return res;
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
