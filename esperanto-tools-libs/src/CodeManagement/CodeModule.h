//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#ifndef ET_RUNTIME_CODE_MODULE_H
#define ET_RUNTIME_CODE_MODULE_H

#include "esperanto/runtime/Common/CommonTypes.h"
#include "esperanto/runtime/Core/Memory.h"
#include "esperanto/runtime/Support/ErrorOr.h"
#include "esperanto/runtime/Support/MemoryRange.h"

#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace et_runtime {

// Forward declaration of the kernel-elf-info
class KernelELFInfo;
class Device;
class EtAction;

/// @brief Dynamically loaded module descriptor.
class Module {
public:
  Module(const std::string &name, const std::string &path);

  /// @return ID of this module
  CodeModuleID id() const { return module_id_; }

  /// @brief Load the ELF file in path in host memory
  bool readELF();

  /// @brief Return true iff we have read and decoded the elf file
  const bool ELFRead();

  /// @brief Return the nanme of the module.
  const std::string &name() const;

  /// @brief Return the elf load address
  uintptr_t elfEntryAddr() const;

  /// @brief return true iff the "raw" kernel exists in the module
  bool rawKernelExists(const std::string &name);

  /// @brief Retrn the offfset in the ELF of the raw kernel
  size_t rawKernelOffset(const std::string &name);

  /// @brief Load the ELF on the device
  bool loadOnDevice(Device *dev);

  /// @brief True iff the module is loaded on the device
  bool onDevice() { return device_buffer_.id() != 0; }

  /// @brief PC where the kernel entrypoint is located
  ErrorOr<uintptr_t> onDeviceKernelEntryPoint(const std::string &kernel_name);

private:
  static CodeModuleID
      module_count_; ///< Static count of all modules in the system

  CodeModuleID module_id_;                  ///< ID of this module
  std::unique_ptr<KernelELFInfo> elf_info_; ///< Pointer to the decoded ELF
  DeviceBuffer device_buffer_; ///< Buffer on the device that holds the ELF data
  std::vector<std::tuple<support::MemoryRange, uintptr_t>>
      device_remap_; ///< Mapping of the ELF segment load address ( and size) to
                     ///< the load address of the segment on the device. To be
                     ///< used in order to find the correct base address to use
                     ///< for the kernel lauch address.
  std::shared_ptr<et_runtime::EtAction> actionEvent_ =
      nullptr; ///<  Action used for synchronize with load completion on the
               ///<  device
};

} // namespace et_runtime

#endif // ET_RUNTIME_CODE_MODULE_H
