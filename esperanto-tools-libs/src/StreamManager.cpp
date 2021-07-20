/*-------------------------------------------------------------------------
 * Copyright (C) 2021, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/

#include "StreamManager.h"
#include "runtime/IRuntime.h"
#include "utils.h"
#include <g3log/loglevels.hpp>
#include <type_traits>
using namespace rt;

Stream::Info StreamManager::getStreamInfo(StreamId stream) const {
  std::lock_guard lock(mutex_);
  return find(streams_, stream)->second.info_;
}

StreamId StreamManager::createStream(DeviceId device) {
  std::lock_guard lock(mutex_);
  auto vq = queueHelper_.nextQueue(device);
  auto [it, res] = streams_.try_emplace(StreamId{nextStreamId_++}, Stream{device, vq});
  if (!res) {
    throw Exception("Error creating stream in device " +
                    std::to_string(static_cast<std::underlying_type<DeviceId>::type>(device)));
  }
  return it->first;
}

void StreamManager::destroyStream(StreamId stream) {
  std::lock_guard lock(mutex_);
  auto res = streams_.erase(stream);
  RT_LOG_IF(WARNING, !res) << "Trying to destroy a non-existing stream.";
  if (!res) {
    throw Exception("Trying to destroy a non-existing stream.");
  }
}

bool StreamManager::hasEventsOnFly(DeviceId device) const {
  std::lock_guard lock(mutex_);
  for (auto& [id, stream] : streams_) {
    if (stream.info_.device_ == static_cast<int>(device) && !stream.submittedEvents_.empty()) {
      return true;
    }
  }
  return false;
}

void StreamManager::addDevice(DeviceId device, int queueCount) {
  std::lock_guard lock(mutex_);
  queueHelper_.addDevice(device, queueCount);
}

void StreamManager::addEvent(StreamId stream, EventId event) {
  std::lock_guard lock(mutex_);
  find(streams_, stream)->second.submittedEvents_.emplace(event);
}

void StreamManager::removeEvent(EventId event) {
  std::lock_guard lock(mutex_);
  for (auto& [id, stream] : streams_) {
    if (stream.submittedEvents_.erase(event) > 0)
      return;
  }
  RT_LOG(WARNING) << "Trying to remove a non-existing event: " << static_cast<uint32_t>(event)
                  << ". Perhaps the associated Stream was already destroyed";
}

std::optional<EventId> StreamManager::getLastEvent(StreamId stream) const {
  std::lock_guard lock(mutex_);
  auto& events = find(streams_, stream)->second.submittedEvents_;
  if (!events.empty()) {
    return *rbegin(events);
  }
  return {};
}