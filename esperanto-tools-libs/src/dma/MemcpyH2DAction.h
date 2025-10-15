/*-------------------------------------------------------------------------
 * Copyright (c) 2025 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------*/

#pragma once
#include "MemcpyContext.h"
#include <hostUtils/actionList/ActionList.h>

namespace rt {

class MemcpyH2DAction : public actionList::IAction {
public:
  MemcpyH2DAction(const std::byte* h_src, std::byte* d_dst, size_t size, bool barrier, MemcpyContext ctx);
  bool update() override;
  void onFinish() override;

private:
  MemcpyContext ctx_;
  std::vector<EventId> cmdEvents_;
  const std::byte* h_src_;
  std::byte* d_dst_;
  size_t size_;
  size_t pos_ = 0;
  bool barrier_;
};
} // namespace rt