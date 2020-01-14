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
#include "DeviceFW.h"
#include "esperanto/runtime/Core/DeviceTarget.h"
#include "esperanto/runtime/Support/Logging.h"

#include <memory>

using namespace std;
using namespace et_runtime;

std::unique_ptr<Firmware> Firmware::allocateFirmware() {
  return std::unique_ptr<Firmware>(new et_runtime::device_fw::DeviceFW());
}

FWManager::FWManager() : firmware_(Firmware::allocateFirmware()) {}
