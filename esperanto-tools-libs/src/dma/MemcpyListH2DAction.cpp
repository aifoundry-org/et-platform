/*-------------------------------------------------------------------------
 * Copyright (C) 2023, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/

#include "MemcpyListH2DAction.h"
#include "MemcpyOps.h"
#include "RuntimeImp.h"
#include "ScopedProfileEvent.h"
#include "dma/CmaManager.h"
#include <numeric>
using namespace actionList;
using namespace rt;
using namespace rt::profiling;

MemcpyListH2DAction::MemcpyListH2DAction(MemcpyList list, bool barrier, MemcpyContext ctx)
  : ctx_(ctx)
  , list_(list)
  , barrier_(barrier) {
  totalSize_ = 0U;
  for (auto& o : list_.operations_) {
    totalSize_ += o.size_;
  }
}

bool MemcpyListH2DAction::update() {

  // alloc buffer for next copy
  auto cmaPtr = ctx_.cmaManager_.alloc(totalSize_);
  if (cmaPtr == nullptr) {
    RT_VLOG(LOW) << "Can't allocate CMA buffer for MemcpyListH2DAction. Required size: " << totalSize_;
    return false;
  }

  MemcpyCommandBuilder builder(MemcpyType::H2D, barrier_, static_cast<uint32_t>(ctx_.dmaInfo_.maxElementCount_));
  builder.setTagId(ctx_.eventId_);

  std::vector<EventId> syncEvents;
  auto processed = 0UL;
  for (auto& op : list_.operations_) {
    auto chunkSize = op.size_;
    builder.addOp(cmaPtr + processed, op.dst_, chunkSize);

    auto syncId = getNextId(ctx_);
    syncEvents.emplace_back(syncId);

    ctx_.threadPool_.pushTask([copyFunction = ctx_.cmaCopyFunction_, &rt = ctx_.runtime_, processed, cmaPtr, chunkSize,
                               syncId, src = op.src_, evt = ctx_.eventId_] {
      // add the tracking information (cmacopy)
      ScopedProfileEvent pevent(profiling::Class::CmaCopy, *rt.getProfiler(), syncId);
      pevent.setParentId(evt);
      copyFunction(src, cmaPtr + processed, chunkSize, CmaCopyType::TO_CMA);
      rt.dispatch(syncId);
    });
    processed += chunkSize;
  }
  RT_VLOG(MID) << ">>> Alloc cmaPtr: " << std::hex << cmaPtr << " associated events: " << stringizeEvents(syncEvents);

  // set the correct command data, once built
  ctx_.commandSender_.setCommandData(ctx_.eventId_, builder.build());

  // once all cmacopies has been done, enable the command
  ctx_.eventManager_.addOnDispatchCallback(
    {std::move(syncEvents), [& cs = ctx_.commandSender_, evt = ctx_.eventId_] { cs.enable(evt); }});

  ctx_.eventManager_.addOnDispatchCallback({{ctx_.eventId_}, [& cm = ctx_.cmaManager_, cmaPtr] {
                                              RT_VLOG(MID) << ">>> Free cmaPtr: " << std::hex << cmaPtr;
                                              cm.free(cmaPtr);
                                            }});

  return true;
}