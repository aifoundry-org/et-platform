//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "DeviceFW.h"

#include "CodeManagement/ELFSupport.h"
#include "Core/DeviceTarget.h"
#include "Support/HelperMacros.h"
#include "Support/Logging.h"

#include <absl/flags/flag.h>
#include <memory>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>

ABSL_FLAG(std::string, master_minion_elf, MASTER_MINION_ELF,
          "Path to the MasterMiniion.elf file");
ABSL_FLAG(std::string, worker_minion_elf, WORKER_MINION_ELF,
          "Path to the WorkerMiniion.elf file");
ABSL_FLAG(std::string, machine_minion_elf, MACHINE_MINION_ELF,
          "Path to the MachineMiniion.elf file");

namespace et_runtime {
namespace device_fw {

DeviceFW::DeviceFW()
{
  auto worker_minion = absl::GetFlag(FLAGS_worker_minion_elf);
  auto machine_minion = absl::GetFlag(FLAGS_machine_minion_elf);
  auto master_minion = absl::GetFlag(FLAGS_master_minion_elf);
  this->setFWFilePaths({master_minion, machine_minion, worker_minion});
}

bool DeviceFW::setFWFilePaths(const std::vector<std::string> &paths) {
  // Expect that the order the paths are going to be given is the one bellow
  std::vector<std::tuple<std::string, std::unique_ptr<ELFInfo> &>> images = {
      {"MachineMinion.elf", machine_minion_},
      {"MasterMinion.elf", master_minion_},
      {"WorkerMinion.elf", worker_minion_},
  };
  std::remove_reference<decltype(paths)>::type::size_type i = 0;
  for (auto &[elf_name, elf_info] : images) {
    auto path = paths[i];
    if (path.find_last_not_of(elf_name) == std::string::npos) {
      THROW("The Expected order of DeviceFW ELF-file paths is: "
            "MasterMinion.elf, MachineMinion.elf, WorkerMinion.elf");
    }
    elf_info = std::make_unique<ELFInfo>(elf_name);
    elf_info->loadELF(path);
    RTINFO << "Loaded " << elf_info->name() << " from: " << path
           << " Load address: " << std::hex << elf_info->loadAddr()
           << " size: " << elf_info->elfSize();
    i++;
  }
  return true;
}

bool DeviceFW::readFW() { return true; }

etrtError DeviceFW::loadOnDevice(device::DeviceTarget *dev) {
  std::vector<decltype(master_minion_) *> elfs = {
      &master_minion_, &machine_minion_, &worker_minion_};
  for (auto &elf : elfs) {
    // Copy over the all LOAD segments specified in the EL0F
    for (auto &segment : (*elf)->reader_.segments) {
      auto type = segment->get_type();
      if (type & PT_LOAD) {
        auto offset = segment->get_offset();
        auto load_address = segment->get_physical_address();
        auto file_size = segment->get_file_size();
        auto mem_size = segment->get_memory_size();

        RTDEBUG << "Found segment: " << segment->get_index()
                << " Offset " << std::hex << offset
                << " Physical Address: 0x" << std::hex
                << load_address << " File Size: 0x"
                << file_size << " Mem Size : 0x" << mem_size << "\n";

        auto &elf_data = (*elf)->data();
        const void* data_ptr = reinterpret_cast<const void *>(elf_data.data() + offset);
        dev->writeDevMemMMIO(load_address, mem_size,
                         data_ptr);
      }
    }
  }

  return etrtSuccess;
};

} // namespace device_fw
} // namespace et_runtime
