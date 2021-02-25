/*-------------------------------------------------------------------------
 * Copyright (C) 2020, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/

#include "EventManager.h"
#include "utils.h"
#include "runtime/IRuntime.h"
#include <algorithm>
#include <cassert>
#include <condition_variable>
#include <mutex>

using namespace rt;

EventId EventManager::getNextId() {
  auto res = EventId{nextEventId_++};
  onflyEvents_.emplace(res);
  return res;
}
std::set<EventId> EventManager::getOnflyEvents() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return onflyEvents_;
}

void EventManager::dispatch(EventId event) {
  RT_DLOG(INFO) << "Dispatching event " << static_cast<int>(event);
  std::unique_lock<std::mutex> lock(mutex_);
  if (onflyEvents_.erase(event) != 1) {
    throw Exception("Couldn't dispatch event, perhaps it was already dispatched?");
  };

  auto it = blockedThreads_.find(event);
  if (it != end(blockedThreads_)) {
    lock.unlock();
    it->second->notifyAll();
  }
}

bool EventManager::isDispatched(EventId event) const {
  return onflyEvents_.find(event) == end(onflyEvents_);
}

void EventManager::blockUntilDispatched(EventId event) {
  std::unique_lock<std::mutex> lock(mutex_);
  if (isDispatched(event)) {
    return; // no block if the event is already dispatched
  }

  auto it = blockedThreads_.find(event);

  Semaphore* sem;
  if (it == end(blockedThreads_)) {
    auto tmp = std::make_unique<Semaphore>();
    sem = tmp.get();
    blockedThreads_[event] = std::move(tmp);
  } else {
    sem = it->second.get();
  }
  sem->wait(lock);
  if (!sem->isAnyThreadBlocked()) {
    blockedThreads_.erase(event);
  }
}
