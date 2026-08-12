//******************************************************************************
// Copyright (c) 2026 Ainekko, Co.
// SPDX-License-Identifier: Apache-2.0
//------------------------------------------------------------------------------

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <getopt.h>
#include <stdexcept>
#include <string>
#include <vector>

#include "GenericLauncher.h"
#include "gpsdk_nested_scratchpad_chimera_gemv.h"

namespace {

using gpsdk::examples::chimera_gemv::KernelArguments;
using gpsdk::examples::chimera_gemv::Result;

struct Options {
  fs::path kernel_path = "";
  int kernel_launch_timeout = 180;
  std::string device_type = "silicon";
  uint32_t shire_mask = 0x200;
};

Options parse_args(int argc, char* const* argv, std::vector<char*>& nextlevel) {
  static constexpr const char* help_msg =
    "Usage: [options]\n\n"
    "Launch the nested scratchpad chimera GEMV demo and verify the result on the host.\n\n"
    "Required:\n"
    "  -k, --kernel_path             path to the chimera GEMV kernel elf file.\n\n"
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

struct VerificationSummary {
  double checksum = 0.0;
  float maxAbsDiff = 0.0f;
  float sampleFirst = 0.0f;
  float sampleMid = 0.0f;
  float sampleLast = 0.0f;
  double tensorChecksum = 0.0;
  float tensorMaxAbsDiff = 0.0f;
  float tensorSampleFirst = 0.0f;
  float tensorSampleMid = 0.0f;
};

VerificationSummary verifyResult(const Result& result) {
  VerificationSummary summary;

  for (uint32_t row = 0U; row < gpsdk::examples::chimera_gemv::kRows; ++row) {
    float expected = gpsdk::examples::chimera_gemv::kBeta * gpsdk::examples::chimera_gemv::makeInitialOutputValue(row);
    for (uint32_t col = 0U; col < gpsdk::examples::chimera_gemv::kCols; ++col) {
      expected += gpsdk::examples::chimera_gemv::kAlpha *
                  (gpsdk::examples::chimera_gemv::makeMatrixValue(row, col) *
                   gpsdk::examples::chimera_gemv::makeVectorValue(col));
    }

    const float got = result.outputs[row];
    summary.checksum += static_cast<double>(got);
    summary.maxAbsDiff = std::max(summary.maxAbsDiff, std::fabs(got - expected));
  }

  summary.sampleFirst = result.outputs[0];
  summary.sampleMid = result.outputs[gpsdk::examples::chimera_gemv::kRows / 2U];
  summary.sampleLast = result.outputs[gpsdk::examples::chimera_gemv::kRows - 1U];

  const uint32_t tensorRowsToCheck[] = {0U, gpsdk::examples::chimera_gemv::kTensorProbeRows / 2U};
  for (const uint32_t row : tensorRowsToCheck) {
    float expected = 0.0f;
    for (uint32_t inner = 0U; inner < gpsdk::examples::chimera_gemv::kTensorProbeK; ++inner) {
      expected += gpsdk::examples::chimera_gemv::makeMatrixValue(row, inner) *
                  gpsdk::examples::chimera_gemv::makeVectorValue(inner);
    }

    const float got = result.tensorProbe[row * gpsdk::examples::chimera_gemv::kTensorProbeCols];
    summary.tensorChecksum += static_cast<double>(got);
    summary.tensorMaxAbsDiff = std::max(summary.tensorMaxAbsDiff, std::fabs(got - expected));
  }

  summary.tensorSampleFirst = result.tensorProbe[0];
  summary.tensorSampleMid = result.tensorProbe[(gpsdk::examples::chimera_gemv::kTensorProbeRows / 2U) *
                                               gpsdk::examples::chimera_gemv::kTensorProbeCols];
  return summary;
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
    if (result.magic != gpsdk::examples::chimera_gemv::kResultMagic) {
      std::cout << "Invalid result magic 0x" << std::hex << result.magic << std::dec << ".\n";
      launcher.freeDeviceBuffer(resultBuffer);
      launcher.unLoadKernel(kernelId);
      launcher.tearDown();
      return 4;
    }

    if ((result.rows != gpsdk::examples::chimera_gemv::kRows) || (result.cols != gpsdk::examples::chimera_gemv::kCols)) {
      std::cout << "Unexpected GEMV shape " << result.rows << "x" << result.cols << ".\n";
      launcher.freeDeviceBuffer(resultBuffer);
      launcher.unLoadKernel(kernelId);
      launcher.tearDown();
      return 5;
    }

    if ((result.activeThreads == 0U) || (result.centerShire != selection.effectiveCenterShire)) {
      std::cout << "Unexpected runtime metadata.\n";
      launcher.freeDeviceBuffer(resultBuffer);
      launcher.unLoadKernel(kernelId);
      launcher.tearDown();
      return 6;
    }

    if ((result.tensorProbeRows != gpsdk::examples::chimera_gemv::kTensorProbeRows) ||
        (result.tensorProbeCols != gpsdk::examples::chimera_gemv::kTensorProbeCols)) {
      std::cout << "Unexpected tensor probe shape " << result.tensorProbeRows << "x" << result.tensorProbeCols
                << ".\n";
      launcher.freeDeviceBuffer(resultBuffer);
      launcher.unLoadKernel(kernelId);
      launcher.tearDown();
      return 6;
    }

    const auto summary = verifyResult(result);
    constexpr float kTolerance = 1.0e-3f;
    if ((summary.maxAbsDiff > kTolerance) || (summary.tensorMaxAbsDiff > kTolerance)) {
      std::cout << "Verification mismatch.\n"
                << "  gemvMaxAbsDiff: " << summary.maxAbsDiff << " tolerance: " << kTolerance << "\n"
                << "  sampleFirst: " << summary.sampleFirst << "\n"
                << "  sampleMid: " << summary.sampleMid << "\n"
                << "  sampleLast: " << summary.sampleLast << "\n"
                << "  tensorMaxAbsDiff: " << summary.tensorMaxAbsDiff << "\n"
                << "  tensorSampleFirst: " << summary.tensorSampleFirst << "\n"
                << "  tensorSampleMid: " << summary.tensorSampleMid << "\n";
      launcher.freeDeviceBuffer(resultBuffer);
      launcher.unLoadKernel(kernelId);
      launcher.tearDown();
      return 7;
    }

    std::cout << std::dec << "Active threads: " << result.activeThreads << "\n";
    std::cout << "Verified GEMV checksum: " << summary.checksum << "\n";
    std::cout << "Verified GEMV max abs diff: " << summary.maxAbsDiff << "\n";
    std::cout << "Verified GEMV samples: first=" << summary.sampleFirst << " mid=" << summary.sampleMid
              << " last=" << summary.sampleLast << "\n";
    std::cout << "Verified tensor checksum: " << summary.tensorChecksum << "\n";
    std::cout << "Verified tensor max abs diff: " << summary.tensorMaxAbsDiff << "\n";
    std::cout << "Verified tensor samples: first=" << summary.tensorSampleFirst
              << " mid=" << summary.tensorSampleMid << "\n";

    launcher.freeDeviceBuffer(resultBuffer);
    launcher.unLoadKernel(kernelId);
    launcher.tearDown();
    return 0;
  } catch (const std::exception& ex) {
    std::cout << "Chimera GEMV demo failed: " << ex.what() << "\n";
    if ((resultBuffer != nullptr) && (launcher.getNumDevices() != 0U)) {
      launcher.freeDeviceBuffer(resultBuffer);
    }
    if (kernelLoaded) {
      launcher.unLoadKernel(kernelId);
    }
    launcher.tearDown();
    return 8;
  }
}
