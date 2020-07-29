//******************************************************************************
// Copyright (C) 2020, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

/// @file

#ifndef ET_RUNTIME_CORE_STATE_H
#define ET_RUNTIME_CORE_STATE_H

#include "esperanto/runtime/Core/DeviceManager.h"

#include <fstream>
#include <memory>

namespace et_runtime {

/// @class CoreState CoreState.h
///
/// This struct holds the core static state of the runtime
/// There is expected to be a single instance of this struct through the
/// lifetime of the runtime and any other static objects that the runtime
/// needs should be its members. This is to ensure that static
/// objects/information is destroyed in a specific order and we do not get
/// random behavior at process exit time where the destruction of objects is not
/// guaranteed (same with the construction order)
struct CoreState {
  std::unique_ptr<std::fstream> runtime_trace;
  std::shared_ptr<DeviceManager> dev_manager;

  CoreState() = default;

  /// The CoreState is not copyable or movable
  CoreState(CoreState &) = delete;
  CoreState(CoreState &&) = delete;
  CoreState &operator=(CoreState &) = delete;
  CoreState &operator=(CoreState &&) = delete;
};

/// @brief Return a pointer to the CoreState of the Runtime
__attribute__((visibility("default"))) CoreState &getCoreState();

} // namespace et_runtime

#endif // ET_RUNTIME_CORE_STATE_H
