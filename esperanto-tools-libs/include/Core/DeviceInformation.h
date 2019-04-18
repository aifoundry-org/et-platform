//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#ifndef ET_RUNTIME_DEVICE_INFORMATION_H
#define ET_RUNTIME_DEVICE_INFORMATION_H

#include "etrt-bin.h"

#include <stddef.h>
#include <stdint.h>

namespace et_runtime {

///
/// @brief ET Device properties
///
struct DeviceInformation final : public etrtDeviceProp {
  DeviceInformation() = default;
  DeviceInformation(const etrtDeviceProp &prop) { *this = prop; }
};

} // namespace et_runtime

#endif // ET_RUNTIME_DEVICE_INFORMATION_H
