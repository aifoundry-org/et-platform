//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#ifndef ET_RUNTIME_DEVICE_HELPERS_H
#define ET_RUNTIME_DEVICE_HELPERS_H

#include <cstdint>

/// Set of classes that use the Device API and help us interact wit the
/// device

namespace et_runtime {

class Device;

// Forward declarations of commands and responses
namespace device_api {
namespace devfw_commands {
class SetMasterLogLevelCmd;
class SetWorkerLogLevelCmd;
}
namespace devfw_responses {
class SetMasterLogLevelRsp;
class SetWorkerLogLevelRsp;
}
} // namespace device_api

template <class SetLogLevelCmd, class SetLogLevelRsp> class SetLogLevel {
public:
  SetLogLevel(Device &dev) : dev_(dev){};

  bool set_level_critical();
  bool set_level_error();
  bool set_level_warning();
  bool set_level_info();
  bool set_level_debug();
  bool set_level_trace();

private:
  Device &dev_;
};

using SetMasterLogLevel =
    SetLogLevel<device_api::devfw_commands::SetMasterLogLevelCmd,
                device_api::devfw_responses::SetMasterLogLevelRsp>;

using SetWorkerLogLevel =
    SetLogLevel<device_api::devfw_commands::SetWorkerLogLevelCmd,
                device_api::devfw_responses::SetWorkerLogLevelRsp>;

} // namespace et_runtime

#endif // ET_RUNTIME_DEVICE_HELPERS_H
