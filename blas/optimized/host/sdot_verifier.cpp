/*-------------------------------------------------------------------------
 * Copyright (c) 2026 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------
 */

#include "GenericLauncher.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <getopt.h>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct DotArgs {
  uint64_t numElements;
  const float* x;
  const float* y;
  float* partials;
  float* res;
} __attribute__((packed));

struct Options {
  fs::path kernel_dir = "";
  fs::path host_results_path = "blas/optimized/host_sdot_results.md";
  fs::path device_results_path = "blas/optimized/device_sdot_results.md";
  int kernel_launch_timeout = 30;
  std::string device_type = "sysemu";
  size_t num_elements = 256;
  double epsilon = 1.0e-5;
};

struct Result {
  bool ok = false;
  fs::path kernel_artifact;
  std::vector<float> input_x;
  std::vector<float> input_y;
  float output = 0.0f;
};

Options parseArgs(int argc, char* const* argv, std::vector<char*>& nextlevel) {
  static constexpr const char* helpMsg =
    "Usage: [options]\n\n"
    "Verifier for the optimized FP32 SDOT kernel.\n\n"
    "Optional switches:\n"
    "  -k, --kernel_dir             directory or explicit path for the SDOT kernel artifact\n"
    "      --host_results_path      markdown file for host reference output\n"
    "      --device_results_path    markdown file for device output\n"
    "  -t, --kernel_launch_timeout  timeout (in seconds) to wait for kernel completion\n"
    "  -d, --device_type            device type to use (sysemu, fake, silicon)\n"
    "  -n, --num_elements           number of elements in dummy vectors\n"
    "  -e, --epsilon                comparison tolerance for float results\n";

  static constexpr const char* shortOpts = "k:t:d:n:e:h";
  static const std::vector<option> longOpts{
    {"kernel_dir", required_argument, nullptr, 'k'},
    {"host_results_path", required_argument, nullptr, 1000},
    {"device_results_path", required_argument, nullptr, 1001},
    {"kernel_launch_timeout", required_argument, nullptr, 't'},
    {"device_type", required_argument, nullptr, 'd'},
    {"num_elements", required_argument, nullptr, 'n'},
    {"epsilon", required_argument, nullptr, 'e'},
    {"help", no_argument, nullptr, 'h'},
    {nullptr, 0, nullptr, 0},
  };

  Options opts;
  opterr = 0;
  int index = 0;
  int ret = 0;

  while ((ret = getopt_long(argc, argv, shortOpts, longOpts.data(), &index)) != -1) {
    switch (ret) {
    case 'k':
      opts.kernel_dir = optarg;
      break;
    case 1000:
      opts.host_results_path = optarg;
      break;
    case 1001:
      opts.device_results_path = optarg;
      break;
    case 't':
      opts.kernel_launch_timeout = std::atoi(optarg);
      break;
    case 'd':
      opts.device_type = optarg;
      break;
    case 'n':
      opts.num_elements = static_cast<size_t>(std::stoull(optarg));
      break;
    case 'e':
      opts.epsilon = std::atof(optarg);
      break;
    case 'h':
      std::cout << helpMsg << GenericLauncher::help_msg << std::endl;
      std::exit(0);
    case '?':
      nextlevel.emplace_back(argv[optind - 1]);
      break;
    default:
      std::cerr << "Unknown option: " << argv[optind - 1] << '\n';
      std::exit(1);
    }
  }

  return opts;
}

fs::path defaultKernelDir() {
  if (const char* envRoot = std::getenv("ET_BLAS_OPTIMIZED_KERNEL_ROOT")) {
    return envRoot;
  }
  return "/opt/et/kernels/blas/optimized/fp32/level1/dot";
}

fs::path resolveKernelArtifact(const fs::path& kernelPathOrDir) {
  if (kernelPathOrDir.extension() == ".elf" || kernelPathOrDir.extension() == ".elf_dbg") {
    return kernelPathOrDir;
  }

  const auto dbgPath = kernelPathOrDir / "blas_dot_optimized_fp32.elf_dbg";
  if (std::filesystem::exists(dbgPath)) {
    return dbgPath;
  }

  const auto elfPath = kernelPathOrDir / "blas_dot_optimized_fp32.elf";
  if (std::filesystem::exists(elfPath)) {
    return elfPath;
  }

  return elfPath;
}

std::vector<float> makeInputX(size_t size) {
  std::vector<float> values(size);
  for (size_t i = 0; i < size; ++i) {
    values[i] = 1.0f + 0.25f * static_cast<float>(i);
  }
  return values;
}

std::vector<float> makeInputY(size_t size) {
  std::vector<float> values(size);
  for (size_t i = 0; i < size; ++i) {
    values[i] = 100.0f - 0.5f * static_cast<float>(i);
  }
  return values;
}

float hostDot(const std::vector<float>& x, const std::vector<float>& y) {
  float result = 0.0f;
  for (size_t i = 0; i < x.size(); ++i) {
    result += x[i] * y[i];
  }
  return result;
}

bool nearlyEqual(float lhs, float rhs, double epsilon) {
  const double diff = std::fabs(static_cast<double>(lhs) - static_cast<double>(rhs));
  const double scale = std::max({1.0, std::fabs(static_cast<double>(lhs)), std::fabs(static_cast<double>(rhs))});
  return diff <= epsilon * scale;
}

void writeVector(std::ofstream& out, const std::string& label, const std::vector<float>& values) {
  out << "### " << label << "\n\n";
  out << "| index | value |\n";
  out << "| ---: | ---: |\n";
  out << std::fixed << std::setprecision(6);
  for (size_t i = 0; i < values.size(); ++i) {
    out << "| " << i << " | " << values[i] << " |\n";
  }
  out << "\n";
}

