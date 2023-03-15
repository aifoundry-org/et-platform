/*-------------------------------------------------------------------------
 * Copyright (C) 2023, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/

#include "MemcpyListD2HAction.h"
#include "MemcpyOps.h"
#include "RuntimeImp.h"
#include "ScopedProfileEvent.h"
#include "StreamManager.h"
#include "dma/CmaManager.h"
#include <numeric>
using namespace actionList;
using namespace rt;
using namespace rt::profiling;

MemcpyListD2HAction::MemcpyListD2HAction(MemcpyList list, bool barrier, MemcpyContext ctx)
  : ctx_(ctx)
  , list_(list)
  , barrier_(barrier) {
  totalSize_ = 0U;
  for (auto& o : list_.operations_) {
    totalSize_ += o.size_;
  }
}

bool MemcpyListD2HAction::update() {

  // alloc buffer for next copy
  auto cmaPtr = ctx_.cmaManager_.alloc(totalSize_);
  if (cmaPtr == nullptr) {
    RT_VLOG(LOW) << "Can't allocate CMA buffer for MemcpyListH2DAction. Required size: " << totalSize_;
    return false;
  }

  auto cmdEvt = getNextId(ctx_);
  MemcpyCommandBuilder builder(MemcpyType::D2H, barrier_, static_cast<uint32_t>(ctx_.dmaInfo_.maxElementCount_));
  builder.setTagId(cmdEvt);
  auto processed = 0UL;

  std::vector<EventId> syncEvents;
  for (auto& op : list_.operations_) {
    auto chunkSize = op.size_;
    builder.addOp(cmaPtr + processed, op.src_, chunkSize);

    auto syncId = getNextId(ctx_);
    syncEvents.emplace_back(syncId);

    // add a callback which will queue a task to the threadpool to do the final copy from cma to host virtual memory
    ctx_.eventManager_.addOnDispatchCallback(
      {{cmdEvt},
       [& rt = ctx_.runtime_, &tp = ctx_.threadPool_, copyFunction = ctx_.cmaCopyFunction_, dst = op.dst_, processed,
        cmaPtr, chunkSize, syncId, evt = ctx_.eventId_] {
         tp.pushTask([dst, copyFunction, &rt, processed, cmaPtr, chunkSize, syncId, evt] {
           // add the tracking information (cmacopy)
           ScopedProfileEvent pevent(profiling::Class::CmaCopy, *rt.getProfiler(), syncId);
           pevent.setParentId(evt);
           copyFunction(cmaPtr + processed, dst, chunkSize, CmaCopyType::FROM_CMA);
           rt.dispatch(syncId);
         });
       }});

    processed += chunkSize;
  }

  // set the proper data once the builder has been filled
  ctx_.commandSender_.sendBefore(ctx_.eventId_,
                                 {builder.build(), ctx_.commandSender_, cmdEvt, ctx_.eventId_, true, true});

  // release the buffer once the command has been completed
  ctx_.eventManager_.addOnDispatchCallback(
    {syncEvents, [& cm = ctx_.cmaManager_, &rt = ctx_.runtime_, cmaPtr, evt = ctx_.eventId_] {
       cm.free(cmaPtr);
       rt.dispatch(evt);
     }});
  // remove the ghost command
  ctx_.commandSender_.cancel(ctx_.eventId_);
  return true;
}
