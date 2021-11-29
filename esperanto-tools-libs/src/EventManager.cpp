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
#include <thread>

using namespace rt;

void EventManager::addOnDispatchCallback(OnDispatchCallback callback) {
  std::unique_lock lock(mutex_);
  auto& events = callback.eventsWatched_;
  events.erase(std::remove_if(begin(events), end(events), [this](EventId event) { return isDispatched(event); }),
               end(events));
  if (events.empty()) {
    callbackExecutor_.pushTask(std::move(callback.callback_));
  } else {
    callbacks_.emplace_back(std::move(callback));
  }
}

EventId EventManager::getNextId() {
  std::lock_guard lock(mutex_);
  auto res = EventId{nextEventId_++};
  onflyEvents_.emplace(res);
  RT_VLOG(LOW) << "Last event id: " << static_cast<int>(res);
  return res;
}

void EventManager::dispatch(EventId event) {
  std::unique_lock lock(mutex_);
  RT_VLOG(LOW) << "Dispatching event " << static_cast<int>(event);
  if (onflyEvents_.erase(event) != 1) {

    std::stringstream ss;
    ss << "Couldn't dispatch event: " << static_cast<int>(event)
       << ". Perhaps it was already dispatched?. Events on-fly: ";
    for (auto& e : onflyEvents_) {
      ss << static_cast<int>(e) << " ";
    }
    if (throwOnMissingEvent_) {
      throw Exception("Couldn't dispatch event: " + std::to_string(static_cast<int>(event)) +
                      ". Perhaps it was already dispatched?");
    } else {
      RT_LOG(WARNING) << ss.str();
    }
  }
  for (auto it = begin(callbacks_); it != end(callbacks_);) {
    auto& events = it->eventsWatched_;
    events.erase(std::remove_if(begin(events), end(events), [event](EventId e) { return e == event; }), end(events));
    if (events.empty()) {
      callbackExecutor_.pushTask(std::move(it->callback_));
      it = callbacks_.erase(it);
    } else {
      ++it;
    }
  }

  if (auto it = blockedThreads_.find(event); it != end(blockedThreads_)) {
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
EventManager::~EventManager() {
  using namespace std::chrono_literals;
  std::unique_lock lock(mutex_);
  for (auto& [event, semaphore] : blockedThreads_) {
    RT_LOG(WARNING)
      << "Destroying eventmanager with non-dispatched events. Notifying all threads that where waiting for event "
      << static_cast<int>(event);
    semaphore->notifyAll();
  }
  lock.unlock();
  // while until threads have remove the events
  while (!blockedThreads_.empty()) {
    std::this_thread::sleep_for(1ms);
  }
}