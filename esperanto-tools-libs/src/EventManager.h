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

#include "Utils.h"
#include "runtime/IRuntime.h"
#include <hostUtils/threadPool/ThreadPool.h>
#include <hostUtils/threadPool/function2.hpp>

#include <atomic>
#include <condition_variable>
#include <set>
#include <unordered_map>
namespace rt {

class EventManager {
public:
  // the callback will be executed when eventsWatched have been dispatched (ie. does not exist on fly those elements)
  struct OnDispatchCallback {
    std::vector<EventId> eventsWatched_;
    fu2::unique_function<void()> callback_;
  };

  EventId getNextId();
  void dispatch(EventId event);
  // returns false if the timeout is reached; true otherwise
  bool blockUntilDispatched(EventId event, std::chrono::milliseconds timeout);
  void setThrowOnMissingEvent(bool value) {
    throwOnMissingEvent_ = value;
  }
  ~EventManager();

  void addOnDispatchCallback(OnDispatchCallback callback);

  // THIS CALL IS NOT THREADSAFE, this is called internally and could be called from a OnDispatchCallback safely, don't
  // call it from other places
  bool isDispatched(EventId event) const;

private:
  mutable std::mutex mutex_;
  bool throwOnMissingEvent_ = false;
  std::set<EventId> onflyEvents_;
  std::vector<OnDispatchCallback> callbacks_;
  std::unordered_map<EventId, std::unique_ptr<std::condition_variable>> blockedThreads_;
  std::underlying_type_t<EventId> nextEventId_ = 0;
  threadPool::ThreadPool callbackExecutor_{2};
};
} // namespace rt
