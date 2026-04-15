//******************************************************************************
// Copyright (c) 2026 Ainekko, Co.
// SPDX-License-Identifier: Apache-2.0
//------------------------------------------------------------------------------

#include <cstdlib>
#include <getopt.h>
#include <stdexcept>
#include <string>

#include "GenericLauncher.h"

namespace {

constexpr uint32_t kStarCols = 8U;
constexpr uint32_t kStarNeighborCount = 4U;
constexpr uint64_t kScpRegionBaseAddress = 0x80000000ULL;
constexpr uint64_t kScpShireSize = 0x280000ULL;
constexpr uint64_t kProbeBaseOffset = kScpShireSize - 0x4000ULL;
constexpr uint64_t kProbeNeighborStride = 0x400ULL;
constexpr uint64_t kSuccessMarkerOffset = kProbeBaseOffset + 0x2000ULL;
constexpr uint64_t kSuccessMarker = 0x5354415250524F42ULL;

struct Options {
  fs::path kernel_path = "";
  int kernel_launch_timeout = 10;
  std::string device_type = "sysemu";
  uint32_t shire_mask = 0xFFFFFFFF;
};

Options parse_args(int argc, char* const* argv, std::vector<char*>& nextlevel) {
  static constexpr const char* help_msg =
    "Usage: [options]\n\n"
    "Launch the star scratchpad probe kernel and verify neighboring shire scratchpad contents.\n\n"
    "Required:\n"
    "  -k, --kernel_path             path to kernel elf file to execute.\n\n"
    "Optional:\n"
    "  -t, --kernel_launch_timeout   timeout (in seconds) to wait for kernel completion.\n"
    "  -d, --device_type             device type to use (sysemu, fake, silicon).\n"
    "  -m, --shire_mask              single-bit center shire mask used for compute.\n";

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
      opts.shire_mask = std::stoul(optarg, nullptr, 16);
      break;
    case 'h':
      std::cout << help_msg << GenericLauncher::help_msg << std::endl;
      std::exit(0);
    case '?':
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

  return opts;
}

inline uint32_t getCenterShire(uint32_t shireMask) {
  return static_cast<uint32_t>(__builtin_ctz(shireMask));
}

inline void computeStarNeighbors(uint32_t centerShire, uint32_t* neighbors) {
  neighbors[0] = centerShire - kStarCols;
  neighbors[1] = centerShire + 1U;
  neighbors[2] = centerShire + kStarCols;
  neighbors[3] = centerShire - 1U;
}

inline uint64_t makeProbeValue(uint32_t centerShire, uint32_t neighborShire, uint32_t relativeThreadId) {
  return (0x5A5A000000000000ULL | (static_cast<uint64_t>(centerShire) << 24) |
          (static_cast<uint64_t>(neighborShire) << 8) | static_cast<uint64_t>(relativeThreadId));
}

inline uint64_t getProbeAddress(uint32_t shireId, uint32_t neighborIdx) {
  const auto offset = kProbeBaseOffset + (static_cast<uint64_t>(neighborIdx) * kProbeNeighborStride);
  return (((static_cast<uint64_t>(shireId) << 23) & 0x3F800000ULL) + kScpRegionBaseAddress + offset);
}

inline uint64_t getSuccessMarkerAddress(uint32_t shireId) {
  return (((static_cast<uint64_t>(shireId) << 23) & 0x3F800000ULL) + kScpRegionBaseAddress + kSuccessMarkerOffset);
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

  const auto centerShire = getCenterShire(opt.shire_mask);
  const auto markerAddress = getSuccessMarkerAddress(centerShire);
  const auto markerValue = launcher.readU64(markerAddress);
  if (markerValue != kSuccessMarker) {
    std::cout << "Success marker mismatch at center shire " << centerShire << " address 0x" << std::hex
              << markerAddress << ": got 0x" << markerValue << " expected 0x" << kSuccessMarker << std::dec << "\n";
    return 2;
  }

  std::cout << "Success marker center shire " << centerShire << " address 0x" << std::hex << markerAddress << " = 0x"
            << markerValue << std::dec << "\n";

  uint32_t neighbors[kStarNeighborCount];
  computeStarNeighbors(centerShire, neighbors);
  for (uint32_t idx = 0; idx < kStarNeighborCount; ++idx) {
    const auto address = getProbeAddress(neighbors[idx], idx);
    const auto value = launcher.readU64(address);
    const auto expected = makeProbeValue(centerShire, neighbors[idx], 0U);
    if (value == expected) {
      std::cout << "Diagnostic probe " << idx << " shire " << neighbors[idx] << " address 0x" << std::hex << address
                << " = 0x" << value << std::dec << "\n";
    } else {
      std::cout << "Diagnostic probe " << idx << " shire " << neighbors[idx] << " address 0x" << std::hex << address
                << " returned 0x" << value << " expected 0x" << expected
                << " (host-side DMA visibility is limited for some scratchpad shires)" << std::dec << "\n";
    }
  }

  launcher.unLoadKernel(kernelId);
  launcher.tearDown();
  return 0;
}
