//******************************************************************************
// Copyright (C) 2020, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "CoreState.h"

namespace et_runtime {

CoreState::~CoreState() {
  dev_manager.reset();
  runtime_trace.reset();
}

struct CoreState &getCoreState() {
  static std::unique_ptr<CoreState> core_state;
  if (!core_state) {
    core_state = std::make_unique<CoreState>();
  }
  return *core_state;
}

} // namespace et_runtime
