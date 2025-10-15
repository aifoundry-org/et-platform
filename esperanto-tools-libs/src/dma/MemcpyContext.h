/*-------------------------------------------------------------------------
 * Copyright (c) 2025 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------*/

#pragma once
#include "EventManager.h"
#include "StreamManager.h"
#include "runtime/IProfileEvent.h"
#include "runtime/Types.h"
#include <device-layer/IDeviceLayer.h>
#include <hostUtils/threadPool/ThreadPool.h>

namespace rt {
struct MemcpyContext {
  CmaCopyFunction cmaCopyFunction_;
  dev::DmaInfo dmaInfo_;
  class RuntimeImp& runtime_;
  class CmaManager& cmaManager_;
  class StreamManager& streamManager_;
  class EventManager& eventManager_;
  class CommandSender& commandSender_;
  threadPool::ThreadPool& threadPool_;
  StreamId stream_;
  EventId eventId_;
};
inline EventId getNextId(MemcpyContext& ctx) {
  auto evt = ctx.eventManager_.getNextId();
  ctx.streamManager_.addEvent(ctx.stream_, evt);
  return evt;
}
} // namespace rt