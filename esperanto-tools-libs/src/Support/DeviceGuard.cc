//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "Support/DeviceGuard.h"

#include "Core/Device.h"
#include "Core/DeviceManager.h"

GetDev::GetDev() : dev(getEtDevice()) { dev.mutex_.lock(); }

GetDev::~GetDev() { dev.mutex_.unlock(); }

et_runtime::Device &GetDev::getEtDevice() {
  static et_runtime::Device et_device_;
  return et_device_;
}
