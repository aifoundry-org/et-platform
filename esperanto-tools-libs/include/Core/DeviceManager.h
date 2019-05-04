//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#ifndef ET_RUNTIME_DEVICE_MANAGER_H
#define ET_RUNTIME_DEVICE_MANAGER_H

#include "Core/DeviceInformation.h"
#include "Support/ErrorOr.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace et_runtime {

class Device;
class DeviceInformation;

/// \brief Esperanto Device Manager class.
///
/// The Device Manager is the entrypoint class to the rest of the system.
/// It provides the functionality to enumerate the devices, report their
/// status as described by the PCIe driver and register the runtime with a
/// specific device.
class DeviceManager {

public:
  DeviceManager();

  ///
  /// @brief  Return the number of Devices found on this Host.
  ///
  /// Returns a count of the number of Devices currently attached and accessible
  /// on the Host.
  /// @return etrtSuccess as the error-code and the number of devices.
  ///
  ErrorOr<int> getDeviceCount();

  ///
  /// @brief  Return information about a given Device
  ///
  /// Takes the ordinal representing one of the Host's currently attached and
  /// accessible Devices and returns a structure with information about the
  /// associated Device in the location given by the pointer argument.
  /// See @ref DeviceInformation for details about the information returned.
  ///
  /// @param[in]  device  An ordinal that refers to the (local) Device that is
  /// to be queried.
  /// @return  etrtErrorInvalidDevice or struct @ref DeviceInformation
  ///
  ErrorOr<DeviceInformation> getDeviceInformation(int device);

  ///
  /// @brief Register the current process with a specific device
  ///
  /// @param[in] device Ordinal to the device we want to register the current
  /// process with
  /// @return Error-code or pointer to @ref Device class representing the
  /// device we registered with and its associated state. Errors
  /// etrtErrorInvalidDevice;
  ///
  ErrorOr<std::shared_ptr<Device>> registerDevice(int device);

  ///
  /// @brief get pointer to \ref Device class associated with a specific device
  ///
  /// @param[in] device Ordinal of the device
  ///
  /// @return Error-code or pointer to @ref Device object holding the device's
  /// state.
  ErrorOr<std::shared_ptr<Device>> getRegisteredDevice(int device) const;

  ///
  /// @brief  Return the version of the ET Device Driver that is being used.
  ///
  /// Returns a string containing the (SemVer 2.0) version number for the ET
  /// Device Driver that is currently being used.
  ///
  /// @return  String of the driver version.
  ///
  const char *getDriverVersion() const;

  /// @brief Get the device the c-api is actively interacting with.
  ///
  /// @return Error or shared pointer
  ErrorOr<std::shared_ptr<Device>> getActiveDevice() const;

  /// @brief Set the device c-api is actively interacting with.
  ///
  /// @param[in] device  Index of active device to set. The
  /// @return etrtErrorInvalidDevice if this is not a valid device index.
  etrtError setActiveDevice(int device) {
    if (static_cast<decltype(devices_)::size_type>(device) > devices_.size()) {
      return etrtErrorInvalidDevice;
    }
    active_device_ = device;
    return etrtSuccess;
  }

  /// @brief Get the device the c-api is actively interacting with.
  ///
  /// @return Integer with the ID of the current active device
  ErrorOr<int> getActiveDeviceID() const {
    if (active_device_ < 0) {
      return etrtErrorInvalidDevice;
    }
    return active_device_;
  }

private:
  static const int MAX_DEVICE_NUM = 6;
  static std::string bootrom_path;
  int active_device_ = -1;
  std::vector<std::shared_ptr<Device>> devices_;
};

/// @brief Return pointer to the DeviceManager
///
/// Avoid creating static objects as the constructor call order is not
/// guaranteed. Wrap the static object in a function that returns a pointer
/// to it.
__attribute__((visibility("default"))) std::shared_ptr<DeviceManager>
getDeviceManager();

}; // namespace et_runtime

#endif // ET_RUNTIME_DEVICE_MANAGER_H
