//******************************************************************************
// Copyright (C) 2020, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#ifndef ET_RUNTIME_DEVICE_FW_TRACE_H
#define ET_RUNTIME_DEVICE_FW_TRACE_H

#include <array>
#include <vector>
#include <cstdint>

namespace et_runtime {
namespace tracing {

int DeviceAPI_DeviceFW_process_device_traces(uint8_t *trace_buffers,
                                             size_t buffer_size,
                                             uint32_t num_of_buffers);
}
}

#endif // ET_RUNTIME_DEVICE_FW_TRACE_H
