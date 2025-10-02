/*-------------------------------------------------------------------------
 * Copyright (C) 2023, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
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