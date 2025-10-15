/*-------------------------------------------------------------------------
 * Copyright (c) 2025 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------*/

#pragma once
#include "MemcpyContext.h"
#include "runtime/Types.h"
#include <hostUtils/actionList/ActionList.h>

namespace rt {

class MemcpyD2HAction : public actionList::IAction {
public:
  MemcpyD2HAction(const std::byte* d_src, std::byte* h_dst, size_t size, bool barrier, MemcpyContext ctx);
  bool update() override;
  void onFinish() override;

private:
  MemcpyContext ctx_;
  std::vector<EventId> cmdEvents_;
  const std::byte* d_src_;
  std::byte* h_dst_;
  size_t size_;
  size_t pos_ = 0;
  bool barrier_;
};
} // namespace rt