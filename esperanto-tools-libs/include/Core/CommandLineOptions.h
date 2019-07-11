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

#include "absl/flags/flag.h"


struct FWType {
  FWType(const std::string &t) : type(t) {}

  std::string type;
};


ABSL_DECLARE_FLAG(FWType, fw_type);

struct DeviceTargetOption {
  DeviceTargetOption(const std::string &t) : dev_target(t) {}

  std::string dev_target;
};

ABSL_DECLARE_FLAG(DeviceTargetOption, dev_target);


#endif // ET_RUNTIME_COMMAND_LINE_OPTIONS_H
