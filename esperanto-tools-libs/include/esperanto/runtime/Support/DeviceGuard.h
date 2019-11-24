//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#ifndef ET_RUNTIME_SUPPORT_DEVICE_GUARD_H
#define ET_RUNTIME_SUPPORT_DEVICE_GUARD_H

#include <memory>

namespace et_runtime {
class DeviceManager;
class Device;
} // namespace et_runtime

/*
 * @brief Helper class to get device object and lock it in RAII manner.
 *
 * It automatically locks the device set as active through the DeviceManager
 * class.
 */
class GetDev {
public:
  GetDev();

  ~GetDev();

  et_runtime::Device *operator->() { return dev.get(); }

private:
  std::shared_ptr<et_runtime::Device> getEtDevice();

  std::shared_ptr<et_runtime::Device> dev;
};

#endif // ET_RUNTIME_SUPPORT_DEVICE_GUARD_H
