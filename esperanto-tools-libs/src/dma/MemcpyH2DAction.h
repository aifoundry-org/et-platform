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