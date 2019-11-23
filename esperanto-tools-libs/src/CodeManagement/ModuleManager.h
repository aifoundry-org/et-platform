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

/// @file

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

/// @class ModuleManager ModuleManager.h
///
/// @brief Helper class responsible for keeping track of all the code modules
/// (ELF)  we have
class ModuleManager {
public:
  ModuleManager() = default;
  ~ModuleManager() = default;

  ///
  /// @brief Create a new Module
  ///
  /// @param[in] name Name of the module which should be unique in the system.
  /// For that we could use
  ///    the path of the ELF (but we do not assume that the name is actually the
  ///    path
  ///
  /// @return Tuple that contained the ID and a referenced to the constructed
  /// module
  std::tuple<CodeModuleID, Module &> createModule(const std::string &name);

  ///
  /// @brief Get a registered module by ID
  ///
  /// @param[in] mid ID of the module we are searching for
  ///
  /// @return  Error or a pointer to the restered module
  ErrorOr<Module *> getModule(CodeModuleID mid);

  ///
  /// @brief Get a regisetered module by name
  ///
  /// @param[in] name Name of the module to search with
  ///
  /// @return Error or pointer to the registered module
  ErrorOr<Module *> getModule(const std::string &name);

  ///
  /// @brief Load the module on the Device
  ///
  /// @param[in] mid ID of the module to load
  /// @param[in] path Path of the ELF to load on the device
  /// @param[in] dev Pointer to the device to load the module on
  ///
  /// @return Return error or the ID of the loaded module
  ErrorOr<CodeModuleID> loadOnDevice(CodeModuleID mid, const std::string &path,
                                     Device *dev);

  ///
  /// @brief Remove a module from the system
  ///
  /// @param[in] mid ID of the module to destroy
  ///
  /// @return True on successs
  bool destroyModule(CodeModuleID mid);

private:
  std::vector<std::tuple<CodeModuleID, std::unique_ptr<et_runtime::Module>>>
      module_storage_;
  CodeModuleID module_count_ = 0;
};

} // namespace et_runtime

#endif // ET_RUNTIME_MODULE_MANAGER_H
