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

#include "CodeManagement/ELFSupport.h"
#include "Core/CommandLineOptions.h"
#include "Core/DeviceTarget.h"
#include "DeviceFW.h"
#include "FakeFW.h"
#include "Support/Logging.h"

#include <absl/flags/flag.h>
#include <absl/flags/marshalling.h>
#include <absl/strings/str_cat.h>
#include <absl/strings/string_view.h>
#include <cassert>
#include <string>

using namespace std;
using namespace et_runtime;

const std::unordered_map<std::string, Firmware::FWType>
Firmware::fwType_str2type = {
  {"fake-fw", Firmware::FWType::FAKE_FW},
  {"device-fw", Firmware::FWType::DEVICE_FW},
};

static std::string AbslUnparseFlag(FWType type) {
  return absl::UnparseFlag(type.type);
}

/// @brief Command line option for specifying the type of fw-holder to
/// initialize
/// FIXME this option should not be available in production code.
static bool AbslParseFlag(absl::string_view text, FWType *type,
                          std::string *error) {
  if (!absl::ParseFlag(text, &type->type, error)) {
    return false;
  }
  for (const auto &i : Firmware::fwType_str2type) {
    if (i.first == type->type) {
      return true;
    }
  }
  *error = absl::StrCat("Invalid value: ", text, "\n Allowed values: ");
  for (const auto &i : Firmware::fwType_str2type) {
    *error += i.first + " ";
  }
  *error += "\n";
  return false;
}

ABSL_FLAG(FWType, fw_type, FWType("fake-fw"),
          "Specify the type of FW to load on the target: device-fw, fake-fw");

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

FWManager::FWManager() : firmware_(Firmware::allocateFirmware(absl::GetFlag(FLAGS_fw_type).type)) {}
