//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#ifndef ET_RUNTIME_DEVICE_DEVICE_FW_TYPES_H
#define ET_RUNTIME_DEVICE_DEVICE_FW_TYPES_H

#include "esperanto/runtime/Common/ProjectAutogen.h"

#include <stdint.h>

/// Wrap the device-fw types in a separate namespace

namespace et_runtime {
namespace device_fw {

#include <esperanto-fw/firmware_helpers/layout.h>
#include <esperanto-fw/firmware_helpers/mailbox_common.h>
#include <esperanto-fw/firmware_helpers/ringbuffer_common.h>
#include <esperanto-fw/firmware_helpers/vqueue_common.h>
#include "esperanto/runtime/Common/mm_dev_intf_reg.h"

} // namespace device_fw
} // namespace et_runtime

#endif // ET_RUNTIME_DEVICE_DEVICE_FW_TYPES_H
