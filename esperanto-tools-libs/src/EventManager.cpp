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
#include "Utils.h"
#include "runtime/IRuntime.h"
#include <algorithm>
#include <cassert>
#include <condition_variable>
#include <mutex>

using namespace rt;

EventId EventManager::getNextId() {
  std::lock_guard lock(mutex_);
  auto res = EventId{nextEventId_++};
  onflyEvents_.emplace(res);
  RT_VLOG(HIGH) << "Last event id: " << static_cast<int>(res);
  return res;
}

void EventManager::dispatch(EventId event) {
  RT_VLOG(HIGH) << "Dispatching event " << static_cast<int>(event);
  std::unique_lock lock(mutex_);
  if (onflyEvents_.erase(event) != 1) {

    std::stringstream ss;
    for (auto& e : onflyEvents_) {
      ss << static_cast<int>(e) << " ";
    }
    RT_VLOG(HIGH) << "Events on-fly: " << ss.str();

    throw Exception("Couldn't dispatch event: " + std::to_string(static_cast<int>(event)) +
                    ". Perhaps it was already dispatched?");
  }

  auto it = blockedThreads_.find(event);
  if (it != end(blockedThreads_)) {
    lock.unlock();
    it->second->notifyAll();
  }
}

bool EventManager::isDispatched(EventId event) const {
  return onflyEvents_.find(event) == end(onflyEvents_);
}

bool EventManager::blockUntilDispatched(EventId event, std::chrono::milliseconds timeout) {
  std::unique_lock lock(mutex_);
  if (isDispatched(event)) {
    return true; // no block if the event is already dispatched
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
  auto res = sem->wait(lock, timeout);
  if (!sem->isAnyThreadBlocked()) {
    blockedThreads_.erase(event);
  }
  return res;
}
