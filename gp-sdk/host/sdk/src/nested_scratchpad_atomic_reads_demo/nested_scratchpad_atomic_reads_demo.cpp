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
#include "gpsdk_nested_scratchpad_atomic_reads.h"

namespace {

using gpsdk::examples::nested_atomic_reads::KernelArguments;
using gpsdk::examples::nested_atomic_reads::Result;

struct Options {
  fs::path kernel_path = "";
  int kernel_launch_timeout = 90;
  std::string device_type = "silicon";
  uint32_t shire_mask = 0x200;
};

Options parse_args(int argc, char* const* argv, std::vector<char*>& nextlevel) {
  static constexpr const char* help_msg =
    "Usage: [options]\n\n"
    "Launch the nested scratchpad atomic-read validation and verify concurrent global reads on the host.\n\n"
    "Required:\n"
    "  -k, --kernel_path             path to the nested scratchpad atomic-read kernel elf file.\n\n"
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

struct ExpectedResult {
  uint64_t aggregateChecksum = 0ULL;
  uint64_t aggregateReadCount = 0ULL;
  uint32_t combinedVisitedLeafMask = 0U;
  uint64_t firstThreadChecksum = 0ULL;
  uint64_t lastThreadChecksum = 0ULL;
  std::array<uint64_t, gpsdk::examples::nested_atomic_reads::kLeafCount> perLeafReadCounts = {};
};

ExpectedResult buildExpectedResult(
  uint32_t centerShire, uint32_t activeThreads,
  const gpsdk::star_scratchpad::ClusterSelection& selection) {
  ExpectedResult expected;

  for (uint32_t threadId = 0U; threadId < activeThreads; ++threadId) {
    uint64_t checksum = 0ULL;
    uint32_t visitedLeafMask = 0U;

    for (uint32_t round = 0U; round < gpsdk::examples::nested_atomic_reads::kRounds; ++round) {
      const uint32_t leafIndex = (threadId + round) % gpsdk::examples::nested_atomic_reads::kLeafCount;
      const uint32_t slotIndex =
        (threadId + (round * activeThreads)) % gpsdk::examples::nested_atomic_reads::kSlotsPerLeaf;
      const uint32_t leafShire = selection.auxiliaryShires[leafIndex];
      const auto value = gpsdk::examples::nested_atomic_reads::makeLeafValue(centerShire, leafShire, slotIndex);
      checksum += gpsdk::examples::nested_atomic_reads::checksumStep(value, threadId, round, leafIndex, slotIndex);
      visitedLeafMask |= (1U << leafIndex);
      ++expected.perLeafReadCounts[leafIndex];
    }

    if (threadId == 0U) {
      expected.firstThreadChecksum = checksum;
    }
    if (threadId + 1U == activeThreads) {
      expected.lastThreadChecksum = checksum;
    }
    expected.aggregateReadCount += gpsdk::examples::nested_atomic_reads::kRounds;
    expected.combinedVisitedLeafMask |= visitedLeafMask;
    expected.aggregateChecksum += checksum;
  }

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
      launcher.freeDeviceBuffer(resultBuffer);
      launcher.unLoadKernel(kernelId);
      launcher.tearDown();
      return 3;
    }

    const auto result = launcher.readObject<Result>(resultBuffer);
    if (result.magic != gpsdk::examples::nested_atomic_reads::kResultMagic) {
      std::cout << "Invalid result magic 0x" << std::hex << result.magic << std::dec << ".\n";
      launcher.freeDeviceBuffer(resultBuffer);
      launcher.unLoadKernel(kernelId);
      launcher.tearDown();
      return 4;
    }

    if ((result.activeThreads == 0U) ||
        (result.activeThreads > gpsdk::examples::nested_atomic_reads::kMaxThreadResults)) {
      std::cout << "Unexpected active thread count " << result.activeThreads << ".\n";
      launcher.freeDeviceBuffer(resultBuffer);
      launcher.unLoadKernel(kernelId);
      launcher.tearDown();
      return 5;
    }

    if ((result.centerShire != selection.effectiveCenterShire) ||
        (result.rounds != gpsdk::examples::nested_atomic_reads::kRounds) ||
        (result.slotsPerLeaf != gpsdk::examples::nested_atomic_reads::kSlotsPerLeaf)) {
      std::cout << "Unexpected kernel metadata.\n";
      launcher.freeDeviceBuffer(resultBuffer);
      launcher.unLoadKernel(kernelId);
      launcher.tearDown();
      return 6;
    }

    for (uint32_t leaf = 0U; leaf < gpsdk::examples::nested_atomic_reads::kLeafCount; ++leaf) {
      if (result.leafShires[leaf] != selection.auxiliaryShires[leaf]) {
        std::cout << "Leaf shire mismatch at index " << leaf << ": got " << static_cast<uint32_t>(result.leafShires[leaf])
                  << " expected " << static_cast<uint32_t>(selection.auxiliaryShires[leaf]) << ".\n";
        launcher.freeDeviceBuffer(resultBuffer);
        launcher.unLoadKernel(kernelId);
        launcher.tearDown();
        return 7;
      }
    }

    const auto expected = buildExpectedResult(result.centerShire, result.activeThreads, selection);

    if (result.aggregateChecksum != expected.aggregateChecksum) {
      std::cout << "Aggregate checksum mismatch.\n"
                << "  activeThreads: " << std::dec << result.activeThreads << "\n"
                << "  aggregateChecksum: got 0x" << std::hex << result.aggregateChecksum << " expected 0x"
                << expected.aggregateChecksum << "\n"
                << "  aggregateReadCount: got " << std::dec << result.aggregateReadCount << " expected "
                << expected.aggregateReadCount << "\n"
                << "  combinedVisitedLeafMask: got 0x" << std::hex << result.combinedVisitedLeafMask
                << " expected 0x" << expected.combinedVisitedLeafMask << "\n"
                << "  firstThreadChecksum: got 0x" << result.firstThreadChecksum << " expected 0x"
                << expected.firstThreadChecksum << "\n"
                << "  lastThreadChecksum: got 0x" << result.lastThreadChecksum << " expected 0x"
                << expected.lastThreadChecksum << std::dec << "\n";
      launcher.freeDeviceBuffer(resultBuffer);
      launcher.unLoadKernel(kernelId);
      launcher.tearDown();
      return 8;
    }

    if ((result.aggregateReadCount != expected.aggregateReadCount) ||
        (result.combinedVisitedLeafMask != expected.combinedVisitedLeafMask) ||
        (result.lastActiveThreadId + 1U != result.activeThreads) ||
        (result.firstThreadChecksum != expected.firstThreadChecksum) ||
        (result.lastThreadChecksum != expected.lastThreadChecksum)) {
      std::cout << "Aggregate metadata mismatch.\n"
                << "  aggregateReadCount: got " << result.aggregateReadCount << " expected "
                << expected.aggregateReadCount << "\n"
                << "  combinedVisitedLeafMask: got 0x" << std::hex << result.combinedVisitedLeafMask
                << " expected 0x" << expected.combinedVisitedLeafMask << "\n"
                << "  firstThreadChecksum: got 0x" << result.firstThreadChecksum << " expected 0x"
                << expected.firstThreadChecksum << "\n"
                << "  lastThreadChecksum: got 0x" << result.lastThreadChecksum << " expected 0x"
                << expected.lastThreadChecksum << std::dec << "\n";
      launcher.freeDeviceBuffer(resultBuffer);
      launcher.unLoadKernel(kernelId);
      launcher.tearDown();
      return 9;
    }

    for (uint32_t leaf = 0U; leaf < gpsdk::examples::nested_atomic_reads::kLeafCount; ++leaf) {
      if (result.perLeafReadCounts[leaf] != expected.perLeafReadCounts[leaf]) {
        std::cout << "Per-leaf read-count mismatch at leaf " << leaf << ": got " << result.perLeafReadCounts[leaf]
                  << " expected " << expected.perLeafReadCounts[leaf] << ".\n";
        launcher.freeDeviceBuffer(resultBuffer);
        launcher.unLoadKernel(kernelId);
        launcher.tearDown();
        return 10;
      }
    }

    std::cout << std::dec << "Active threads: " << result.activeThreads << "\n";
    std::cout << "Verified aggregate checksum: 0x" << std::hex << result.aggregateChecksum << std::dec << "\n";
    std::cout << "Verified aggregate reads: " << result.aggregateReadCount << "\n";
    std::cout << "Verified per-leaf reads:";
    for (uint32_t leaf = 0U; leaf < gpsdk::examples::nested_atomic_reads::kLeafCount; ++leaf) {
      std::cout << " [" << leaf << "->shire " << static_cast<uint32_t>(result.leafShires[leaf]) << ": "
                << result.perLeafReadCounts[leaf] << "]";
    }
    std::cout << "\n";

    launcher.freeDeviceBuffer(resultBuffer);
    launcher.unLoadKernel(kernelId);
    launcher.tearDown();
    return 0;
  } catch (const std::exception& ex) {
    std::cout << "Atomic read validation failed: " << ex.what() << "\n";
    if ((resultBuffer != nullptr) && (launcher.getNumDevices() != 0U)) {
      launcher.freeDeviceBuffer(resultBuffer);
    }
    if (kernelLoaded) {
      launcher.unLoadKernel(kernelId);
    }
    launcher.tearDown();
    return 11;
  }
}
