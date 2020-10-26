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
#include <algorithm>
#include <cassert>
#include <condition_variable>

using namespace rt;

EventId EventManager::getNextId() {
  return EventId{nextEventId_++};
}

void EventManager::dispatch(EventId event) {
  auto seq = EventSequence{event};
  std::unique_lock<std::mutex> lock(mutex_);
  if (dispatched_.empty()) {
    dispatched_.emplace_back(seq);
  } else {
    auto it = std::upper_bound(begin(dispatched_), end(dispatched_), seq);
    EventSequence::Merged merged;
    if (it == begin(dispatched_)) {
      merged = it->tryToMerge(seq);
    } else if (it == end(dispatched_)) {
      merged = (it - 1)->tryToMerge(seq);
    } else {
      merged = it->tryToMerge(seq);
      if (merged == EventSequence::Merged::False) {
        merged = (it - 1)->tryToMerge(seq);
      }
    }
    // try to do further merges
    if (merged == EventSequence::Merged::Begin && it != begin(dispatched_)) {
      auto tmp = it->tryToMerge(*(it - 1));
      if (tmp != EventSequence::Merged::False) {
        assert(tmp == EventSequence::Merged::Begin);
        dispatched_.erase(it - 1);
      }
    } else if (merged == EventSequence::Merged::End && it != end(dispatched_)) {
      auto tmp = it->tryToMerge(*(it + 1));
      if (tmp != EventSequence::Merged::False) {
        assert(tmp == EventSequence::Merged::End);
        dispatched_.erase(it + 1);
      }
    } else if (merged == EventSequence::Merged::False) {
      dispatched_.emplace(it, seq);
    }
  }
  lock.unlock();
  awakeBlockedThreads(event);
}

void EventManager::awakeBlockedThreads(EventId event) {
  auto it = blockedThreads_.find(event);
  if (it != end(blockedThreads_)) {
    it->second->notifyAll();
  }
}

bool EventManager::isDispatched(EventId event) const {
  if (dispatched_.empty()) {
    return false;
  }
  auto seq = EventSequence{event};
  auto it = std::upper_bound(begin(dispatched_), end(dispatched_), seq);
  return it != begin(dispatched_) && (--it)->contains(event);
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