/*-------------------------------------------------------------------------
 * Copyright (c) 2025 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------*/

#pragma once
#include "Utils.h"
#include "runtime/Types.h"
#include <hostUtils/threadPool/ThreadPool.h>
#include <mutex>
#include <set>
#include <type_traits>
#include <unordered_map>

namespace rt {
struct Stream {
  static_assert(sizeof(std::underlying_type<DeviceId>::type) <= sizeof(int));
  Stream(DeviceId deviceId, int vq, StreamId id)
    : info_{static_cast<int>(deviceId), vq, id} {
  }
  ~Stream() {
    RT_LOG_IF(WARNING, !submittedEvents_.empty()) << "Destroying stream with pending events";
    RT_LOG_IF(WARNING, !errors_.empty()) << "Destroying stream with pending errors";
  }
  std::set<EventId> submittedEvents_;
  struct Info {
    int device_;
    int vq_;
    StreamId id_;
  } info_;
  std::vector<StreamError> errors_;
};

class QueueHelper {
public:
  void addDevice(DeviceId device, int queueCount) {
    queues_.try_emplace(device, QueueInfo{queueCount});
  }
  int nextQueue(DeviceId device) {
    return find(queues_, device)->second.getNextQueue();
  }

private:
  struct QueueInfo {
    explicit QueueInfo(int count)
      : nextQueue_{0}
      , queueCount_{count} {
    }
    int getNextQueue() {
      auto res = nextQueue_;
      nextQueue_ = (nextQueue_ + 1) % queueCount_;
      return res;
    }
    int nextQueue_;
    const int queueCount_;
  };
  std::unordered_map<DeviceId, QueueInfo> queues_;
};

class StreamManager {
public:
  Stream::Info getStreamInfo(StreamId stream) const;
  std::optional<Stream::Info> getStreamInfo(EventId event) const;
  StreamId createStream(DeviceId device);
  void destroyStream(StreamId stream);
  bool hasEventsOnFly(DeviceId device) const;
  std::unordered_map<DeviceId, uint32_t> getEventCount() const;

  void addDevice(DeviceId, int queueCount);
  std::vector<EventId> getLiveEvents(StreamId stream) const;

  void addEvent(StreamId stream, EventId event);
  void removeEvent(EventId event);
  std::vector<StreamError> retrieveErrors(StreamId stream);
  void setErrorCallback(StreamErrorCallback callback);
  // returns false if there is no callback. If true, it will execute the callback and after that it will execute the
  // "executeAfterCallback"
  bool executeCallback(EventId eventId, const StreamError& error, const std::function<void()>& executeAfterCallback);
  void addError(EventId event, StreamError error);
  void addError(const StreamError& error);

private:
  threadPool::ThreadPool threadPool_{2};
  QueueHelper queueHelper_;
  std::unordered_map<StreamId, Stream> streams_;
  std::underlying_type<StreamId>::type nextStreamId_ = 0;
  StreamErrorCallback streamErrorCallback_;
  mutable std::mutex mutex_;
};

} // namespace rt