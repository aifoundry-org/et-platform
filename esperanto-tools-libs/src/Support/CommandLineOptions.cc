//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "Core/CommandLineOptions.h"

#include <absl/flags/flag.h>
#include <absl/flags/parse.h>
#include <absl/flags/usage.h>
#include <absl/flags/usage_config.h>
#include <experimental/filesystem>
#include <string>
#include <vector>

namespace fs = std::experimental::filesystem;

namespace et_runtime {
void ParseCommandLineOptions(int argc, char **argv,
                             const std::vector<std::string> &help_enable) {
  // Parse runtime arguments using abseil
  absl::FlagsUsageConfig config;
  std::vector<std::string> gen_help_for{"DeviceTarget.cc", "FWManager.cc",
                                        "DeviceFW.cc"};
  gen_help_for.insert(gen_help_for.end(), help_enable.begin(),
                      help_enable.end());
  auto main_help_files = [](absl::string_view path) -> bool {
    for (auto &i : {"runtime_tester.cc", "DeviceTarget.cc", "FWManager.cc",
                    "DeviceFW.cc"}) {
      auto fname = fs::path(path).filename().string();
      if (fname == i) {
        return true;
      }
    }
    return false;
  };

  config.contains_helpshort_flags = config.contains_help_flags =
      main_help_files;
  config.version_string = []() { return "0.0.1"; };
  config.normalize_filename = [](absl::string_view path) -> std::string {
    return fs::path(path).filename().string();
  };
  absl::SetFlagsUsageConfig(config);

  absl::ParseCommandLine(argc, argv);
}

} // namespace et_runtime
