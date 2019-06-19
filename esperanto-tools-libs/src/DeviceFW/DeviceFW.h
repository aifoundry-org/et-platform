//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#ifndef ET_RUNTIME_DEVICEFW_DEVICEFW_H
#define ET_RUNTIME_DEVICEFW_DEVICEFW_H

#include "FWManager.h"

#include <cstdlib>

namespace et_runtime {
namespace device_fw {

/// @brief Class holding the information of the real DeviceFW
class DeviceFW final : public Firmware {
public:
  DeviceFW() = default;
  bool setFWFilePaths(const std::vector<std::string> &paths) override {
    abort();
    return true;
  }
  bool readFW() override {
    abort();
    return true;
  }
  etrtError loadOnDevice(device::DeviceTarget *dev) override {
    abort();
    return etrtSuccess;
  };
};
} // namespace device_fw
} // namespace et_runtime

#endif // ET_RUNTIME_DEVICEFW_DEVICEFW_H
