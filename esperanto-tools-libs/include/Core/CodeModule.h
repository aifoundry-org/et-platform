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

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace et_runtime {

// Forward declaration of the kernel-elf-info
class KernelELFInfo;

/// @brief Dynamically loaded module descriptor.
class Module {
  // FIXME provide a proper interface to the module members
public:
  Module(const std::string &name);

  bool loadELF(const std::string path);
  const std::vector<char> &elfRawData() { return elf_raw_data_; }

  bool rawKernelExists(const std::string &name);
  size_t rawKernelOffset(const std::string &name);

private:
  std::unique_ptr<KernelELFInfo> elf_info_;
  std::vector<char> elf_raw_data_;
};

} // namespace et_runtime

#endif // ET_RUNTIME_CODE_MODULE_H
