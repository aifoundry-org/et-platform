//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "esperanto/runtime/CodeManagement/Kernel.h"

#include "Tracing/Tracing.h"
#include "esperanto/runtime/Common/CommonTypes.h"
#include "esperanto/runtime/Support/ErrorOr.h"
#include "esperanto/runtime/Support/Logging.h"

namespace et_runtime {

KernelCodeID Kernel::global_kernel_id_ = 1;
Kernel::KernelMap Kernel::kernel_registry_;

Kernel::Kernel(CodeModuleID mid) : id_(global_kernel_id_++), mid_(mid) {}

Kernel::Kernel(const ::std::string &name, CodeModuleID mid) : Kernel(mid) {
  name_ = name;
  auto success = registerKernel(name, this);
  if (!success) {
    RTERROR << "Kernel by name: " << name << " already exists \n";
    std::terminate();
  }
  TRACE_CodeManager_create_kernel(id_, name_);
}

bool Kernel::registerKernel(const std::string &name, Kernel *kernel) {
  auto it = kernel_registry_.find(name);
  if (it != kernel_registry_.end()) {
    return false;
  }
  kernel_registry_[name] = {kernel->id(), std::unique_ptr<Kernel>(kernel)};
  return true;
}

} // namespace et_runtime
