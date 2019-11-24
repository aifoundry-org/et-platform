//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#ifndef ET_RUNTIME_DEVICEFW_FWMANAGER_H
#define ET_RUNTIME_DEVICEFW_FWMANAGER_H

#include "esperanto/runtime/Common/ErrorTypes.h"

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace et_runtime {

namespace device {
class DeviceTarget;
struct MemoryRegionConf;
}

/// @brief Toplevel abstract class that defines the interface for the different
/// flavors of firmware we can load on the device
class Firmware {
public:
  Firmware() = default;
  virtual ~Firmware() = default;

  enum class FWType : uint8_t {
    NONE = 0,
    FAKE_FW,
    DEVICE_FW,
  };

  static const std::unordered_map<std::string, Firmware::FWType>
      fwType_str2type;

  virtual bool setFWFilePaths(const std::vector<std::string> &paths) = 0;
  virtual bool readFW() = 0;
  virtual etrtError loadOnDevice(device::DeviceTarget *dev) = 0;

  static std::unique_ptr<Firmware> allocateFirmware(std::string type);
};

class FWManager {
public:
  FWManager();
  ~FWManager() = default;

  Firmware &firmware() { return *firmware_.get(); }

private:
  std::unique_ptr<Firmware> firmware_;
};

} // namespace et_runtime

#endif // ET_RUNTIME_DEVICEFW_FWMANAGER_H
