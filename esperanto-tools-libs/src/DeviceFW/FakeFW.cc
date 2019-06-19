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

etrtError FakeFW::loadOnDevice(device::DeviceTarget *dev) {
  const void *bootrom_p = reinterpret_cast<const void *>(bootrom_data_.data());
  size_t bootrom_file_size = bootrom_data_.size();

  dev->defineDevMem(BOOTROM_START_IP, align_up(bootrom_file_size, 0x1000),
                    true);
  dev->writeDevMem(BOOTROM_START_IP, bootrom_file_size, bootrom_p);
  return etrtSuccess;
}

} // namespace device_fw
} // namespace et_runtime
