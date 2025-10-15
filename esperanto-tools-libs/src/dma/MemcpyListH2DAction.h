/*-------------------------------------------------------------------------
 * Copyright (c) 2025 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------*/

#pragma once
#include "MemcpyContext.h"
#include "runtime/Types.h"
#include <hostUtils/actionList/ActionList.h>

namespace rt {

class MemcpyListH2DAction : public actionList::IAction {
public:
  MemcpyListH2DAction(MemcpyList list, bool barrier, MemcpyContext ctx);
  bool update() override;

private:
  MemcpyContext ctx_;
  MemcpyList list_;
  size_t totalSize_;
  bool barrier_;
};
} // namespace rt