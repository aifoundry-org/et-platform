//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "esperanto/runtime/CodeManagement/CodeRegistry.h"
#include "CodeManagement/CodeModule.h"
#include "CodeManagement/ELFSupport.h"
#include "CodeManagement/ModuleManager.h"
#include "esperanto/runtime/CodeManagement/Kernel.h"
#include "esperanto/runtime/CodeManagement/UberKernel.h"

namespace et_runtime {

CodeRegistry::CodeRegistry() : mod_manager_(new ModuleManager()) {}

CodeRegistry &CodeRegistry::registry() {
  static CodeRegistry registry;

  return registry;
}

template <class KernelClass>
ErrorOr<std::tuple<KernelCodeID, KernelClass &>>
CodeRegistry::registerKernelHelper(const std::string &name,
                                   const std::string &elf_path) {

  auto mod_exists = mod_manager_->getModule(name);
  if ((bool)mod_exists == true) {
    return etrtErrorModuleELFDataExists;
  }

  auto mod_info = mod_manager_->createModule(name, elf_path);

  auto mid = std::get<0>(mod_info);
  /// This pointer should be tracked by the KernelRegistry
  auto kernel = new KernelClass(name, mid);

  return std::tuple<KernelCodeID, KernelClass &>{kernel->id(), *kernel};
}

ErrorOr<std::tuple<KernelCodeID, Kernel &>>
CodeRegistry::registerKernel(const std::string &name,
                             const std::string &elf_path) {

  return registerKernelHelper<Kernel>(name, elf_path);
}

ErrorOr<std::tuple<KernelCodeID, UberKernel &>>
CodeRegistry::registerUberKernel(const std::string &name,
                                 const std::string &elf_path) {

  return registerKernelHelper<UberKernel>(name, elf_path);
}

} // namespace et_runtime
