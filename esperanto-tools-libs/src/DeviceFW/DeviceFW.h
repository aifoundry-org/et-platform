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
#include <memory>

namespace et_runtime {

class ELFInfo;

namespace device_fw {

/// @brief Class holding the information of the real DeviceFW
class DeviceFW final : public Firmware {
public:
  DeviceFW();

  bool setFWFilePaths(const std::vector<std::string> &paths) override;
  bool readFW() override;
  etrtError loadOnDevice(device::DeviceTarget *dev) override;
  etrtError configureFirmware(device::DeviceTarget *dev) override;
  etrtError bootFirmware(device::DeviceTarget *dev) override;

private:
  std::unique_ptr<ELFInfo> master_minion_, machine_minion_, worker_minion_;
};
} // namespace device_fw
} // namespace et_runtime

#endif // ET_RUNTIME_DEVICEFW_DEVICEFW_H
