//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "ModuleManager.h"

#include "CodeModule.h"
#include "ELFSupport.h"

#include <algorithm>
#include <tuple>

namespace et_runtime {
std::tuple<CodeModuleID, Module &>
ModuleManager::createModule(const std::string &name) {
  auto elem = std::find_if(module_storage_.begin(), module_storage_.end(),
                           [&name](decltype(module_storage_)::value_type &e) {
                             return std::get<1>(e)->name() == name;
                           });
  if (elem != module_storage_.end()) {
    auto &[id, ptr] = *elem;
    return {id, *ptr.get()};
  }

  module_count_++;
  auto new_module = std::make_unique<Module>(module_count_, name);

  auto val = std::move(std::make_tuple(module_count_, std::move(new_module)));
  module_storage_.emplace_back(std::move(val));
  return {module_count_, *new_module};
}

ErrorOr<Module *> ModuleManager::getModule(CodeModuleID mid) {

  auto elem = std::find_if(module_storage_.begin(), module_storage_.end(),
                           [mid](decltype(module_storage_)::value_type &e) {
                             return std::get<0>(e) == mid;
                           });
  if (elem == module_storage_.end()) {
    return etrtErrorRuntime;
  }
  return std::get<1>(*elem).get();
}

ErrorOr<Module *> ModuleManager::getModule(const std::string &name) {

  auto elem = std::find_if(module_storage_.begin(), module_storage_.end(),
                           [name](decltype(module_storage_)::value_type &e) {
                             return std::get<1>(e)->name() == name;
                           });
  if (elem == module_storage_.end()) {
    return etrtErrorRuntime;
  }
  return std::get<1>(*elem).get();
}

ErrorOr<CodeModuleID> ModuleManager::loadOnDevice(CodeModuleID mid,
                                                  const std::string &path,
                                                  Device *dev) {
  auto find_res = getModule(mid);
  if (!find_res) {
    return find_res.getError();
  }
  auto module = find_res.get();
  if (module->ELFRead()) {
    return etrtErrorModuleELFDataExists;
  }
  auto status = module->readELF(path);
  assert(status == true);
  status = module->loadOnDevice(dev);
  if (!status) {
    return etrtErrorModuleFailedToLoadOnDevice;
  }
  return mid;
}

bool ModuleManager::destroyModule(CodeModuleID mid) {
  module_storage_.erase(
      std::remove_if(module_storage_.begin(), module_storage_.end(),
                     [mid](const decltype(module_storage_)::value_type &e) {
                       return std::get<0>(e) == mid;
                     }),
      module_storage_.end());
  /// SW-1370
  // FIXME we should be de-allocating memory from the device when we delete a
  // loaded module and free Code-Space
  return true;
}

} // namespace et_runtime