void writeResults(const fs::path& path, const std::string& side, const Options& opt, const Result& result) {
  if (path.has_parent_path()) {
    std::filesystem::create_directories(path.parent_path());
  }

  std::ofstream out(path);
  if (!out.is_open()) {
    throw std::runtime_error("Unable to open results file: " + path.string());
  }

  out << "# Optimized FP32 SDOT " << side << " Results\n\n";
  out << "- device_type: `" << opt.device_type << "`\n";
  out << "- num_elements: `" << opt.num_elements << "`\n";
  out << "- epsilon: `" << opt.epsilon << "`\n";
  out << "- kernel_dir: `" << opt.kernel_dir.string() << "`\n";
  out << "- kernel_artifact: `" << result.kernel_artifact.string() << "`\n";
  out << "- status: `" << (result.ok ? "ok" : "mismatch") << "`\n\n";

  writeVector(out, "input_x", result.input_x);
  writeVector(out, "input_y", result.input_y);

  out << "### result\n\n";
  out << "| value |\n";
  out << "| ---: |\n";
  out << std::fixed << std::setprecision(6);
  out << "| " << result.output << " |\n\n";
}

class DotVerifierLauncher : public GenericLauncher {
public:
  using GenericLauncher::GenericLauncher;

  std::byte* allocateBytes(size_t bytes, uint32_t deviceIdx = 0) {
    return runtime_->mallocDevice(devices_.at(deviceIdx), bytes);
  }

  void freeBytes(std::byte* ptr, uint32_t deviceIdx = 0) {
    runtime_->freeDevice(devices_.at(deviceIdx), ptr);
  }

  void copyHostToDevice(const void* src, std::byte* dst, size_t bytes, uint32_t deviceIdx = 0) {
    runtime_->memcpyHostToDevice(
      defaultStreams_.at(deviceIdx), reinterpret_cast<const std::byte*>(src), dst, bytes);
  }

  void copyDeviceToHost(const std::byte* src, void* dst, size_t bytes, uint32_t deviceIdx = 0) {
    runtime_->memcpyDeviceToHost(
      defaultStreams_.at(deviceIdx), src, reinterpret_cast<std::byte*>(dst), bytes);
  }
};

Result runDeviceDot(DotVerifierLauncher& launcher, const Options& opt, const fs::path& kernelPath) {
  Result result;
  result.kernel_artifact = kernelPath;
  result.input_x = makeInputX(opt.num_elements);
  result.input_y = makeInputY(opt.num_elements);
  std::vector<float> partials(opt.num_elements == 0 ? 1 : opt.num_elements, 0.0f);

  auto kernelId = launcher.loadKernel(kernelPath.string());
  auto deviceX = launcher.allocateBytes(result.input_x.size() * sizeof(float));
  auto deviceY = launcher.allocateBytes(result.input_y.size() * sizeof(float));
  auto devicePartials = launcher.allocateBytes(partials.size() * sizeof(float));
  auto deviceResultBuffer = launcher.allocateBytes(sizeof(float));

  launcher.copyHostToDevice(result.input_x.data(), deviceX, result.input_x.size() * sizeof(float));
  launcher.copyHostToDevice(result.input_y.data(), deviceY, result.input_y.size() * sizeof(float));
  launcher.copyHostToDevice(partials.data(), devicePartials, partials.size() * sizeof(float));

  DotArgs args{static_cast<uint64_t>(result.input_x.size()), reinterpret_cast<float*>(deviceX),
               reinterpret_cast<float*>(deviceY), reinterpret_cast<float*>(devicePartials),
               reinterpret_cast<float*>(deviceResultBuffer)};

  launcher.kernelLaunch(kernelId, &args);
  launcher.waitKernelCompletion(std::chrono::seconds(opt.kernel_launch_timeout));
  launcher.copyDeviceToHost(deviceResultBuffer, &result.output, sizeof(float));
  launcher.waitKernelCompletion(std::chrono::seconds(opt.kernel_launch_timeout));
  result.ok = !launcher.checkKernelExecutionErrors();

  launcher.freeBytes(deviceX);
  launcher.freeBytes(deviceY);
  launcher.freeBytes(devicePartials);
  launcher.freeBytes(deviceResultBuffer);
  launcher.unLoadKernel(kernelId);
  return result;
}

} // namespace

int main(int argc, char** argv) {
  std::vector<char*> argvPendingToParse{argv[0]};
  Options opt = parseArgs(argc, argv, argvPendingToParse);

  if (opt.kernel_dir.empty()) {
    opt.kernel_dir = defaultKernelDir();
  }

  Config config{modeFromString(opt.device_type), 1};
  config.dump();

  DotVerifierLauncher launcher(config, static_cast<int>(argvPendingToParse.size()), argvPendingToParse.data());
  launcher.initialize();

  const Result deviceResult = runDeviceDot(launcher, opt, resolveKernelArtifact(opt.kernel_dir));

  launcher.tearDown();

  Result hostResult = deviceResult;
  hostResult.output = hostDot(hostResult.input_x, hostResult.input_y);

  const bool ok = deviceResult.ok && nearlyEqual(hostResult.output, deviceResult.output, opt.epsilon);
  hostResult.ok = ok;

  Result finalDeviceResult = deviceResult;
  finalDeviceResult.ok = ok;

  writeResults(opt.host_results_path, "Host", opt, hostResult);
  writeResults(opt.device_results_path, "Device", opt, finalDeviceResult);

  if (!ok) {
    std::cerr << "Optimized FP32 SDOT verification failed" << std::endl;
    return 1;
  }

  std::cout << "Optimized FP32 SDOT verification passed" << std::endl;
  return 0;
}
