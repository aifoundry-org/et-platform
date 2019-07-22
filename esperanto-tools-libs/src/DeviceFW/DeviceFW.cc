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

#include "Core/DeviceTarget.h"
#include "Core/ELFSupport.h"
#include "Support/HelperMacros.h"
#include "Support/Logging.h"

#include <absl/flags/flag.h>
#include <memory>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>

ABSL_FLAG(std::string, master_minion_elf, "",
          "Path to the MasterMiniion.elf file");
ABSL_FLAG(std::string, worker_minion_elf, "",
          "Path to the WorkerMiniion.elf file");
ABSL_FLAG(std::string, machine_minion_elf, "",
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

std::vector<device::MemoryRegionConf> DeviceFW::memoryRegionConfigurations() {
  return {
      // Machine Minion region
      {/*R_L3_MCODE_BASEADDR*/ 0x8000000000, /*R_L3_MCODE_SIZE*/ 0x200000,
       false},
      // Lower 16MB are allocated by device fw
      {/*R_L3_DRAM_BASEADDR*/ 0x8100000000, /*16 MB */ 0x0001000000, false},
  };
};

etrtError DeviceFW::loadOnDevice(device::DeviceTarget *dev) {
  std::vector<decltype(master_minion_) *> elfs = {
      &master_minion_, &machine_minion_, &worker_minion_};
  for (auto &elf : elfs) {
    auto &elf_data = (*elf)->data();
    const auto addr = (*elf)->loadAddr();
    dev->writeDevMem(addr, elf_data.size(),
                     reinterpret_cast<const void *>(elf_data.data()));
  }

  return etrtSuccess;
};

} // namespace device_fw
} // namespace et_runtime
