//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#ifndef ET_RUNTIME_DEVICE_SYSEMU_H
#define ET_RUNTIME_DEVICE_SYSEMU_H

#include "Core/DeviceTarget.h"

#include <cassert>

namespace et_runtime {
namespace device {

class TargetSysEmu final : public DeviceTarget {
public:
  TargetSysEmu();
  virtual ~TargetSysEmu() = default;

  bool init() override {
    assert(true);
    return false;
  }

  bool deinit() override {
    assert(true);
    return false;
  }

  virtual bool getStatus() override {
    assert(true);
    return false;
  }

  virtual bool getStaticConfiguration() override {
    assert(true);
    return false;
  }

  virtual bool submitCommand() override {
    assert(true);
    return false;
  }

  virtual bool registerResponseCallback() override {
    assert(true);
    return false;
  }

  virtual bool registerDeviceEventCallback() override {
    assert(true);
    return false;
  }

  CardProxy *getCardProxy() override {
    assert(true);
    return nullptr;
  }

private:
};

} // namespace device
} // namespace et_runtime

#endif // ET_RUNTIME_DEVICE_SYSEMU_H
