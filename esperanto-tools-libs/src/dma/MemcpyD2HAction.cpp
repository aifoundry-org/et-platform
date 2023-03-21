/*-------------------------------------------------------------------------
 * Copyright (C) 2023, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/

#include "MemcpyD2HAction.h"
#include "EventManager.h"
#include "MemcpyOps.h"
#include "RuntimeImp.h"
#include "ScopedProfileEvent.h"
#include "StreamManager.h"
#include "dma/CmaManager.h"
using namespace actionList;
using namespace rt;
using namespace rt::profiling;

MemcpyD2HAction::MemcpyD2HAction(const std::byte* d_src, std::byte* h_dst, size_t size, bool barrier, MemcpyContext ctx)
  : ctx_(ctx)
  , d_src_(d_src)
  , h_dst_(h_dst)
  , size_(size)
  , barrier_(barrier) {
}

bool MemcpyD2HAction::update() {
  assert(pos_ < size_);

  // alloc buffer for next copy
  auto availableBytes = ctx_.cmaManager_.getFreeBytes();
  if (availableBytes == 0) {
    return false;
  }

  auto currentSize =
    std::min(std::min(availableBytes, size_ - pos_), ctx_.dmaInfo_.maxElementSize_ * ctx_.dmaInfo_.maxElementCount_);
  auto cmaPtr = ctx_.cmaManager_.alloc(currentSize);

  auto cmdEvt = getNextId(ctx_);
  MemcpyCommandBuilder builder(MemcpyType::D2H, barrier_, static_cast<uint32_t>(ctx_.dmaInfo_.maxElementCount_));
  builder.setTagId(cmdEvt);
  auto processed = 0UL;

  std::vector<EventId> syncEvents;
  while (processed < currentSize) {
    auto chunkSize = std::min(ctx_.dmaInfo_.maxElementSize_, currentSize - processed);
    builder.addOp(cmaPtr + processed, d_src_ + pos_ + processed, chunkSize);

    auto syncId = getNextId(ctx_);
    syncEvents.emplace_back(syncId);

    // add a callback which will queue a task to the threadpool to do the final copy from cma to host virtual memory
    ctx_.eventManager_.addOnDispatchCallback(
      {{cmdEvt},
       [& tp = ctx_.threadPool_, copyFunc = ctx_.cmaCopyFunction_, processed, cmaPtr, chunkSize, syncId, dst = h_dst_,
        pos = pos_, &rt = ctx_.runtime_, evt = ctx_.eventId_] {
         tp.pushTask([copyFunc, &rt, processed, cmaPtr, chunkSize, syncId, pos, dst, evt] {
           ScopedProfileEvent pevent(profiling::Class::CmaCopy, *rt.getProfiler(), syncId);
           pevent.setParentId(evt);
           copyFunc(cmaPtr + processed, dst + pos + processed, chunkSize, CmaCopyType::FROM_CMA);
           rt.dispatch(syncId);
         });
       }});

    processed += chunkSize;
  }
  pos_ += currentSize;

  RT_VLOG(MID) << ">>> Alloc cmaPtr: " << std::hex << cmaPtr << " associated events: " << stringizeEvents(syncEvents);

  /***/
  // set the proper data once the builder has been filled
  ctx_.commandSender_.sendBefore(ctx_.eventId_,
                                 {builder.build(), ctx_.commandSender_, cmdEvt, ctx_.eventId_, true, true});

  // release the buffer once the command has been completed
  ctx_.eventManager_.addOnDispatchCallback({syncEvents, [& cm = ctx_.cmaManager_, cmaPtr] {
                                              RT_VLOG(MID) << ">>> Free cmaPtr: " << std::hex << cmaPtr;
                                              cm.free(cmaPtr);
                                            }});
  for (auto e : syncEvents) {
    cmdEvents_.emplace_back(e);
  }
  return pos_ == size_;
}

void MemcpyD2HAction::onFinish() {
  ctx_.commandSender_.cancel(ctx_.eventId_);
  // add a callback when all commands have been completed, dispatch the event
  ctx_.eventManager_.addOnDispatchCallback(
    {std::move(cmdEvents_), [& rt = ctx_.runtime_, eventId = ctx_.eventId_] { rt.dispatch(eventId); }});
}
