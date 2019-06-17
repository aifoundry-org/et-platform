//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#ifndef ET_RUNTIME_COMMON_TYPES_H
#define ET_RUNTIME_COMMON_TYPES_H

#include <cstdint>

namespace et_runtime {
/// Unique identifier for a code module loaded by the runtime
using ModuleID = uint64_t;
} // namespace et_runtime

#endif // ET_RUNTIME_COMMON_TYPES_H
