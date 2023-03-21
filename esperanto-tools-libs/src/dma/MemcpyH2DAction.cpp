/*-------------------------------------------------------------------------
 * Copyright (C) 2023, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/

#include "MemcpyH2DAction.h"
#include "EventManager.h"
#include "MemcpyOps.h"
#include "RuntimeImp.h"
#include "ScopedProfileEvent.h"
#include "StreamManager.h"
#include "dma/CmaManager.h"

using namespace actionList;
using namespace rt;
using namespace rt::profiling;

MemcpyH2DAction::MemcpyH2DAction(const std::byte* h_src, std::byte* d_dst, size_t size, bool barrier, MemcpyContext ctx)
  : ctx_(ctx)
  , h_src_(h_src)
  , d_dst_(d_dst)
  , size_(size)
  , barrier_(barrier) {
}

bool MemcpyH2DAction::update() {
  RT_VLOG(MID) << "MemcpyH2DAction::update for command with eventId: " << static_cast<int>(ctx_.eventId_);
  assert(pos_ < size_);

  // alloc buffer for next copy
  auto availableBytes = ctx_.cmaManager_.getFreeBytes();
  if (availableBytes == 0) {
    return false;
  }

  auto currentSize =
    std::min(std::min(availableBytes, size_ - pos_), ctx_.dmaInfo_.maxElementSize_ * ctx_.dmaInfo_.maxElementCount_);
  auto cmaPtr = ctx_.cmaManager_.alloc(currentSize);

  // add a copy task to the threadpool
  auto cmdEvt = getNextId(ctx_);
  MemcpyCommandBuilder builder(MemcpyType::H2D, barrier_, static_cast<uint32_t>(ctx_.dmaInfo_.maxElementCount_));
  builder.setTagId(cmdEvt);
  auto processed = 0UL;

  std::vector<EventId> syncEvents;
  while (processed < currentSize) {
    auto chunkSize = std::min(ctx_.dmaInfo_.maxElementSize_, currentSize - processed);
    builder.addOp(cmaPtr + processed, d_dst_ + pos_ + processed, chunkSize);

    auto syncId = getNextId(ctx_);
    syncEvents.emplace_back(syncId);

    ctx_.threadPool_.pushTask([& rt = ctx_.runtime_, copyFunction = ctx_.cmaCopyFunction_, processed, cmaPtr, chunkSize,
                               syncId, src = h_src_, pos = pos_, evt = ctx_.eventId_] {
      ScopedProfileEvent pevent(profiling::Class::CmaCopy, *rt.getProfiler(), syncId);
      pevent.setParentId(evt);
      copyFunction(src + pos + processed, cmaPtr + processed, chunkSize, CmaCopyType::TO_CMA);
      rt.dispatch(syncId);
    });

    processed += chunkSize;
  }
  pos_ += currentSize;

  RT_VLOG(MID) << ">>> Alloc cmaPtr: " << std::hex << cmaPtr << " associated event: " << int(cmdEvt);

  // set the proper data once the builder has been filled
  ctx_.commandSender_.sendBefore(ctx_.eventId_,
                                 {builder.build(), ctx_.commandSender_, cmdEvt, ctx_.eventId_, true, false});

  // once all cmacopies has been done, enable the command
  ctx_.eventManager_.addOnDispatchCallback(
    {std::move(syncEvents), [& cs = ctx_.commandSender_, cmdEvt] { cs.enable(cmdEvt); }});
  cmdEvents_.emplace_back(cmdEvt);

  // release the buffer once the command has been completed
  ctx_.eventManager_.addOnDispatchCallback({{cmdEvt}, [& cm = ctx_.cmaManager_, cmaPtr] {
                                              RT_VLOG(MID) << ">>> Free cmaPtr: " << std::hex << cmaPtr;
                                              cm.free(cmaPtr);
                                            }});

  return pos_ == size_;
}

void MemcpyH2DAction::onFinish() {
  ctx_.commandSender_.cancel(ctx_.eventId_);
  // add a callback when all commands have been completed, dispatch the event
  ctx_.eventManager_.addOnDispatchCallback(
    {std::move(cmdEvents_), [& rt = ctx_.runtime_, eventId = ctx_.eventId_] { rt.dispatch(eventId); }});
}
