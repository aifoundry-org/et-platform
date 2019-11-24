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
/// @brief Helper class for checking the device-firmware commit
//
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

///
/// @brief Helper class that checks the the DeviceAPI version on the Device
//
class DeviceAPIChecker {

public:
  DeviceAPIChecker(Device &dev);

  /// @brief Query the MasterMinion and pull the DeviceAPI version that it
  /// supports
  bool getDeviceAPIVersion();

  /// @brief Return true if the target version of the DeviceAPI on the Device is
  /// supported
  //// by the runtime.
  bool isDeviceSupported();

private:
  bool deviceQueried_ = false; ///< True if the device has been already queried
  Device &dev_;                ///< Reference to the Device
  // Avoid including the device-api struct here to avoid leaking this
  // implementation in this public header
  uint64_t mmFWDevAPIMajor_ =
      0; ///< Major Version of the DeviceAPI on the device
  uint64_t mmFWDevAPIMinor_ =
      0; ///< Minor Version of the DeviceAPI on the device
  uint64_t mmFWDevAPIPatch_ =
      0; ///< Patch Version of the DeviceAPI on the device
  uint64_t mmFWDevAPIHash_ = 0; ///< Hash of the DeviceAPI schema on the device
  bool mmFWAccept_ = false; ///< Accept reply of the DeviceAPI version that the
                            ///< runtime supports from the master minion
};

} // namespace et_runtime

#endif // ET_RUNTIME_VERSION_CHECKERS_H
