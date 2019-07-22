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
#include "Core/CommandLineOptions.h"
#include "PCIeDevice.h"
#include "Support/Logging.h"
#include "TargetRPC.h"
#include "TargetSysEmu.h"

#include <absl/flags/flag.h>
#include <absl/flags/marshalling.h>
#include <absl/strings/numbers.h>
#include <absl/strings/str_cat.h>
#include <absl/strings/string_view.h>
#include <cassert>
#include <cstdio>
#include <memory>
#include <string>

using namespace std;
using namespace et_runtime::device;

const std::map<std::string, DeviceTarget::TargetType>
    DeviceTarget::Str2TargetType = {
        {"pcie", TargetType::PCIe},
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


std::string AbslUnparseFlag(DeviceTargetOption target) {
  return absl::UnparseFlag(target.dev_target);
}

/// @brief Command line option for specifying the type of device to initialize
/// FIXME this option should not be available in production code.
 bool AbslParseFlag(absl::string_view text, DeviceTargetOption *target,
                          std::string *error) {
  if (!absl::ParseFlag(text, &target->dev_target, error)) {
    return false;
  }
  for (const auto &i : et_runtime::device::DeviceTarget::Str2TargetType) {
    if (i.first == target->dev_target) {
      return true;
    }
  }
  *error = absl::StrCat("Invalid value: ", text, "\n",
                        "Allowed values: ", getDeviceTypesStr(), "\n");
  return false;
}

ABSL_FLAG(DeviceTargetOption, dev_target, DeviceTargetOption("device_grpc"),
          "Specify the target device or simulator we would like to talk "
          "to: pcie , sysemu_grpc, device_grpc");

DeviceTarget::DeviceTarget(int index) : index_(index), device_alive_(false) {}

std::unique_ptr<DeviceTarget> DeviceTarget::deviceFactory(TargetType target,
                                                          int index) {
  switch (target) {
  case TargetType::PCIe: {
    return make_unique<PCIeDevice>(index);
  }
  case TargetType::SysEmuGRPC:
    return make_unique<TargetSysEmu>(index);
    // FIXME we have no usecase yet where we instantite RPCTarget directly
  // case TargetType::DeviceGRPC:
  //   return make_unique<RPCTarget>(index, "");
  default:
    RTERROR << "Unknwon Device type";
    assert(false);
    return nullptr;
  }
  return nullptr;
}

DeviceTarget::TargetType DeviceTarget::deviceToCreate() {
  auto target = absl::GetFlag(FLAGS_dev_target);
  for (const auto &i : Str2TargetType) {
    if (target.dev_target.find(i.first) != string::npos) {
      return i.second;
    }
  }
  return TargetType::None;
}

bool DeviceTarget::setDeviceType(const std::string &device_type) {
  absl::SetFlag(&FLAGS_dev_target, DeviceTargetOption(device_type));
  return true;
}
