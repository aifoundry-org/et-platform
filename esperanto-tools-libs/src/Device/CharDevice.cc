//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "CharDevice.h"

#include "Support/Logging.h"

using namespace std;

namespace et_runtime {
namespace device {

CharacterDevice::CharacterDevice(
    const std::experimental::filesystem::path &char_dev)
    : path_(char_dev), device_(char_dev.string(), ios_base::in | ios_base::out |
                                                      ios_base::binary) {

  std::ios_base::iostate exceptionMask =
      device_.exceptions() | std::ios::failbit;
  device_.exceptions(exceptionMask);
}

CharacterDevice::CharacterDevice(CharacterDevice &&other)
    : device_(std::move(other.device_)) {}

bool CharacterDevice::write(uintptr_t addr, const void *data, size_t size) {
  try {
    device_.seekg(addr);
  } catch (std::ios_base::failure &e) {
    RTERROR << "Failed to seek to device_offset " << e.what();
    return false;
  }
  device_.write(reinterpret_cast<const char *>(data), size);
  if (device_.fail() || device_.bad()) {
    RTERROR << "Failed to write to device " << path_;
    device_.clear();
    return false;
  }
  return true;
}

} // namespace device
} // namespace et_runtime
