//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "Core/CodeModule.h"

#include "ELFSupport.h"

#include <cassert>

using namespace std;
using namespace et_runtime;

Module::Module(const std::string &name)
    : elf_info_(make_unique<KernelELFInfo>(name)) {}

bool Module::loadELF(const std::string path) {
  std::ifstream f(path, std::ios::binary);
  auto stream_it = std::istreambuf_iterator<char>{f};
  elf_raw_data_.insert(elf_raw_data_.begin(), stream_it, {} /*end iterator*/);

  elf_info_->loadELF(path);

  assert(elf_info_->elfSize() <= elf_raw_data_.size());

  return true;
}

bool Module::rawKernelExists(const std::string &name) {
  return elf_info_->rawKernelExists(name);
}

size_t Module::rawKernelOffset(const std::string &name) {
  return elf_info_->rawKernelOffset(name);
}
