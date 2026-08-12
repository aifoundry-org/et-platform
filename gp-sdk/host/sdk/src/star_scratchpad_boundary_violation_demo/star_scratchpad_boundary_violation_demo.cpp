//******************************************************************************
// Copyright (c) 2026 Ainekko, Co.
// SPDX-License-Identifier: Apache-2.0
//------------------------------------------------------------------------------

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <getopt.h>
#include <stdexcept>
#include <string>
#include <vector>

#include "GenericLauncher.h"
#include "gpsdk_star_scratchpad_boundary_violation.h"

namespace {

using gpsdk::examples::star_scratchpad_boundary_violation::KernelArguments;
using gpsdk::examples::star_scratchpad_boundary_violation::Result;
using gpsdk::examples::star_scratchpad_boundary_violation::ViolationMode;

struct Options {
  fs::path kernel_path = "";
  int kernel_launch_timeout = 30;
  std::string device_type = "silicon";
  uint32_t shire_mask = 0x200;
};

Options parse_args(int argc, char* const* argv, std::vector<char*>& nextlevel) {
  static constexpr const char* help_msg =
    "Usage: [options]\n\n"
    "Launch a kernel that intentionally requests invalid contiguous scratchpad windows and verify\n"
    "that the pool API rejects them.\n\n"
    "Required:\n"
    "  -k, --kernel_path             path to the boundary violation kernel elf file.\n\n"
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

bool hasArgument(const std::vector<char*>& args, const char* target) {
  for (const char* arg : args) {
    if ((arg != nullptr) && (std::strcmp(arg, target) == 0)) {
      return true;
    }
  }
  return false;
}

class Launcher : public GenericLauncher {
public:
  using GenericLauncher::GenericLauncher;

  std::byte* allocateDeviceBuffer(size_t size, uint32_t deviceIdx = 0) {
    return runtime_->mallocDevice(devices_.at(deviceIdx), size);
  }

  void freeDeviceBuffer(std::byte* deviceBuffer, uint32_t deviceIdx = 0) {
    runtime_->freeDevice(devices_.at(deviceIdx), deviceBuffer);
  }

  template <typename T> void writeObject(std::byte* deviceAddress, const T& value, uint32_t deviceIdx = 0) {
    runtime_->memcpyHostToDevice(defaultStreams_[deviceIdx], reinterpret_cast<const std::byte*>(&value), deviceAddress,
                                 sizeof(T));
    if (!runtime_->waitForStream(defaultStreams_[deviceIdx], std::chrono::seconds(5))) {
      throw std::runtime_error("Timed out while writing device memory");
    }
  }

  template <typename T> T readObject(std::byte* deviceAddress, uint32_t deviceIdx = 0) {
    T value{};
    runtime_->memcpyDeviceToHost(defaultStreams_[deviceIdx], deviceAddress, reinterpret_cast<std::byte*>(&value),
                                 sizeof(T));
    if (!runtime_->waitForStream(defaultStreams_[deviceIdx], std::chrono::seconds(5))) {
      throw std::runtime_error("Timed out while reading device memory");
    }
    return value;
  }
};

bool runViolationCase(const Config& config, const std::vector<char*>& forwardedArgs, const Options& opt,
                      ViolationMode mode) {
  Launcher launcher(config, static_cast<int>(forwardedArgs.size()), const_cast<char**>(forwardedArgs.data()), false);
  std::byte* resultBuffer = nullptr;
  bool kernelLoaded = false;
  rt::KernelId kernelId{};

  try {
    launcher.initialize();

    resultBuffer = launcher.allocateDeviceBuffer(sizeof(Result));
    launcher.writeObject(resultBuffer, Result{});

    kernelId = launcher.loadKernel(opt.kernel_path.string());
    kernelLoaded = true;

    KernelArguments args;
    args.resultAddress = reinterpret_cast<uint64_t>(resultBuffer);
    args.violationMode = static_cast<uint32_t>(mode);

    launcher.kernelLaunch(kernelId, &args, nullptr, 0, 0, opt.shire_mask);
    launcher.waitKernelCompletion(std::chrono::seconds(opt.kernel_launch_timeout));

    const bool errored = launcher.checkKernelExecutionErrors();
    if (!errored) {
      const auto result = launcher.readObject<Result>(resultBuffer);
      std::cout << "Unexpected success for "
                << gpsdk::examples::star_scratchpad_boundary_violation::violationModeName(mode)
                << ". started=" << result.started << " completed=" << result.completed << " logicalOffset=0x"
                << std::hex << result.logicalOffset << " requestedBytes=0x" << result.requestedBytes
                << " observedAddress=0x" << result.observedAddress << std::dec << "\n";
    } else {
      std::cout << "Observed expected kernel failure for "
                << gpsdk::examples::star_scratchpad_boundary_violation::violationModeName(mode) << ".\n";
    }

    launcher.freeDeviceBuffer(resultBuffer);
    launcher.unLoadKernel(kernelId);
    launcher.tearDown();
    return errored;
  } catch (const std::exception& ex) {
    std::cout << "Violation case failed unexpectedly for "
              << gpsdk::examples::star_scratchpad_boundary_violation::violationModeName(mode) << ": " << ex.what()
              << "\n";
    if ((resultBuffer != nullptr) && (launcher.getNumDevices() != 0U)) {
      launcher.freeDeviceBuffer(resultBuffer);
    }
    if (kernelLoaded) {
      launcher.unLoadKernel(kernelId);
    }
    launcher.tearDown();
    return false;
  }
}

} // namespace

int main(int argc, char** argv) {
  std::vector<char*> argvPendingToParse{argv[0]};
  const Options opt = parse_args(argc, argv, argvPendingToParse);

  Config config{modeFromString(opt.device_type), 1};
  config.dump();

  const ViolationMode modes[] = {
    ViolationMode::OversizeContiguousRequest,
    ViolationMode::CrossShardContiguousRequest,
  };

  bool allFaulted = true;
  for (const auto mode : modes) {
    allFaulted &= runViolationCase(config, argvPendingToParse, opt, mode);
  }

  return allFaulted ? 0 : 1;
}
