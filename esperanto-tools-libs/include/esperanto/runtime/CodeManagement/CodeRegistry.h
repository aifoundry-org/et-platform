//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#ifndef ET_RUNTIME_CODE_REGISTRY_H
#define ET_RUNTIME_CODE_REGISTRY_H

/// @file

#include "esperanto/runtime/CodeManagement/Kernel.h"
#include "esperanto/runtime/Common/CommonTypes.h"
#include "esperanto/runtime/Support/ErrorOr.h"

#include <memory>
#include <tuple>
#include <vector>

namespace et_runtime {

class Device;
class Module;
class ModuleManager;
class Kernel;
class UberKernel;

/// @class CodeRegistry CodeRegistry.h
///
/// @brief Global registry of all ELFs and contained kernels across all
/// the system
class CodeRegistry {
public:
  /// @brief Default contructor for the registry
  CodeRegistry();

  /// @brief Return Reference to the class singleton that cores the registered
  /// code.
  static CodeRegistry &registry();

  ///
  /// @brief Register a kernel with the runtime
  ///
  /// @param[in] name  String with the name of the kernel
  /// @param[in] arg_list Vector with type of arguments this kernel can accept
  /// @param[in] elf_path Path to the ELF containing the kernel. The expectation
  /// is that the ELF contains a single kernel
  ///
  /// @return Return an error or a tuple with the the ID and a reference to the
  /// registered kernel
  ErrorOr<std::tuple<KernelCodeID, Kernel &>>
  registerKernel(const std::string &name,
                 const std::vector<Kernel::ArgType> &arg_list,
                 const std::string &elf_path);

  ///
  /// @brief Register a UberKernel with the runtime
  ///
  /// @param[in] name  String with the name of the kernel
  /// @param[in] elf_path Path to the ELF containing the kernel. The expectation
  /// is that the ELF contains a single kernel
  ///
  /// @return Return an error or a tuple with the the ID and a reference to the
  /// registered kernel
  ErrorOr<std::tuple<KernelCodeID, UberKernel &>>
  registerUberKernel(const std::string &name,
                     const std::vector<std::vector<Kernel::ArgType>> &arg_list,
                     const std::string &elf_path);

  /// @brief Get a pointer to a regisetered module
  ///
  /// @param[in] mid Id of the module
  ///
  /// @return Pointer to code module
  et_runtime::Module *getModule(CodeModuleID mid);

  /// @brief Destroy a registered module
  ///
  /// @param[in] ID of the mdoule to destroy
  void destroyModule(CodeModuleID mid);

  /// @brief Load a module to a device
  ///
  /// @param[in] mid ID of the module to load
  /// @param[in] dev Pointer to device to load the module to
  ///
  /// @return Error or ID of the loaded module that should match the passed mid
  /// value
  ErrorOr<et_runtime::CodeModuleID> moduleLoad(CodeModuleID mid, Device *dev);

  /// @brief Unload module from device
  ///
  /// @param[in] mid ID of the module to unload
  /// @param[in] dev Pointer to device to load the device
  ///
  /// @return Success of error of the action
  etrtError moduleUnload(CodeModuleID mid, Device *dev);

private:
  std::unique_ptr<ModuleManager> mod_manager_;

  ///
  /// @brief Register a UberKernel with the runtime
  ///
  /// @tparam KernelClass Class type of the kernel to create
  /// @tparam KernelArgTypes Class that holds the list of argument types the
  /// kernel accepts
  ///
  /// @param[in] name  String with the name of the kernel
  /// @param[in] elf_path Path to the ELF containing the kernel. The expectation
  /// is that the ELF contains a single kernel
  ///
  /// @return Return an error or a tuple with the the ID and a reference to the
  /// registered kernel
  template <class KernelClass, class KernelArgTypes>
  ErrorOr<std::tuple<KernelCodeID, KernelClass &>>
  registerKernelHelper(const std::string &name, const KernelArgTypes &arg_types,
                       const std::string &elf_path);
};

} // namespace et_runtime
#endif // ET_RUNTIME_CODE_REGISTRY_H
