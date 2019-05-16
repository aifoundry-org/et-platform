//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "Core/DeviceTarget.h"
#include "PCIeDevice.h"
#include "Support/Logging.h"
#include "TargetCardProxy.h"
#include "TargetRPC.h"
#include "TargetSysEmu.h"

#include <cassert>
#include <cstdio>
#include <gflags/gflags.h>
#include <memory>
#include <string>

using namespace std;

namespace et_runtime {
namespace device {

const std::map<std::string, DeviceTarget::TargetType>
    DeviceTarget::Str2TargetType = {
        {"pcie", TargetType::PCIe},
        {"sysemu_card_proxy", TargetType::SysEmuCardProxy},
        {"sysemu_grpc", TargetType::SysEmuGRPC},
        {"device_grpc", TargetType::DeviceGRPC},
};

static string getDeviceTypesStr() {
  string res = "";
  for (auto &i : DeviceTarget::Str2TargetType) {
    res += i.first + ",";
  }
  res.pop_back();
  return res;
}

/// @brief Command line option for specifying the type of device to initialize
/// FIXME this option should not be availabl in production code.
static bool validateDeviceTarget(const char *flagname, const string &value) {
  for (const auto &i : DeviceTarget::Str2TargetType) {
    if (i.first == value) {
      return true;
    }
  }
  RTERROR << "Option " << flagname << " invalid value: " << value << "\n"
          << "Allowed values: " << getDeviceTypesStr() << "\n";
  return false;
}

DEFINE_string(
    dev_target, "",
    "Specify the target device or simulator we would like to talk to");
DEFINE_validator(dev_target, validateDeviceTarget);

DeviceTarget::DeviceTarget(const string &path)
    : path_(path), device_alive_(false) {}

std::unique_ptr<DeviceTarget>
DeviceTarget::deviceFactory(TargetType target, const std::string &path) {
  switch (target) {
  case TargetType::PCIe:
    return make_unique<PCIeDevice>(path);
  case TargetType::SysEmuGRPC:
    return make_unique<TargetSysEmu>();
  case TargetType::SysEmuCardProxy:
    return make_unique<CardProxyTarget>(path);
  case TargetType::DeviceGRPC:
    return make_unique<RPCTarget>(path);
  default:
    RTERROR << "Unknwon Device type";
    assert(true);
    return nullptr;
  }
  return nullptr;
}

DeviceTarget::TargetType DeviceTarget::deviceToCreate() {
  for (const auto &i : Str2TargetType) {
    if (FLAGS_dev_target.find(i.first) != string::npos) {
      return i.second;
    }
  }
  return TargetType::None;
}

bool DeviceTarget::setDeviceType(const std::string &device_type) {
  FLAGS_dev_target = device_type;
  for (const auto &i : Str2TargetType) {
    if (FLAGS_dev_target.find(i.first) != string::npos) {
      return true;
    }
  }
  return false;
}

} // namespace device
} // namespace et_runtime
