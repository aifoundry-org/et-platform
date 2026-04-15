//******************************************************************************
// Copyright (c) 2026 Ainekko, Co.
// SPDX-License-Identifier: Apache-2.0
//------------------------------------------------------------------------------

#include <cstdlib>
#include <cstring>
#include <getopt.h>
#include <stdexcept>
#include <string>
#include <vector>

#include "GenericLauncher.h"
#include "gpsdk_star_scratchpad.h"

namespace {

struct Options {
  fs::path kernel_path = "";
  int kernel_launch_timeout = 60;
  std::string device_type = "silicon";
  uint32_t shire_mask = 0x200;
};

Options parse_args(int argc, char* const* argv, std::vector<char*>& nextlevel) {
  static constexpr const char* help_msg =
    "Usage: [options]\n\n"
    "Infer per-device topology and launch the nested scratchpad proof demo.\n\n"
    "Required:\n"
    "  -k, --kernel_path             path to the proof kernel elf file to execute.\n\n"
    "Optional:\n"
    "  -t, --kernel_launch_timeout   timeout (in seconds) to wait for kernel completion.\n"
    "  -d, --device_type             device type to use (sysemu, fake, silicon).\n"
    "  -m, --shire_mask              single-bit requested center shire mask.\n";

  static constexpr const char* short_opts = "k:t:d:m:h";
  static const std::vector<struct option> long_opts_vect{{"kernel_path", required_argument, nullptr, 'k'},
                                                         {"kernel_launch_timeout", required_argument, nullptr, 't'},
                                                         {"device_type", required_argument, nullptr, 'd'},
                                                         {"shire_mask", required_argument, nullptr, 'm'},
                                                         {"help", no_argument, nullptr, 'h'},
                                                         {nullptr, 0, nullptr, 0}};

  Options opts;
  int ret = 0;
  int index = 0;
  opterr = 0;

  while ((ret = getopt_long_only(argc, argv, short_opts, long_opts_vect.data(), &index)) != -1) {
    switch (ret) {
    case 'k':
      opts.kernel_path = optarg;
      break;
    case 't':
      opts.kernel_launch_timeout = std::atoi(optarg);
      break;
    case 'd':
      opts.device_type = optarg;
      break;
    case 'm':
      opts.shire_mask = std::stoul(optarg, nullptr, 0);
      break;
    case 'h':
      std::cout << help_msg << GenericLauncher::help_msg << std::endl;
      std::exit(0);
    case '?':
      nextlevel.emplace_back(argv[optind - 1]);
      break;
    default:
      std::cout << "Error: Unknown option " << argv[optind - 1] << ". See " << argv[0] << " --help'.\n";
      std::exit(1);
    }
  }

  if (opts.kernel_path.empty()) {
    std::cout << "Error: --kernel_path is required.\n";
    std::exit(1);
  }

  if (__builtin_popcount(opts.shire_mask) != 1) {
    std::cout << "Error: --shire_mask must select exactly one center shire.\n";
    std::exit(1);
  }

  return opts;
}

class Launcher : public GenericLauncher {
public:
  using GenericLauncher::GenericLauncher;

  uint64_t readU64(uint64_t deviceAddress, uint32_t deviceIdx = 0) {
    uint64_t value = 0;
    runtime_->memcpyDeviceToHost(defaultStreams_[deviceIdx], reinterpret_cast<std::byte*>(deviceAddress),
                                 reinterpret_cast<std::byte*>(&value), sizeof(value));
    const auto timeout = std::chrono::seconds(5);
    if (!runtime_->waitForStream(defaultStreams_[deviceIdx], timeout)) {
      throw std::runtime_error("Timed out while reading device memory");
    }
    return value;
  }
};

} // namespace

int main(int argc, char** argv) {
  std::vector<char*> argvPendingToParse{argv[0]};
  const Options opt = parse_args(argc, argv, argvPendingToParse);

  static char nestedFlag[] = "--scratchpad_nested_star";
  argvPendingToParse.emplace_back(nestedFlag);

  Config config{modeFromString(opt.device_type), 1};
  config.dump();

  Launcher launcher(config, static_cast<int>(argvPendingToParse.size()), argvPendingToParse.data(), false);
  launcher.initialize();

  const auto selection = launcher.resolveScratchpadClusterSelection(
    opt.shire_mask, gpsdk::star_scratchpad::ClusterLayout::NestedStar);
  if (!selection.valid()) {
    std::cout << "Unable to resolve a nested scratchpad cluster for requested mask 0x" << std::hex << opt.shire_mask
              << std::dec << ".\n";
    return 2;
  }

  std::cout << "Topology cache: " << launcher.getTopologyCachePath() << "\n";
  std::cout << "Nested center shire: " << selection.effectiveCenterShire;
  if (selection.centerShifted) {
    std::cout << " (shifted)";
  }
  std::cout << "\nRelays:";
  for (uint32_t idx = 0U; idx < selection.relayCount; ++idx) {
    std::cout << " " << static_cast<uint32_t>(selection.relayShires[idx]);
  }
  std::cout << "\nLeaves:";
  for (uint32_t idx = 0U; idx < selection.auxiliaryCount; ++idx) {
    std::cout << " " << static_cast<uint32_t>(selection.auxiliaryShires[idx]);
  }
  std::cout << "\n";

  const auto kernelId = launcher.loadKernel(opt.kernel_path);
  launcher.kernelLaunch(kernelId, 0, opt.shire_mask);
  launcher.waitKernelCompletion(std::chrono::seconds(opt.kernel_launch_timeout));

  if (launcher.checkKernelExecutionErrors()) {
    return 3;
  }

  const auto markerAddress = gpsdk::star_scratchpad::successMarkerAddress(selection.effectiveCenterShire);
  const auto markerValue = launcher.readU64(markerAddress);
  if (markerValue != gpsdk::star_scratchpad::kSuccessMarkerValue) {
    std::cout << "Success marker mismatch at center shire " << selection.effectiveCenterShire << " address 0x"
              << std::hex << markerAddress << ": got 0x" << markerValue << " expected 0x"
              << gpsdk::star_scratchpad::kSuccessMarkerValue << std::dec << "\n";
    return 4;
  }

  std::cout << "Success marker center shire " << selection.effectiveCenterShire << " address 0x" << std::hex
            << markerAddress << " = 0x" << markerValue << std::dec << "\n";

  launcher.unLoadKernel(kernelId);
  launcher.tearDown();
  return 0;
}
