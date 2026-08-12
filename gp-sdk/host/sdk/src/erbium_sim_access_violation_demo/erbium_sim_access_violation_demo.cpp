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
#include "gpsdk_erbium_sim_access_violation.h"

namespace {

using gpsdk::examples::erbium_sim_access_violation::AccessMode;
using gpsdk::examples::erbium_sim_access_violation::KernelArguments;
using gpsdk::examples::erbium_sim_access_violation::Result;

struct Options {
  fs::path kernel_path = "";
  int kernel_launch_timeout = 30;
  std::string device_type = "silicon";
  uint32_t shire_mask = 0x200;
};

Options parse_args(int argc, char* const* argv, std::vector<char*>& nextlevel) {
  static constexpr const char* help_msg =
    "Usage: [options]\n\n"
    "Launch a kernel that intentionally accesses SCP outside the selected cluster and verify\n"
    "that --erbium_sim turns those accesses into kernel failures.\n\n"
    "Required:\n"
    "  -k, --kernel_path             path to the violation kernel elf file.\n\n"
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

gpsdk::star_scratchpad::ClusterLayout getRequestedClusterLayout(const std::vector<char*>& args) {
  static constexpr char starFlag[] = "--scratchpad_star";
  static constexpr char blockFlag[] = "--scratchpad_block";
  static constexpr char nestedFlag[] = "--scratchpad_nested_star";

  const bool star = hasArgument(args, starFlag);
  const bool block = hasArgument(args, blockFlag);
  const bool nested = hasArgument(args, nestedFlag);
  const auto count = static_cast<uint32_t>(star) + static_cast<uint32_t>(block) + static_cast<uint32_t>(nested);
  if (count != 1U) {
    throw std::runtime_error("Pass exactly one of --scratchpad_star, --scratchpad_block, or --scratchpad_nested_star");
  }

  if (nested) {
    return gpsdk::star_scratchpad::ClusterLayout::NestedStar;
  }
  return block ? gpsdk::star_scratchpad::ClusterLayout::Block : gpsdk::star_scratchpad::ClusterLayout::Star;
}

uint32_t findIllegalShire(const gpsdk::star_scratchpad::ClusterSelection& selection) {
  for (uint32_t shire = 0U; shire < 32U; ++shire) {
    if (((selection.launchedShireMask >> shire) & 0x1ULL) == 0ULL) {
      return shire;
    }
  }
  throw std::runtime_error("Could not find a scratchpad shire outside the selected cluster");
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
                      gpsdk::star_scratchpad::ClusterLayout layout, AccessMode mode) {
  Launcher launcher(config, static_cast<int>(forwardedArgs.size()), const_cast<char**>(forwardedArgs.data()), false);
  std::byte* resultBuffer = nullptr;
  bool kernelLoaded = false;
  rt::KernelId kernelId{};

  try {
    launcher.initialize();
    const auto selection = launcher.resolveScratchpadClusterSelection(opt.shire_mask, layout);
    if (!selection.valid()) {
      throw std::runtime_error("Unable to resolve scratchpad cluster selection");
    }

    const auto illegalShire = findIllegalShire(selection);
    resultBuffer = launcher.allocateDeviceBuffer(sizeof(Result));
    launcher.writeObject(resultBuffer, Result{});

    kernelId = launcher.loadKernel(opt.kernel_path.string());
    kernelLoaded = true;

    KernelArguments args;
    args.resultAddress = reinterpret_cast<uint64_t>(resultBuffer);
    args.targetShire = illegalShire;
    args.accessMode = static_cast<uint32_t>(mode);

    launcher.kernelLaunch(kernelId, &args, nullptr, 0, 0, opt.shire_mask);
    launcher.waitKernelCompletion(std::chrono::seconds(opt.kernel_launch_timeout));

    const bool errored = launcher.checkKernelExecutionErrors();
    if (!errored) {
      const auto result = launcher.readObject<Result>(resultBuffer);
      std::cout << "Unexpected success for " << gpsdk::examples::erbium_sim_access_violation::accessModeName(mode)
                << ". targetShire=" << illegalShire << " started=" << result.started
                << " completed=" << result.completed << " observedWord=0x" << std::hex << result.observedWord
                << std::dec << "\n";
    } else {
      std::cout << "Observed expected kernel failure for "
                << gpsdk::examples::erbium_sim_access_violation::accessModeName(mode) << " at out-of-cluster shire "
                << illegalShire << ".\n";
    }

    launcher.freeDeviceBuffer(resultBuffer);
    launcher.unLoadKernel(kernelId);
    launcher.tearDown();
    return errored;
  } catch (const std::exception& ex) {
    std::cout << "Violation case failed unexpectedly for "
              << gpsdk::examples::erbium_sim_access_violation::accessModeName(mode) << ": " << ex.what() << "\n";
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

  static char erbiumSimFlag[] = "--erbium_sim";
  if (!hasArgument(argvPendingToParse, erbiumSimFlag)) {
    argvPendingToParse.emplace_back(erbiumSimFlag);
  }

  const auto layout = getRequestedClusterLayout(argvPendingToParse);
  Config config{modeFromString(opt.device_type), 1};
  config.dump();

  const AccessMode modes[] = {
    AccessMode::AtomicRead,
    AccessMode::GlobalMemcpy,
    AccessMode::TensorLoad,
  };

  bool allFaulted = true;
  for (const auto mode : modes) {
    allFaulted &= runViolationCase(config, argvPendingToParse, opt, layout, mode);
  }

  return allFaulted ? 0 : 1;
}
