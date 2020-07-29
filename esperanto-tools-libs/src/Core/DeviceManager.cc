//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "esperanto/runtime/Core/DeviceManager.h"
#include "CoreState.h"
#include "PCIEDevice/PCIeDevice.h"
#include "esperanto/runtime/Common/CommonTypes.h"
#include "esperanto/runtime/Core/Device.h"
#include "esperanto/runtime/Support/Logging.h"

#include "esperanto/runtime/etrt-bin.h"

#include <cstring>
#include <memory>

using namespace std;

/*
 * @brief Initalization values for an empty Device properties struct
 */
static const struct et_runtime::etrtDeviceProp etrtDevicePropDontCare = {
    {'\0'}, /* char   name[256];               */
};          /**< Empty device properties */

namespace et_runtime {

DeviceManager::DeviceManager() : active_device_(0), devices_(MAX_DEVICE_NUM) {}

shared_ptr<DeviceManager> getDeviceManager() {
  auto &core_state = getCoreState();
  auto &deviceManager = core_state.dev_manager;

  if (!deviceManager) {
    deviceManager = make_shared<DeviceManager>();
  }
  return deviceManager;
}

ErrorOr<std::vector<DeviceInformation>> DeviceManager::enumerateDevices() {
  std::vector<DeviceInformation> info;
  switch (device::DeviceTarget::deviceToCreate()) {
  case device::DeviceTarget::TargetType::PCIe:
    return device::PCIeDevice::enumerateDevices();
    break;
  default:
    RTERROR << "Device enumeration is supported only for PCIE type devices \n";
    abort();
  }
  return etrtErrorDevicesUnavailable;
}

ErrorOr<int> DeviceManager::getDeviceCount() { return devices_.size(); }

ErrorOr<DeviceInformation> DeviceManager::getDeviceInformation(int device) {
  // FIXME SW-256
  assert(device == 0);
  auto prop = etrtDevicePropDontCare;
  return prop;
}

ErrorOr<std::shared_ptr<Device>> DeviceManager::registerDevice(int device) {
  // FIXME Implement the real registration functionality that will depend on the
  // target device.
  if (static_cast<decltype(devices_)::size_type>(device) > devices_.size()) {
    return etrtErrorInvalidDevice;
  }

  devices_[device] = make_shared<Device>(device);

  return devices_[device];
}

ErrorOr<std::shared_ptr<Device>>
DeviceManager::getRegisteredDevice(int device) const {
  if (static_cast<decltype(devices_)::size_type>(device) > devices_.size()) {
    return etrtErrorInvalidDevice;
  }

  if (devices_[device].get() == nullptr) {
    return etrtErrorInvalidDevice;
  }

  return devices_[device];
}

ErrorOr<std::shared_ptr<Device>> DeviceManager::getActiveDevice() const {
  if (active_device_ > etrtErrorInvalidDevice) {
    return etrtErrorInvalidDevice;
  }
  return devices_[active_device_];
}

const char *DeviceManager::getDriverVersion() const {
  // FIXME Add support for queriing the device
  return "";
}

} // namespace et_runtime
