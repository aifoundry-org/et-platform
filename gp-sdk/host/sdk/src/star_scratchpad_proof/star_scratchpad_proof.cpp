//******************************************************************************
// Copyright (c) 2026 Ainekko, Co.
// SPDX-License-Identifier: Apache-2.0
//------------------------------------------------------------------------------

#include <cstdlib>
#include <cstring>
#include <getopt.h>
#include <stdexcept>
#include <string>

#include "GenericLauncher.h"
#include "gpsdk_star_scratchpad.h"

namespace {

struct Options {
  fs::path kernel_path = "";
  int kernel_launch_timeout = 10;
  std::string device_type = "sysemu";
  uint32_t shire_mask = 0xFFFFFFFF;
  bool read_neighbor_probes = false;
  bool scratchpad_star = false;
  bool scratchpad_block = false;
  bool scratchpad_nested_star = false;
};

Options parse_args(int argc, char* const* argv, std::vector<char*>& nextlevel) {
  static constexpr const char* help_msg =
    "Usage: [options]\n\n"
    "Launch a scratchpad-cluster validation kernel and verify the center success marker.\n\n"
    "Required:\n"
    "  -k, --kernel_path             path to kernel elf file to execute.\n\n"
    "Optional:\n"
    "  -t, --kernel_launch_timeout   timeout (in seconds) to wait for kernel completion.\n"
    "  -d, --device_type             device type to use (sysemu, fake, silicon).\n"
    "  -m, --shire_mask              single-bit center shire mask used for compute.\n"
    "      --read_neighbor_probes    attempt host-side reads of the four probe slots.\n";

  static constexpr const char* short_opts = "k:t:d:m:h";
  static const std::vector<struct option> long_opts_vect{{"kernel_path", required_argument, nullptr, 'k'},
                                                         {"kernel_launch_timeout", required_argument, nullptr, 't'},
                                                         {"device_type", required_argument, nullptr, 'd'},
                                                         {"shire_mask", required_argument, nullptr, 'm'},
                                                         {"read_neighbor_probes", no_argument, nullptr, 0},
                                                         {"help", no_argument, nullptr, 'h'},
                                                         {nullptr, 0, nullptr, 0}};

  Options opts;
  int ret = 0;
  int index = 0;
  opterr = 0;

  while ((ret = getopt_long_only(argc, argv, short_opts, long_opts_vect.data(), &index)) != -1) {
    if ((ret == 0) && !std::strcmp(long_opts_vect[index].name, "read_neighbor_probes")) {
      opts.read_neighbor_probes = true;
      continue;
    }

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
      opts.shire_mask = std::stoul(optarg, nullptr, 16);
      break;
    case 'h':
      std::cout << help_msg << GenericLauncher::help_msg << std::endl;
      std::exit(0);
    case '?':
      if (!std::strcmp(argv[optind - 1], "--scratchpad_star")) {
        opts.scratchpad_star = true;
      } else if (!std::strcmp(argv[optind - 1], "--scratchpad_block")) {
        opts.scratchpad_block = true;
      } else if (!std::strcmp(argv[optind - 1], "--scratchpad_nested_star")) {
        opts.scratchpad_nested_star = true;
      }
      nextlevel.emplace_back(argv[optind - 1]);
      break;
    default:
      std::cout << "Error: Unknown option " << argv[optind - 1] << ". See " << argv[0] << " --help'.\n" << std::endl;
      std::exit(1);
    }
  }

  if (__builtin_popcount(opts.shire_mask) != 1) {
    std::cout << "Error: --shire_mask must select exactly one center shire.\n";
    std::exit(1);
  }

  if ((static_cast<uint32_t>(opts.scratchpad_star) + static_cast<uint32_t>(opts.scratchpad_block) +
       static_cast<uint32_t>(opts.scratchpad_nested_star)) > 1U) {
    std::cout << "Error: scratchpad cluster modes are mutually exclusive.\n";
    std::exit(1);
  }

  return opts;
}

