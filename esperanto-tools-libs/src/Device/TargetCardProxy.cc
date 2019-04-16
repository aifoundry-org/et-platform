//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------
#include "TargetCardProxy.h"

#include "et-rpc/et-card-proxy.h"

#include <glog/logging.h>

#include <stddef.h>
#include <stdint.h>

namespace et_runtime {
namespace device {

CardProxyTarget::CardProxyTarget(const std::string &path)
    : DeviceTarget(path) {}

bool CardProxyTarget::init() {
  assert(true);
  return false;
}

bool CardProxyTarget::deinit() {
  assert(true);
  return false;
}

bool CardProxyTarget::getStatus() {
  assert(true);
  return false;
}

bool CardProxyTarget::getStaticConfiguration() {
  assert(true);
  return false;
}

bool CardProxyTarget::submitCommand() {
  assert(true);
  return false;
}

bool CardProxyTarget::registerResponseCallback() {
  assert(true);
  return false;
}

bool CardProxyTarget::registerDeviceEventCallback() {
  assert(true);
  return false;
}

} // namespace device
} // namespace et_runtime
