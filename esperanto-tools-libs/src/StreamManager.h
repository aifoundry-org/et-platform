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
#include <mutex>
#include <runtime/IRuntime.h>
#include <set>
#include <unordered_map>

namespace rt {
struct Stream {
  Stream(DeviceId deviceId, int vq)
    : deviceId_(deviceId)
    , vq_(vq) {
  }
  bool removeEvent(EventId event) {
    return submittedEvents_.erase(event) > 0;
  }

  void addEvent(EventId event) {
    submittedEvents_.emplace(event);
  }

  std::set<EventId> submittedEvents_;
  DeviceId deviceId_;
  int vq_;
};

class StreamManager {

public:
  const Stream getStream(StreamId stream) const;
  int getSubmissionQueue(StreamId stream) const;
  bool hasEventsOnFly(DeviceId device) const;

  void addEvent(StreamId stream);
  void removeEvent(EventId event);

private:
  std::unordered_map<StreamId, Stream> streams_;
  mutable std::mutex mutex_;
};

} // namespace rt