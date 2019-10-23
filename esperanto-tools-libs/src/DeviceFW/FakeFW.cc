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

#include "Core/CommandLineOptions.h"
#include "Core/DeviceTarget.h"

#include <fstream>

ABSL_FLAG(std::string, fake_fw, "", "Path to the fake-fw path");

namespace et_runtime {
namespace device_fw {

FakeFW::FakeFW() { setFWFilePaths({absl::GetFlag(FLAGS_fake_fw)}); }

bool FakeFW::setFWFilePaths(const std::vector<std::string> &paths) {
  if (paths.size() > 1) {
    RTERROR << "FakeFW expects a single path";
    abort();
  }
  firmware_path_ = paths[0];

  std::ifstream input(firmware_path_.c_str(), std::ios::binary);
  // Read the input file in binary-form in the holder buffer.
  // Use the stread iterator to read out all the file data
  firmware_data_ =
      decltype(firmware_data_)(std::istreambuf_iterator<char>(input), {});

  return true;
}
bool FakeFW::readFW() { return true; }

etrtError FakeFW::loadOnDevice(device::DeviceTarget *dev) {
  const void *firmware_p = reinterpret_cast<const void *>(firmware_data_.data());
  size_t firmware_file_size = firmware_data_.size();

  // TODO: Currently when running with Fake-FW, the firmware is loaded in
  // the sys_emu's mem_desc.txt (check EMemoryMap::dumpMemoryDescriptor)
  dev->writeDevMemMMIO(FIRMWARE_LOAD_ADDR, firmware_file_size, firmware_p);

  dev->boot(FIRMWARE_LOAD_ADDR); // TODO Where to place this?

  return etrtSuccess;
}

} // namespace device_fw
} // namespace et_runtime
