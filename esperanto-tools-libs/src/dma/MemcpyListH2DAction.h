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