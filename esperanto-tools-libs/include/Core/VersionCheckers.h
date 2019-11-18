//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#ifndef ET_RUNTIME_VERSION_CHECKERS_H
#define ET_RUNTIME_VERSION_CHECKERS_H

#include <cstdint>

namespace et_runtime {

class Device;

///
/// @brief Add support for checking the device-firmware commit
class GitVersionChecker {

public:
  GitVersionChecker(Device &dev);

  ///
  /// @brief Return the hash of the device_fw we are interacting with
  ///
  /// Query the device for 8 bytes of the git hash of the device-fw. If
  /// we have not queried the device before we are going to send a DeviceAPI
  /// command and block until the device replies. Otherwise return the cached
  /// version.
  uint64_t deviceFWHash();

private:
  Device &dev_;             ///< Reference to the Device
  uint64_t
      runtime_sw_commit_; ///< The first 8 bytes of the git hash to adverstise
  ///< to the device
  uint64_t device_fw_commit_ = 0; ///< The first 8 bytes of the device-fw commit
};

} // namespace et_runtime

#endif // ET_RUNTIME_VERSION_CHECKERS_H
