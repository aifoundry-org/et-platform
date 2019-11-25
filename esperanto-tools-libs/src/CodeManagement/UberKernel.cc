//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "esperanto/runtime/CodeManagement/UberKernel.h"
#include "Tracing/Tracing.h"

namespace et_runtime {

UberKernel::UberKernel(const std::string &name,
                       const std::vector<std::vector<ArgType>> &arg_list,
                       CodeModuleID mid)
    : Kernel(mid) {
  name_ = name;
  arg_list_ = arg_list;
  auto success = registerKernel(name, this);
  if (!success) {
    RTERROR << "Kernel by name: " << name << " already exists \n";
    std::terminate();
  }
  TRACE_CodeManager_create_uber_kernel(id_, name);
}

ErrorOr<UberKernel &> UberKernel::findKernel(KernelCodeID id) {
  auto find_res = Kernel::findKernel(id);
  if (!(bool)find_res) {
    return find_res.getError();
  }
  return dynamic_cast<UberKernel &>(find_res.get());
}

} // namespace et_runtime
