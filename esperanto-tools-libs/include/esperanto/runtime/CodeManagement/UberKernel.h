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

namespace et_runtime {

/// @class UberKernel UberKernel.h
///
/// @brief Class holdiding the infomration of a uber kernel code and type of
/// arguments
///
///
class UberKernel final : public Kernel {
public:
  /// @brief Costruct a new Uber Kernel
  ///
  /// @param[in] name  Name of the UberKernel that needs to be unique across all
  /// kernels in the system
  /// @param[in] mid   Id of the module this module is part of
  UberKernel(const std::string &name, CodeModuleID mid);

private:
};

} // namespace et_runtime
#endif // ET_RUNTIME_CODE_MANAGEMENT_UBERKERNEL_H
