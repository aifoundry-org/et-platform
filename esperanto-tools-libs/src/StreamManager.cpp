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
#include "Utils.h"
#include "runtime/IRuntime.h"
#include "runtime/Types.h"
#include <g3log/loglevels.hpp>
#include <iterator>
#include <mutex>
#include <type_traits>
using namespace rt;

Stream::Info StreamManager::getStreamInfo(StreamId stream) const {
  std::lock_guard lock(mutex_);
  return find(streams_, stream)->second.info_;
}

std::optional<Stream::Info> StreamManager::getStreamInfo(EventId event) const {
  std::lock_guard lock(mutex_);
  for (auto& [id, stream] : streams_) {
    unused(id);
    if (stream.submittedEvents_.find(event) != end(stream.submittedEvents_)) {
      return stream.info_;
    }
  }
  return {};
}

StreamId StreamManager::createStream(DeviceId device) {
  std::lock_guard lock(mutex_);
  auto vq = queueHelper_.nextQueue(device);
  auto id = StreamId{nextStreamId_++};
  auto [it, res] = streams_.try_emplace(id, Stream{device, vq, id});
  if (!res) {
    throw Exception("Error creating stream in device " +
                    std::to_string(static_cast<std::underlying_type<DeviceId>::type>(device)));
  }
  return it->first;
}

void StreamManager::destroyStream(StreamId stream) {
  std::lock_guard lock(mutex_);
  auto res = streams_.erase(stream);
  if (!res) {
    throw Exception("Trying to destroy a non-existing stream.");
  }
}

bool StreamManager::hasEventsOnFly(DeviceId device) const {
  std::lock_guard lock(mutex_);
  for (auto& [id, stream] : streams_) {
    unused(id);
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
  auto [it, result] = find(streams_, stream)->second.submittedEvents_.emplace(event);
  unused(it);
  if (!result) {
    throw Exception("Trying to add an event that already exists in the stream");
  }
}

void StreamManager::removeEvent(EventId event) {
  std::lock_guard lock(mutex_);
  for (auto& [id, stream] : streams_) {
    unused(id);
    if (stream.submittedEvents_.erase(event) > 0)
      return;
  }
  RT_LOG(WARNING) << "Trying to remove a non-existing event: " << static_cast<uint32_t>(event)
                  << ". Perhaps the associated Stream was already destroyed";
}

std::vector<EventId> StreamManager::getLiveEvents(StreamId stream) const {
  std::lock_guard lock(mutex_);
  auto& events = find(streams_, stream)->second.submittedEvents_;
  std::vector<EventId> res;
  res.reserve(events.size());
  std::copy(begin(events), end(events), std::back_inserter(res));
  return res;
}

bool StreamManager::executeCallback(EventId eventId, const StreamError& error) {
  SpinLock lock(mutex_);
  if (!streamErrorCallback_) {
    RT_VLOG(LOW) << "No error callback.";
    return false;
  } else {
    auto cb = streamErrorCallback_;
    lock.unlock();
    cb(eventId, error);
    return true;
  }
}

std::vector<StreamError> StreamManager::retrieveErrors(StreamId stream) {
  std::lock_guard lock(mutex_);
  return std::move(find(streams_, stream)->second.errors_);
}

void StreamManager::setErrorCallback(StreamErrorCallback callback) {
  std::lock_guard lock(mutex_);
  streamErrorCallback_ = std::move(callback);
}

void StreamManager::addError(EventId event, StreamError error) {
  std::lock_guard lock(mutex_);
  for (auto& [id, stream] : streams_) {
    unused(id);
    if (stream.submittedEvents_.find(event) != end(stream.submittedEvents_)) {
      stream.errors_.emplace_back(std::move(error));
      return;
    }
  }
  RT_LOG(WARNING) << "Trying to process an error without a host callback set and the stream was already destroyed. "
                     "So this error will be unnoticed by host.";
}

void StreamManager::addError(const StreamError& error) {
  std::lock_guard lock(mutex_);
  for (auto& [_, stream] : streams_) {
    unused(_);
    stream.errors_.emplace_back(error);
  }
}
