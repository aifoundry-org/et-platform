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
#include "TargetCardProxy.h"
#include "TargetRPC.h"
#include "TargetSysEmu.h"

#include <cassert>
#include <memory>
#include <string>

using namespace std;

namespace et_runtime {
namespace device {

DeviceTarget::DeviceTarget(const string& path):
  path_(path)
{
}

std::unique_ptr<DeviceTarget>
DeviceTarget::deviceFactory(TargetType target, const std::string &path) {
  switch (target) {
  case TargetType::PCIe:
    return make_unique<PCIeDevice>(path);
  case TargetType::SysEmu:
    return make_unique<TargetSysEmu>();
  case TargetType::CardProxy:
    return make_unique<CardProxyTarget>(path);
  case TargetType::DeviceRPC:
    return make_unique<RPCTarget>(path);
  default:
    assert(true);
    return nullptr;
  }
  return nullptr;
}

} // namespace device
} // namespace et_runtime
