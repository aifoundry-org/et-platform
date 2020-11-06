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
#include "RPCDevice/TargetRPC.h"
#include "RPCDevice/RPCTarget_MM.h"

#include "esperanto/runtime/Common/ProjectAutogen.h"
#include "esperanto/runtime/Core/DeviceTarget.h"
#include "esperanto/runtime/Support/HelperMacros.h"
#include "esperanto/runtime/Support/Logging.h"
// FIXME TODO: Remove this!
#include "esperanto/runtime/Core/CommandLineOptions.h"

#include <esperanto-fw/firmware_helpers/layout.h>
#include <esperanto-fw/firmware_helpers/minion_esr_defines.h>
#include <esperanto-fw/firmware_helpers/minion_fw_boot_config.h>

#include <absl/flags/flag.h>
#include <memory>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>

ABSL_FLAG(std::string, master_minion_elf, MASTER_MINION_ELF,
          "Path to the MasterMinion ELF file");
ABSL_FLAG(std::string, worker_minion_elf, WORKER_MINION_ELF,
          "Path to the WorkerMinion ELF file");
ABSL_FLAG(std::string, machine_minion_elf, MACHINE_MINION_ELF,
          "Path to the MachineMinion ELF file");

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
      THROW("The expected order of DeviceFW ELF-file paths is: "
            "MasterMinion.elf, MachineMinion.elf, WorkerMinion.elf");
    }
    elf_info = std::make_unique<ELFInfo>(elf_name, path);
    elf_info->loadELF();
    RTINFO << "Loaded " << elf_info->name() << " from: " << path
           << " Load address: " << std::hex << elf_info->entryAddr()
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

etrtError DeviceFW::configureFirmware(device::DeviceTarget *dev) {
  // [SW-3998] TODO: For Zebu we will create a new Firmware class because we start booting BL2 there
  if (dev->type() != device::DeviceTarget::TargetType::SysEmuGRPC &&
      dev->type() != device::DeviceTarget::TargetType::SysEmuGRPC_VQ_MM) {
    return etrtSuccess;
  }

  // [SW-3998] TODO: This is not the proper place to put this. Create a new "BL2" Firmware sub-class
  // BL2 will write the fw_boot_config from the OTP data, so do nothing here.
  auto boot_sp = absl::GetFlag(FLAGS_sysemu_boot_sp);
  if (boot_sp) {
    return etrtSuccess;
  }

  minion_fw_boot_config_t boot_config;
  memset(&boot_config, 0, sizeof(boot_config));
  // TODO FIXME: Pass properly the shire mask instead of reading it from command line options!!!!
  boot_config.minion_shires = absl::GetFlag(FLAGS_sysemu_shires_mask);

  auto ret = dev->writeDevMemMMIO(FW_MINION_FW_BOOT_CONFIG, sizeof(boot_config),
                                  reinterpret_cast<const void *>(&boot_config));
  if (!ret) {
    return etrtErrorUnknown;
  }

  return etrtSuccess;
};

etrtError DeviceFW::bootFirmware(device::DeviceTarget *dev) {
  // TODO: Not the best place to check for this
  // On Zebu the Minions are booted directly
  if (dev->type() != device::DeviceTarget::TargetType::SysEmuGRPC &&
      dev->type() != device::DeviceTarget::TargetType::SysEmuGRPC_VQ_MM) {
    return etrtSuccess;
  }

  uint32_t shire_id, thread0_enable, thread1_enable;
  auto boot_sp = absl::GetFlag(FLAGS_sysemu_boot_sp);

  if (boot_sp) {
    // Service Processor
    shire_id = 254; // IOShire ID
    thread0_enable = 1;
    thread1_enable = 0;
  } else {
    // Device Minion Runtime FW Minions
    shire_id = MM_SHIRE_ID;
    thread0_enable = MM_RT_THREADS;
    thread1_enable = 0;
  }

  if (dev->type() == device::DeviceTarget::TargetType::SysEmuGRPC) {
    auto *rpc_target = dynamic_cast<device::RPCTarget *>(dev);
    auto ret = rpc_target->rpcBootShire(shire_id, thread0_enable, thread1_enable);
    if (!ret) {
      return etrtErrorUnknown;
    }
  } else {
    auto *rpc_target = dynamic_cast<device::RPCTargetMM *>(dev);
    auto ret = rpc_target->boot(MM_SHIRE_ID, MM_RT_THREADS, 0);
    if (!ret) {
      return etrtErrorUnknown;
    }
  }

  return etrtSuccess;
};

} // namespace device_fw
} // namespace et_runtime
