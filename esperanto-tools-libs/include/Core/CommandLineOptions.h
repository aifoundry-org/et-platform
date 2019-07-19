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

#include <string>
#include <vector>

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

ABSL_DECLARE_FLAG(std::string, master_minion_elf);
ABSL_DECLARE_FLAG(std::string, worker_minion_elf);
ABSL_DECLARE_FLAG(std::string, machine_minion_elf);

namespace et_runtime {
///
/// @brief Helper function that parses any runtime command
/// line specific options. To be used by in the main function
/// of tests or utilities
///
/// @param[in] argc Original argc from main
/// @param[in] argv Origianl argv from main
/// @param[in] help_enable Vector of strings where the string is the filename
/// whose
///    abseil options we want to be listed in the "short" help message that
///    abseil generates
void ParseCommandLineOptions(int argc, char **argv,
                             const std::vector<std::string> &help_enable = {});
} // namespace et_runtime

#endif // ET_RUNTIME_COMMAND_LINE_OPTIONS_H
