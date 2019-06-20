//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#ifndef ET_RUNTIME_COMMAND_LINE_OPTIONS_H
#define ET_RUNTIME_COMMAND_LINE_OPTIONS_H

#include <gflags/gflags.h>

namespace et_runtime {
DECLARE_string(fw_type);
// extern std::string FLAGS_
namespace device {

DECLARE_string(dev_target);
}
} // namespace et_runtime

#endif // ET_RUNTIME_COMMAND_LINE_OPTIONS_H
