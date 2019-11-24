//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#ifndef ET_RUNTIME_MODULE_MANAGER_H
#define ET_RUNTIME_MODULE_MANAGER_H

#include "esperanto/runtime/Common/CommonTypes.h"
#include "esperanto/runtime/Core/Device.h"
#include "esperanto/runtime/Support/ErrorOr.h"

#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace et_runtime {

class Device;
class Module;

class ModuleManager {
public:
  ModuleManager() = default;
  ~ModuleManager() = default;

  std::tuple<CodeModuleID, Module &> createModule(const std::string &name);
  ErrorOr<Module *> getModule(CodeModuleID mid);
  ErrorOr<CodeModuleID> loadOnDevice(CodeModuleID mid, const std::string &path,
                                     Device *dev);
  bool destroyModule(CodeModuleID mid);

private:
  std::vector<std::tuple<CodeModuleID, std::unique_ptr<et_runtime::Module>>>
      module_storage_;
  CodeModuleID module_count_ = 0;
};

} // namespace et_runtime

#endif // ET_RUNTIME_MODULE_MANAGER_H
