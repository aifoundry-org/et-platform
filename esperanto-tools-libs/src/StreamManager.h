/*-------------------------------------------------------------------------
 * Copyright (C) 2021, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/

#pragma once
#include "utils.h"
#include <mutex>
#include <runtime/IRuntime.h>
#include <set>
#include <type_traits>
#include <unordered_map>

namespace rt {
struct Stream {
  static_assert(sizeof(std::underlying_type<DeviceId>::type) <= sizeof(int));
  Stream(DeviceId deviceId, int vq)
    : info_{static_cast<int>(deviceId), vq} {
  }
  ~Stream() {
    RT_LOG_IF(WARNING, !submittedEvents_.empty()) << "Destroying stream with pending events";
  }
  std::set<EventId> submittedEvents_;
  struct Info {
    int device_;
    int vq_;
  } info_;
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
  StreamId createStream(DeviceId device);
  void destroyStream(StreamId stream);
  bool hasEventsOnFly(DeviceId device) const;
  void addDevice(DeviceId, int queueCount);
  std::optional<EventId> getLastEvent(StreamId stream) const;

  void addEvent(StreamId stream, EventId event);
  void removeEvent(EventId event);

private:
  QueueHelper queueHelper_;
  std::unordered_map<StreamId, Stream> streams_;
  std::underlying_type<StreamId>::type nextStreamId_ = 0;
  mutable std::mutex mutex_;
};

} // namespace rt