//******************************************************************************
// Copyright (C) 2020, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//-----------------------------------------------------------------------------

#ifndef ET_RUNTIME_TRACING_H
#define ET_RUNTIME_TRACING_H

#include "esperanto/runtime/Common/ErrorTypes.h"


namespace et_runtime {

class Device;

/// @class Profiler Profiler.h esperanto/runtime/Core/Profiler.h
///
/// @brief Class that implements basic profiling interface to
/// interact with the runtime & device.
///
class Profiler {

public:
  /// @brierf Profiler constructor
  ///
  /// @param[in] dev Reference to the associate device
  Profiler(Device &dev);

  /// @brief Starts profile collection. If it was already
  /// started, then has no effect.
  /// 
  /// @returns etrtSuccess, etrtErrorInvalidValue,
  etrtError start();

  /// @brief Stops profile collection. If it was already
  /// disabled, then has no effect.
  /// 
  /// @returns etrtSuccess, etrtErrorInvalidValue,
  etrtError stop();

  /// @brief Sends all collected events to disc
  /// 
  /// @returns etrtSuccess, etrtErrorInvalidValue,
  etrtError flush();

private:
  Device &dev_; ///< Device object, this class interacts with
};

} // namespace et_runtime

#endif
