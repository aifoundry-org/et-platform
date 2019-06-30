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

#include "Core/Device.h"
#include "DeviceAPI/Commands.h"
#include "ELFSupport.h"

#include <cassert>

using namespace std;
using namespace et_runtime;

Module::Module(ModuleID mid, const std::string &name)
    : module_id_(mid), elf_info_(make_unique<KernelELFInfo>(name)) {}

bool Module::readELF(const std::string path) {
  std::ifstream f(path, std::ios::binary);
  auto stream_it = std::istreambuf_iterator<char>{f};
  elf_raw_data_.insert(elf_raw_data_.begin(), stream_it, {} /*end iterator*/);

  elf_info_->loadELF(path);

  assert(elf_info_->elfSize() <= elf_raw_data_.size());

  return true;
}

const std::string &Module::name() const { return elf_info_->name(); };

bool Module::rawKernelExists(const std::string &name) {
  return elf_info_->rawKernelExists(name);
}

size_t Module::rawKernelOffset(const std::string &name) {
  return elf_info_->rawKernelOffset(name);
}

/// @Brief Load the ELF on the device
bool Module::loadOnDevice(Device *dev) {
  dev->malloc((void **)&devPtr_, elf_raw_data_.size());

  dev->addAction(
      dev->defaultStream(),
      std::shared_ptr<EtAction>(new EtActionWrite(
          (void *)devPtr_, elf_raw_data_.data(), elf_raw_data_.size())));

  assert(actionEvent_ == nullptr);
  actionEvent_ = std::shared_ptr<EtAction>(new EtActionEvent());
  dev->addAction(dev->defaultStream(), actionEvent_);

  // synchronize the default stream
  dev->streamSynchronize(nullptr);

  assert(devPtr_ != 0);
  assert(actionEvent_ != nullptr);
  auto event = dynamic_cast<EtActionEvent *>(actionEvent_.get());
  assert(event->isExecuted());
  actionEvent_ = nullptr;

  onDevice_ = true;
  return true;
}

ErrorOr<uintptr_t>
Module::onDeviceKernelEntryPoint(const std::string &kernel_name) {
  if (!onDevice_) {
    return etrtErrorModuleNotOnDevice;
  }
  return (uintptr_t)devPtr_ + rawKernelOffset(kernel_name);
}
