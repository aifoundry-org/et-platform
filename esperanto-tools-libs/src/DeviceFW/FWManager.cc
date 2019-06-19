//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "FWManager.h"

#include "DeviceFW.h"
#include "FakeFW.h"
#include "Support/Logging.h"

#include <cassert>
#include <gflags/gflags.h>

using namespace std;

namespace et_runtime {

const std::unordered_map<std::string, Firmware::FWType>
    Firmware::fwType_str2type = {
        {"fake-fw", Firmware::FWType::FAKE_FW},
        {"device-fw", Firmware::FWType::DEVICE_FW},
};

/// @brief Command line option for specifying the type of fw-holder to
/// initialize
/// FIXME this option should not be available in production code.
static bool validateFWType(const char *flagname, const string &value) {
  for (const auto &i : Firmware::fwType_str2type) {
    if (i.first == value) {
      return true;
    }
  }
  RTERROR << "Option " << flagname << " invalid value: " << value << "\n"
          << "Allowed values: ";
  for (const auto &i : Firmware::fwType_str2type) {
    RTERROR << i.first << " ";
  }
  RTERROR << "\n";
  return false;
}

DEFINE_string(fw_type, "", "Specify the type of FW to load on the target.");
DEFINE_validator(fw_type, validateFWType);

std::unique_ptr<Firmware> Firmware::allocateFirmware(std::string type) {
  auto it = Firmware::fwType_str2type.find(type);
  assert(it != Firmware::fwType_str2type.end());
  switch (it->second) {
  case Firmware::FWType::FAKE_FW:
    return std::unique_ptr<Firmware>(new et_runtime::device_fw::FakeFW());
    break;
  case Firmware::FWType::DEVICE_FW:
    return std::unique_ptr<Firmware>(new et_runtime::device_fw::DeviceFW());
    break;
  default:
    abort();
  }
  return nullptr;
}

FWManager::FWManager(const std::string &type)
    : firmware_(Firmware::allocateFirmware(type)) {}

} // namespace et_runtime
