//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#ifndef ET_RUNTIME_CODE_MANAGEMENT_UBERKERNEL_H
#define ET_RUNTIME_CODE_MANAGEMENT_UBERKERNEL_H

/// @file
#include "esperanto/runtime/CodeManagement/Kernel.h"

#include <vector>

namespace et_runtime {

/// @class UberKernel UberKernel.h
///
/// @brief Class holdiding the infomration of a uber kernel code and type of
/// arguments
///
///
class UberKernel final : public Kernel {
public:
  using ArgSignature = std::vector<std::vector<ArgType>>;
  using ArgValues = std::vector<std::vector<LaunchArg>>;
  /// @class UberKernelLaunch
  ///
  /// @brief Launch a kernel on the devie with the specified arguments
  //
  class UberKernelLaunch {
  public:
    ///
    /// @param[in] kernel Reference to a registered kernel
    /// @param[in] args  Vector with the arguments to pass to the kernel and
    /// their type
    ///   Their types will be checked against the registered signature of the
    ///   kernel
    UberKernelLaunch(const UberKernel &kernel, const ArgValues &args);

    /// @brief Launch the kernel and wait for the result
    ///
    /// This is a blocking kernel launch the function will block until a result
    /// is received from the device
    ///
    /// @param[in] stream  Stream to launch this kernel on
    /// @returns etrtSuccess or error describing the kernel launch status
    etrtError launchBlocking(Stream *stream);

    /// @brief Non blocking kernel launch
    ///
    /// @param[in] stream  Stream to launch this kernel on
    /// @returns etrtSuccess or error describing the kernel launch status
    etrtError launchNonBlocking(Stream *stream);

  private:
    const UberKernel &kernel_;
    // Flattened out buffer of all arguments to pass to the UberKernel
    std::vector<unsigned char>
        arg_vals_; ///< Actual byte array with all arguments to be transfered to
                   ///< the device

    /// @brief Helper function for launching a kernel
    ErrorOr<std::shared_ptr<device_api::devfw_commands::KernelLaunchCmd>>
    launchHelper(Stream *stream);
  };

  /// @brief Costruct a new Uber Kernel
  ///
  /// @param[in] name  Name of the UberKernel that needs to be unique across all
  /// kernels in the system
  /// @param[in] arg_list : Signature of the arguments the kernel expects
  /// @param[in] mid   Id of the module this module is part of
  UberKernel(const std::string &name, const ArgSignature &arg_list,
             CodeModuleID mid);

  const std::vector<std::vector<ArgType>> argList() const { return arg_list_; }

  std::unique_ptr<UberKernelLaunch>
  createKernelLaunch(const ArgValues &args) const {
    return std::make_unique<UberKernelLaunch>(*this, args);
  }

  static ErrorOr<UberKernel &> findKernel(KernelCodeID id);

private:
  std::vector<std::vector<ArgType>> arg_list_;

  static constexpr int MASTER_SHIRE_NUM = 32;
};

} // namespace et_runtime
#endif // ET_RUNTIME_CODE_MANAGEMENT_UBERKERNEL_H
