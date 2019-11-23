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

#include "esperanto/runtime/Common/CommonTypes.h"
#include "esperanto/runtime/Support/ErrorOr.h"

#include <memory>
#include <tuple>

namespace et_runtime {

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
  /// @param[in] elf_path Path to the ELF containing the kernel. The expectation
  /// is that the ELF contains a single kernel
  ///
  /// @return Return an error or a tuple with the the ID and a reference to the
  /// registered kernel
  ErrorOr<std::tuple<KernelCodeID, Kernel &>>
  registerKernel(const std::string &name, const std::string &elf_path);

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
  registerUberKernel(const std::string &name, const std::string &elf_path);

private:
  std::unique_ptr<ModuleManager> mod_manager_;

  ///
  /// @brief Register a UberKernel with the runtime
  ///
  /// @tparam KernelClass Class type of the kernel to create
  ///
  /// @param[in] name  String with the name of the kernel
  /// @param[in] elf_path Path to the ELF containing the kernel. The expectation
  /// is that the ELF contains a single kernel
  ///
  /// @return Return an error or a tuple with the the ID and a reference to the
  /// registered kernel
  template <class KernelClass>
  ErrorOr<std::tuple<KernelCodeID, KernelClass &>>
  registerKernelHelper(const std::string &name, const std::string &elf_path);
};

} // namespace et_runtime
#endif // ET_RUNTIME_CODE_REGISTRY_H
