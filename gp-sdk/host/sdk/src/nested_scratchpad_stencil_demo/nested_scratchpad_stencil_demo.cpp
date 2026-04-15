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
#include "gpsdk_nested_scratchpad_example.h"
#include "gpsdk_star_scratchpad.h"

namespace {

using gpsdk::examples::nested_scratchpad::KernelArguments;
using gpsdk::examples::nested_scratchpad::Result;

struct Options {
  fs::path kernel_path = "";
  int kernel_launch_timeout = 90;
  std::string device_type = "silicon";
  uint32_t shire_mask = 0x200;
};

Options parse_args(int argc, char* const* argv, std::vector<char*>& nextlevel) {
  static constexpr const char* help_msg =
    "Usage: [options]\n\n"
    "Launch the practical nested scratchpad stencil demo and verify the result on the host.\n\n"
    "Required:\n"
    "  -k, --kernel_path             path to the nested scratchpad stencil kernel elf file.\n\n"
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

  std::byte* allocateDeviceBuffer(size_t size, uint32_t deviceIdx = 0) {
    return runtime_->mallocDevice(devices_.at(deviceIdx), size);
  }

  void freeDeviceBuffer(std::byte* deviceBuffer, uint32_t deviceIdx = 0) {
    runtime_->freeDevice(devices_.at(deviceIdx), deviceBuffer);
  }

  template <typename T> void writeObject(std::byte* deviceAddress, const T& value, uint32_t deviceIdx = 0) {
    runtime_->memcpyHostToDevice(defaultStreams_[deviceIdx], reinterpret_cast<const std::byte*>(&value), deviceAddress,
                                 sizeof(T));
    const auto timeout = std::chrono::seconds(5);
    if (!runtime_->waitForStream(defaultStreams_[deviceIdx], timeout)) {
      throw std::runtime_error("Timed out while writing device memory");
    }
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

struct ExpectedResult {
  uint64_t checksum = 0ULL;
  uint32_t sampleTopLeft = 0U;
  uint32_t sampleCenter = 0U;
  uint32_t sampleBottomRight = 0U;
};

ExpectedResult buildExpectedResult() {
  ExpectedResult expected;

  for (uint32_t y = 0U; y < gpsdk::examples::nested_scratchpad::kTileHeight; ++y) {
    for (uint32_t x = 0U; x < gpsdk::examples::nested_scratchpad::kTileWidth; ++x) {
      expected.checksum += gpsdk::examples::nested_scratchpad::linearStencilValue(x, y);
    }
  }

  expected.sampleTopLeft = gpsdk::examples::nested_scratchpad::linearStencilValue(1U, 1U);
  expected.sampleCenter = gpsdk::examples::nested_scratchpad::linearStencilValue(
    gpsdk::examples::nested_scratchpad::kTileWidth / 2U, gpsdk::examples::nested_scratchpad::kTileHeight / 2U);
  expected.sampleBottomRight = gpsdk::examples::nested_scratchpad::linearStencilValue(
    gpsdk::examples::nested_scratchpad::kTileWidth - 2U, gpsdk::examples::nested_scratchpad::kTileHeight - 2U);

  return expected;
}

bool hasArgument(const std::vector<char*>& args, const char* target) {
  for (const char* arg : args) {
    if ((arg != nullptr) && (std::strcmp(arg, target) == 0)) {
      return true;
    }
  }
  return false;
}

void printShireList(const char* label, const uint8_t* shires, uint32_t count) {
  std::cout << std::dec << label << ":";
  for (uint32_t idx = 0U; idx < count; ++idx) {
    std::cout << " " << static_cast<uint32_t>(shires[idx]);
  }
  std::cout << "\n";
}

} // namespace

int main(int argc, char** argv) {
  std::vector<char*> argvPendingToParse{argv[0]};
  const Options opt = parse_args(argc, argv, argvPendingToParse);

  static char nestedFlag[] = "--scratchpad_nested_star";
  if (!hasArgument(argvPendingToParse, nestedFlag)) {
    argvPendingToParse.emplace_back(nestedFlag);
  }

  Config config{modeFromString(opt.device_type), 1};
  config.dump();

  Launcher launcher(config, static_cast<int>(argvPendingToParse.size()), argvPendingToParse.data(), false);

  std::byte* resultBuffer = nullptr;
  bool kernelLoaded = false;
  rt::KernelId kernelId{};

  try {
    launcher.initialize();

    const auto selection = launcher.resolveScratchpadClusterSelection(
      opt.shire_mask, gpsdk::star_scratchpad::ClusterLayout::NestedStar);
    if (!selection.valid()) {
      std::cout << "Unable to resolve a nested scratchpad cluster for requested mask 0x" << std::hex
                << opt.shire_mask << std::dec << ".\n";
      launcher.tearDown();
      return 2;
    }

    std::cout << std::dec << "Topology cache: " << launcher.getTopologyCachePath() << "\n";
    std::cout << "Nested center shire: " << selection.effectiveCenterShire;
    if (selection.centerShifted) {
      std::cout << " (shifted)";
    }
    std::cout << "\n";
    printShireList("Relays", selection.relayShires.data(), selection.relayCount);
    printShireList("Leaves", selection.auxiliaryShires.data(), selection.auxiliaryCount);

    resultBuffer = launcher.allocateDeviceBuffer(sizeof(Result));
    launcher.writeObject(resultBuffer, Result{});

    kernelId = launcher.loadKernel(opt.kernel_path.string());
    kernelLoaded = true;

    KernelArguments args;
    args.resultAddress = reinterpret_cast<uint64_t>(resultBuffer);

    launcher.kernelLaunch(kernelId, &args, nullptr, 0, 0, opt.shire_mask);
    launcher.waitKernelCompletion(std::chrono::seconds(opt.kernel_launch_timeout));

    if (launcher.checkKernelExecutionErrors()) {
      if (resultBuffer != nullptr) {
        launcher.freeDeviceBuffer(resultBuffer);
      }
      launcher.unLoadKernel(kernelId);
      launcher.tearDown();
      return 3;
    }

    const auto result = launcher.readObject<Result>(resultBuffer);
    const auto expected = buildExpectedResult();

    if (result.magic != gpsdk::examples::nested_scratchpad::kResultMagic) {
      std::cout << "Invalid result magic 0x" << std::hex << result.magic << std::dec << ".\n";
      launcher.freeDeviceBuffer(resultBuffer);
      launcher.unLoadKernel(kernelId);
      launcher.tearDown();
      return 4;
    }

    if ((result.width != gpsdk::examples::nested_scratchpad::kTileWidth) ||
        (result.height != gpsdk::examples::nested_scratchpad::kTileHeight)) {
      std::cout << "Unexpected tile shape " << result.width << "x" << result.height << ".\n";
      launcher.freeDeviceBuffer(resultBuffer);
      launcher.unLoadKernel(kernelId);
      launcher.tearDown();
      return 5;
    }

    if ((result.activeThreads == 0U) ||
        (result.activeThreads > gpsdk::examples::nested_scratchpad::kMaxThreadChecksums)) {
      std::cout << "Unexpected active thread count " << result.activeThreads << ".\n";
      launcher.freeDeviceBuffer(resultBuffer);
      launcher.unLoadKernel(kernelId);
      launcher.tearDown();
      return 6;
    }

    if (result.centerShire != selection.effectiveCenterShire) {
      std::cout << "Kernel reported center shire " << result.centerShire << ", expected "
                << selection.effectiveCenterShire << ".\n";
      launcher.freeDeviceBuffer(resultBuffer);
      launcher.unLoadKernel(kernelId);
      launcher.tearDown();
      return 7;
    }

    uint64_t threadChecksumTotal = 0ULL;
    for (uint32_t idx = 0U; idx < result.activeThreads; ++idx) {
      threadChecksumTotal += result.threadChecksums[idx];
    }

    if (threadChecksumTotal != result.checksum) {
      std::cout << "Per-thread checksum total mismatch: got " << threadChecksumTotal << " expected " << result.checksum
                << ".\n";
      launcher.freeDeviceBuffer(resultBuffer);
      launcher.unLoadKernel(kernelId);
      launcher.tearDown();
      return 8;
    }

    if ((result.checksum != expected.checksum) || (result.sampleTopLeft != expected.sampleTopLeft) ||
        (result.sampleCenter != expected.sampleCenter) || (result.sampleBottomRight != expected.sampleBottomRight)) {
      std::cout << "Verification mismatch.\n"
                << "  checksum: got " << result.checksum << " expected " << expected.checksum << "\n"
                << "  sampleTopLeft: got 0x" << std::hex << result.sampleTopLeft << " expected 0x"
                << expected.sampleTopLeft << "\n"
                << "  sampleCenter: got 0x" << result.sampleCenter << " expected 0x" << expected.sampleCenter << "\n"
                << "  sampleBottomRight: got 0x" << result.sampleBottomRight << " expected 0x"
                << expected.sampleBottomRight << std::dec << "\n";
      launcher.freeDeviceBuffer(resultBuffer);
      launcher.unLoadKernel(kernelId);
      launcher.tearDown();
      return 9;
    }

    std::cout << std::dec << "Active threads: " << result.activeThreads << "\n";
    std::cout << "Verified checksum: 0x" << std::hex << result.checksum << std::dec << "\n";
    std::cout << "Verified samples: top-left=0x" << std::hex << result.sampleTopLeft << " center=0x"
              << result.sampleCenter << " bottom-right=0x" << result.sampleBottomRight << std::dec << "\n";

    launcher.freeDeviceBuffer(resultBuffer);
    launcher.unLoadKernel(kernelId);
    launcher.tearDown();
    return 0;
  } catch (const std::exception& ex) {
    std::cout << "Stencil demo failed: " << ex.what() << "\n";
    if ((resultBuffer != nullptr) && (launcher.getNumDevices() != 0U)) {
      launcher.freeDeviceBuffer(resultBuffer);
    }
    if (kernelLoaded) {
      launcher.unLoadKernel(kernelId);
    }
    launcher.tearDown();
    return 10;
  }
}
