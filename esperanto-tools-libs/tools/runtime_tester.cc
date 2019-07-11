#include "EsperantoRuntime.h"

#include <absl/flags/flag.h>
#include <absl/flags/parse.h>
#include <absl/flags/usage.h>
#include <absl/flags/usage_config.h>
#include <absl/strings/match.h>
#include <absl/strings/str_cat.h>
#include <absl/strings/string_view.h>
#include <cstdlib>
#include <experimental/filesystem>
#include <iostream>

namespace fs = std::experimental::filesystem;

ABSL_FLAG(bool, list_devices, true,
          "List all devices we can find in the system");

int main(int argc, char *argv[]) {
  absl::FlagsUsageConfig config;

  auto main_help_files = [](absl::string_view path) -> bool {
    for (auto &i : {"runtime_tester.cc", "DeviceTarget.cc", "FWManager.cc"}) {
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

  absl::SetProgramUsageMessage(
      absl::StrCat("This program exercises the runtime library", argv[0]));
  absl::ParseCommandLine(argc, argv);
  return 0;
}
