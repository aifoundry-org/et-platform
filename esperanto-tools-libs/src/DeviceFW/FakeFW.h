//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#ifndef ET_RUNTIME_DEVICEFW_FAKEFW_H_
#define ET_RUNTIME_DEVICEFW_FAKEFW_H_

#include "FWManager.h"

// FIXME fix include path of this file
#include "../kernels/sys_inc.h"
#include "Support/STLHelpers.h"
#include <vector>

namespace et_runtime {
namespace device_fw {

/// @brief Class holding the information of the Fake-FW
class FakeFW final : public Firmware {
public:
  FakeFW();
  bool setFWFilePaths(const std::vector<std::string> &paths) override;
  bool readFW() override;
  etrtError loadOnDevice(device::DeviceTarget *dev) override;

private:
  std::string firmware_path_;                ///< Path to the firmware file
  std::vector<unsigned char> firmware_data_; ///< Data of the firmware file
};
} // namespace device_fw
} // namespace et_runtime

#endif // ET_RUNTIME_DEVICEFW_FAKEFW_H_
