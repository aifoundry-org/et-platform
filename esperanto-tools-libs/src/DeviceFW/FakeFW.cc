//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "FakeFW.h"

#include "Support/Logging.h"

#include "Core/DeviceTarget.h"

#include <fstream>

namespace et_runtime {
namespace device_fw {

bool FakeFW::setFWFilePaths(const std::vector<std::string> &paths) {
  if (paths.size() > 1) {
    RTERROR << "FakeFW expects a single path";
    abort();
  }
  bootrom_path_ = paths[0];

  return true;
}
bool FakeFW::readFW() {
  std::ifstream input(bootrom_path_.c_str(), std::ios::binary);
  // Read the input file in binary-form in the holder buffer.
  // Use the stread iterator to read out all the file data
  bootrom_data_ =
      decltype(bootrom_data_)(std::istreambuf_iterator<char>(input), {});
  return true;
}

std::vector<device::MemoryRegionConf> FakeFW::memoryRegionConfigurations() {
  size_t bootrom_file_size = bootrom_data_.size();

  return {
      {LAUNCH_PARAMS_AREA_BASE, LAUNCH_PARAMS_AREA_SIZE, false},
      {BLOCK_SHARED_REGION, BLOCK_SHARED_REGION_TOTAL_SIZE, false},
      {STACK_REGION, STACK_REGION_TOTAL_SIZE << 3, false},
      {0x8000100000, 0x100000, false},
      {0x8000300000, 0x108000, false},
      {0x8000408000, 0x108000, false},
      {0x8200000000, 64, false},
      {0x8000600000, 64, false},
      {BOOTROM_START_IP, align_up(bootrom_file_size, 0x1000), true},
  };
};

etrtError FakeFW::loadOnDevice(device::DeviceTarget *dev) {
  const void *bootrom_p = reinterpret_cast<const void *>(bootrom_data_.data());
  size_t bootrom_file_size = bootrom_data_.size();

  dev->writeDevMemMMIO(BOOTROM_START_IP, bootrom_file_size, bootrom_p);

  // FIXME deprecate the following eventually
#if 1

  static const long ETSOC_init = 0x000000810000600c;
  static const long ETSOC_mtrap = 0x0000008100007000;

  {
    struct BootromInitDescr_t {
      uint64_t init_pc;
      uint64_t trap_pc;
    } descr;

    descr.init_pc = ETSOC_init;
    descr.trap_pc = ETSOC_mtrap;

    dev->writeDevMemMMIO(LAUNCH_PARAMS_AREA_BASE, sizeof(descr), &descr);
  }

  dev->boot(ETSOC_init, ETSOC_mtrap);
#else
  dev->boot(0, 0);
#endif

  return etrtSuccess;
}

} // namespace device_fw
} // namespace et_runtime