inline gpsdk::star_scratchpad::ClusterLayout getClusterLayout(const Options& opts) {
  if (opts.scratchpad_nested_star) {
    return gpsdk::star_scratchpad::ClusterLayout::NestedStar;
  }
  return opts.scratchpad_block ? gpsdk::star_scratchpad::ClusterLayout::Block
                               : gpsdk::star_scratchpad::ClusterLayout::Star;
}

inline void computeStarNeighbors(uint32_t centerShire, uint32_t* neighbors) {
  for (uint32_t idx = 0; idx < gpsdk::star_scratchpad::kStarNeighborCount; ++idx) {
    neighbors[idx] = gpsdk::star_scratchpad::neighborShire(centerShire, idx);
  }
}

inline uint64_t makeProbeValue(uint32_t centerShire, uint32_t neighborShire, uint32_t relativeThreadId) {
  return (0x5A5A000000000000ULL | (static_cast<uint64_t>(centerShire) << 24) |
          (static_cast<uint64_t>(neighborShire) << 8) | static_cast<uint64_t>(relativeThreadId));
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

  uint32_t resolveCenterShire(uint64_t requestedShireMask, gpsdk::star_scratchpad::ClusterLayout layout) {
    return resolveScratchpadClusterSelection(requestedShireMask, layout).effectiveCenterShire;
  }
};

} // namespace

int main(int argc, char** argv) {
  std::vector<char*> argvPendingToParse{argv[0]};
  const Options opt = parse_args(argc, argv, argvPendingToParse);

  Config config{modeFromString(opt.device_type), 1};
  config.dump();

  Launcher launcher(config, static_cast<int>(argvPendingToParse.size()), argvPendingToParse.data());
  launcher.initialize();

  const auto kernelId = launcher.loadKernel(opt.kernel_path);
  launcher.kernelLaunch(kernelId, 0, opt.shire_mask);
  launcher.waitKernelCompletion(std::chrono::seconds(opt.kernel_launch_timeout));

  if (launcher.checkKernelExecutionErrors()) {
    return -1;
  }

  const auto centerShire = launcher.resolveCenterShire(opt.shire_mask, getClusterLayout(opt));
  if (opt.scratchpad_nested_star) {
    std::cout << "Using per-device topology cache " << launcher.getTopologyCachePath() << "\n";
  }
  const auto markerAddress = gpsdk::star_scratchpad::successMarkerAddress(centerShire);
  const auto markerValue = launcher.readU64(markerAddress);
  if (markerValue != gpsdk::star_scratchpad::kSuccessMarkerValue) {
    std::cout << "Success marker mismatch at center shire " << centerShire << " address 0x" << std::hex
              << markerAddress << ": got 0x" << markerValue << " expected 0x"
              << gpsdk::star_scratchpad::kSuccessMarkerValue << std::dec << "\n";
    return 2;
  }

  std::cout << "Success marker center shire " << centerShire << " address 0x" << std::hex << markerAddress << " = 0x"
            << markerValue << std::dec << "\n";

  if (opt.read_neighbor_probes) {
    uint32_t neighbors[gpsdk::star_scratchpad::kStarNeighborCount];
    computeStarNeighbors(centerShire, neighbors);
    for (uint32_t idx = 0; idx < gpsdk::star_scratchpad::kStarNeighborCount; ++idx) {
      const auto address = gpsdk::star_scratchpad::probeAddress(neighbors[idx], idx);
      const auto value = launcher.readU64(address);
      const auto expected = makeProbeValue(centerShire, neighbors[idx], 0U);
      if (value == expected) {
        std::cout << "Diagnostic probe " << idx << " shire " << neighbors[idx] << " address 0x" << std::hex
                  << address << " = 0x" << value << std::dec << "\n";
      } else {
        std::cout << "Diagnostic probe " << idx << " shire " << neighbors[idx] << " address 0x" << std::hex
                  << address << " returned 0x" << value << " expected 0x" << expected
                  << " (host-side DMA visibility is limited for some scratchpad shires)" << std::dec << "\n";
      }
    }
  }

  launcher.unLoadKernel(kernelId);
  launcher.tearDown();
  return 0;
}
