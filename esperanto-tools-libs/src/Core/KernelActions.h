//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#ifndef ET_RUNTIME_DEVICE_KERNEL_ACTIONS_H
#define ET_RUNTIME_DEVICE_KERNEL_ACTIONS_H

/// @file

#include "esperanto/runtime/CodeManagement/Kernel.h"
#include "esperanto/runtime/CodeManagement/UberKernel.h"
#include "esperanto/runtime/Support/ErrorOr.h"

#include <esperanto/device-api/device_api.h>
#include <stdint.h>

/// This file implements a number of helper classes that enable interatiions
/// with the kernels classes: lauching a kernel, querying its state and aborting
/// it

namespace et_runtime {

/// @class KernelActions
///
// @brief Helper class that exposes a set of common actions we can perform
// with a kernel
class KernelActions {
public:
  KernelActions() = default;

  /// @brief Laucnh a kernel and wait until completion
  ///
  /// @param[in] stream: Stream where the kernel is going to be launched
  /// @returns @ref Error Success if the launch was successful or error
  /// otherwise
  template <class KernelType>
  etrtError
  launchBlocking(Stream *stream, const KernelType &kernel,
                 const typename KernelType::ArgValues &arguments) const {
    auto kernel_launch = kernel.createKernelLaunch(arguments);
    return kernel_launch->launchBlocking(stream);
  }

  /// @brief Laucnh a kernel and return immediately. The user should synchronize
  /// on the stream in order to wait for completion
  ///
  /// @param[in] stream: Stream where the kernel is going to be launched
  /// @returns @ref Error Success if the launch was successful or error
  /// otherwise
  template <class KernelType>
  etrtError
  launchNonBlocking(Stream *stream, const KernelType &kernel,
                    const typename KernelType::ArgValues &arguments) const {
    auto kernel_launch = kernel.createKernelLaunch(arguments);
    return kernel_launch->launchNonBlocking(stream);
  }

  /// @brief Return the state of the kernel or encountered error
  ErrorOr<::device_api::DEV_API_KERNEL_STATE> state(Stream *stream) const;

  /// @brief Abort a running kenrel or return an encountered error
  ErrorOr<::device_api::DEV_API_KERNEL_ABORT_RESPONSE_RESULT>
  abort(Stream *stream) const;

private:
  unsigned int id_ =
      0; ///< ID of the kenrel to use on the device, currently the device fw can
         ///< keep track of up to 4 running kernels but we are using only 1
};

} // namespace et_runtime

#endif // ET_RUNTIME_DEVICE_KERNEL_ACTIONS_H
