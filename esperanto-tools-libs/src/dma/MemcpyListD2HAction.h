/*-------------------------------------------------------------------------
 * Copyright (c) 2025 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------*/

#pragma once
#include "MemcpyContext.h"
#include <hostUtils/actionList/ActionList.h>

namespace rt {

class MemcpyListD2HAction : public actionList::IAction {
public:
  MemcpyListD2HAction(MemcpyList list, bool barrier, MemcpyContext ctx);
  bool update() override;

private:
  MemcpyContext ctx_;
  MemcpyList list_;
  size_t totalSize_;
  bool barrier_;
};
} // namespace rt