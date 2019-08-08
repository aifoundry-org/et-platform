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
#include <elfio/elfio.hpp>

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

uintptr_t Module::elfLoadAddr() const { return elf_info_->loadAddr(); }

bool Module::rawKernelExists(const std::string &name) {
  return elf_info_->rawKernelExists(name);
}

size_t Module::rawKernelOffset(const std::string &name) {
  return elf_info_->rawKernelOffset(name);
}

/// @Brief Load the ELF on the device
bool Module::loadOnDevice(Device *dev) {
  auto &mem_manager = dev->mem_manager_;

  // Check if the elf has been compiled with absolute SOC DRAM offsets or not
  assert(elf_info_->reader_.segments.size() > 0);
  auto first_segment = elf_info_->reader_.segments[0];
  uintptr_t dev_base_addr = 0;
  // If compiled as PIC (i.e. without absolute SOC addresses) then allocate a
  // buffer the size of the ELF file
  if (first_segment->get_physical_address() < mem_manager->ramBase()) {
    mem_manager->malloc((void **)&dev_base_addr, elf_info_->elfSize());
    RTDEBUG << "Allocating memory for PIC ELF, Addr: 0x" << std::hex
            << dev_base_addr << "\n";
  }
  // Copy over the all LOAD segments specified in the EL0F
  for (auto &segment : elf_info_->reader_.segments) {
    auto type = segment->get_type();
    if (type & PT_LOAD) {
      auto offset = segment->get_offset();
      auto load_address = segment->get_physical_address();
      auto file_size = segment->get_file_size();
      auto mem_size = segment->get_memory_size();
      uintptr_t devPtr = 0;

      RTDEBUG << "Found segment: " << segment->get_index()
              << " Physical Address: 0x" << std::hex
              << segment->get_physical_address() << " File Size: 0x"
              << file_size << " Mem Size : 0x" << mem_size << "\n";

      // FIXME for any segment that has as a load address below the RAM base
      // then allocate a buffer and "relocate" it there. Still our ELFs are not
      // PIE and the force them to load to a specific address
      uintptr_t write_address = 0;
      if (dev_base_addr > 0) {
        write_address = dev_base_addr + offset;
        device_remap_.push_back({{load_address, mem_size}, dev_base_addr});
      } else {
        // We are loading the segment its specified the LOAD address
        // Set devPtr to zero as the functions have entrypoint addresses
        // that are absolute
        devPtr = load_address;
        auto res = mem_manager->reserveMemory(reinterpret_cast<void *>(devPtr),
                                              mem_size);
        assert(res == etrtSuccess);
        write_address = devPtr;
      }

      RTDEBUG << "Loading segment: " << segment->get_index()
              << " Physical Address: 0x" << std::hex << write_address
              << " Mem Size : 0x" << mem_size << "\n";

      auto write_command = make_shared<device_api::WriteCommand>(
          (void *)write_address, elf_raw_data_.data() + offset, mem_size);

      dev->addCommand(
          dev->defaultStream(),
          std::dynamic_pointer_cast<device_api::CommandBase>(write_command));

      auto response_future = write_command->getFuture();
      auto response = response_future.get();
      if (response.error() != etrtSuccess) {
        return false;
      }
      onDevice_ = true;
    }
  }
  return true;
}

ErrorOr<uintptr_t>
Module::onDeviceKernelEntryPoint(const std::string &kernel_name) {
  if (!onDevice_) {
    return etrtErrorModuleNotOnDevice;
  }
  auto kernel_offset = rawKernelOffset(kernel_name);

  if (device_remap_.size() > 0) {
    for (auto &[elf_mem_range, dev_addr] : device_remap_) {
      if (elf_mem_range.addr_ < kernel_offset &&
          kernel_offset < elf_mem_range.end()) {
        return dev_addr + kernel_offset;
      }
    }
    RTERROR << "For kernel " << kernel_name << " found elf offset: " << std::hex
            << kernel_offset << "but no containing rellocated ELF segment \n";
    abort();
    return -1;
  }
  return kernel_offset;
}
