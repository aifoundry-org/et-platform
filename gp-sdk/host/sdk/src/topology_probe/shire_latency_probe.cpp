//******************************************************************************
// Copyright (c) 2026 Ainekko, Co.
// SPDX-License-Identifier: Apache-2.0
//------------------------------------------------------------------------------

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <getopt.h>
#include <iomanip>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "GenericLauncher.h"
#include "gpsdk_topology_probe.h"

namespace {

struct Options {
  fs::path kernel_path = "";
  int kernel_launch_timeout = 20;
  std::string device_type = "silicon";
  std::optional<uint64_t> center_mask;
  std::optional<uint64_t> target_mask;
  fs::path csv_path = "";
};

Options parse_args(int argc, char* const* argv, std::vector<char*>& nextlevel) {
  static constexpr const char* help_msg =
    "Usage: [options]\n\n"
    "Launch a shire-to-shire scratchpad latency probe and collect the center->target cycle matrix.\n\n"
    "Required:\n"
    "  -k, --kernel_path             path to kernel elf file to execute.\n\n"
    "Optional:\n"
    "  -t, --kernel_launch_timeout   timeout (in seconds) to wait for kernel completion.\n"
    "  -d, --device_type             device type to use (sysemu, fake, silicon).\n"
    "      --center_mask             shire mask of centers to probe. Defaults to device active shires.\n"
    "      --target_mask             shire mask of targets to measure. Defaults to device active shires.\n"
    "      --csv                     optional CSV output path.\n";

  static constexpr const char* short_opts = "k:t:d:h";
  static const std::vector<struct option> long_opts_vect{
    {"kernel_path", required_argument, nullptr, 'k'},
    {"kernel_launch_timeout", required_argument, nullptr, 't'},
    {"device_type", required_argument, nullptr, 'd'},
    {"center_mask", required_argument, nullptr, 0},
    {"target_mask", required_argument, nullptr, 0},
    {"csv", required_argument, nullptr, 0},
    {"help", no_argument, nullptr, 'h'},
    {nullptr, 0, nullptr, 0}};

  Options opts;
  int ret = 0;
  int index = 0;
  opterr = 0;

  while ((ret = getopt_long_only(argc, argv, short_opts, long_opts_vect.data(), &index)) != -1) {
    if (ret == 0) {
      const char* const name = long_opts_vect[index].name;
      if (!std::strcmp(name, "center_mask")) {
        opts.center_mask = std::stoull(optarg, nullptr, 0);
      } else if (!std::strcmp(name, "target_mask")) {
        opts.target_mask = std::stoull(optarg, nullptr, 0);
      } else if (!std::strcmp(name, "csv")) {
        opts.csv_path = optarg;
      }
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

  return opts;
}

class Launcher : public GenericLauncher {
public:
  using GenericLauncher::GenericLauncher;

  rt::DeviceProperties getDeviceConfig(uint32_t deviceIdx = 0) const {
    return runtime_->getDeviceProperties(devices_.at(deviceIdx));
  }

  std::byte* allocateDeviceBuffer(size_t size, uint32_t deviceIdx = 0) {
    return runtime_->mallocDevice(devices_.at(deviceIdx), size);
  }

  void freeDeviceBuffer(std::byte* deviceBuffer, uint32_t deviceIdx = 0) {
    runtime_->freeDevice(devices_.at(deviceIdx), deviceBuffer);
  }

  template <typename T> T readObject(std::byte* deviceAddress, uint32_t deviceIdx = 0) {
    T value{};
    runtime_->memcpyDeviceToHost(defaultStreams_[deviceIdx], deviceAddress, reinterpret_cast<std::byte*>(&value),
                                 sizeof(T));
    const auto timeout = std::chrono::seconds(5);
    if (!runtime_->waitForStream(defaultStreams_[deviceIdx], timeout)) {
      throw std::runtime_error("Timed out while reading device memory");
    }
    return value;
  }
};

std::vector<uint32_t> shiresFromMask(uint64_t mask) {
  std::vector<uint32_t> shires;
  while (mask != 0ULL) {
    const auto shire = static_cast<uint32_t>(__builtin_ctzll(mask));
    shires.push_back(shire);
    mask &= (mask - 1ULL);
  }
  return shires;
}

void emitCsv(const fs::path& csvPath, const std::vector<gpsdk::topology_probe::ShireLatencyResults>& allResults) {
  std::ofstream csv(csvPath);
  csv << "center_shire,target_shire,load_cycles,store_cycles\n";
  for (const auto& result : allResults) {
    for (uint32_t shire = 0U; shire < gpsdk::topology_probe::kVisibleComputeShires; ++shire) {
      if (((result.targetShireMask >> shire) & 0x1ULL) == 0ULL) {
        continue;
      }
      csv << result.centerShire << "," << shire << "," << result.loadBestCycles[shire] << ","
          << result.storeBestCycles[shire] << "\n";
    }
  }
}

} // namespace

int main(int argc, char** argv) {
  std::vector<char*> argvPendingToParse{argv[0]};
  const Options opt = parse_args(argc, argv, argvPendingToParse);

  Config config{modeFromString(opt.device_type), 1};
  config.dump();

  Launcher launcher(config, static_cast<int>(argvPendingToParse.size()), argvPendingToParse.data());
  launcher.initialize();

  const auto deviceConfig = launcher.getDeviceConfig();
  const uint64_t activeMask = deviceConfig.computeMinionShireMask_;
  const uint64_t centerMask = opt.center_mask.value_or(activeMask) & activeMask;
  const uint64_t targetMask = opt.target_mask.value_or(activeMask) & activeMask;

  if (centerMask == 0ULL) {
    std::cout << "No center shires selected after intersecting with active mask 0x" << std::hex << activeMask
              << std::dec << ".\n";
    return 1;
  }

  const auto kernelId = launcher.loadKernel(opt.kernel_path);
  auto* const resultsBuffer = launcher.allocateDeviceBuffer(sizeof(gpsdk::topology_probe::ShireLatencyResults));
  std::vector<gpsdk::topology_probe::ShireLatencyResults> allResults;

  for (const auto centerShire : shiresFromMask(centerMask)) {
    gpsdk::topology_probe::ProbeArguments args;
    args.targetShireMask = targetMask;
    args.resultsAddress = reinterpret_cast<uint64_t>(resultsBuffer);

    launcher.kernelLaunch(kernelId, &args, nullptr, 0, 0, (1ULL << centerShire));
    launcher.waitKernelCompletion(std::chrono::seconds(opt.kernel_launch_timeout));

    if (launcher.checkKernelExecutionErrors()) {
      launcher.freeDeviceBuffer(resultsBuffer);
      launcher.unLoadKernel(kernelId);
      launcher.tearDown();
      return 2;
    }

    const auto results = launcher.readObject<gpsdk::topology_probe::ShireLatencyResults>(resultsBuffer);
    if ((results.magic != gpsdk::topology_probe::kResultsMagic) || (results.centerShire != centerShire)) {
      std::cout << std::dec << "Invalid probe results for center shire " << centerShire << ". magic=0x" << std::hex
                << results.magic << " reported_center=" << std::dec << results.centerShire << "\n";
      launcher.freeDeviceBuffer(resultsBuffer);
      launcher.unLoadKernel(kernelId);
      launcher.tearDown();
      return 3;
    }

    allResults.push_back(results);
    std::cout << std::dec << "Center shire " << centerShire << " load/store cycles:";
    for (uint32_t shire = 0U; shire < gpsdk::topology_probe::kVisibleComputeShires; ++shire) {
      if (((targetMask >> shire) & 0x1ULL) == 0ULL) {
        continue;
      }
      std::cout << " " << shire << ":" << results.loadBestCycles[shire] << "/" << results.storeBestCycles[shire];
    }
    std::cout << "\n";
  }

  if (!opt.csv_path.empty()) {
    emitCsv(opt.csv_path, allResults);
    std::cout << "Wrote CSV to " << opt.csv_path << "\n";
  }

  launcher.freeDeviceBuffer(resultsBuffer);
  launcher.unLoadKernel(kernelId);
  launcher.tearDown();
  return 0;
}
