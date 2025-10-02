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