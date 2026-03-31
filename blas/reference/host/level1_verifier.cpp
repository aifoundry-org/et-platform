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
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct AxpyArgs {
  uint64_t numElements;
  const float* x;
  float* y;
  float alpha;
} __attribute__((packed));

struct CopyArgs {
  uint64_t numElements;
  const float* x;
  float* y;
} __attribute__((packed));

struct ScalArgs {
  uint64_t numElements;
  float* x;
  float alpha;
} __attribute__((packed));

struct SwapArgs {
  uint64_t numElements;
  float* x;
  float* y;
} __attribute__((packed));

struct DotArgs {
  uint64_t numElements;
  const float* x;
  const float* y;
  float* partials;
  float* res;
} __attribute__((packed));

struct Norm2Args {
  uint64_t numElements;
  const float* x;
  float* partials;
  float* res;
} __attribute__((packed));

struct AsumArgs {
  uint64_t numElements;
  const float* x;
  float* partials;
  float* res;
} __attribute__((packed));

struct Options {
  fs::path kernel_root = "";
  fs::path host_results_path = "blas/reference/host_results.md";
  fs::path device_results_path = "blas/reference/device_results.md";
  int kernel_launch_timeout = 30;
  std::string device_type = "sysemu";
  size_t num_elements = 256;
  double epsilon = 1.0e-5;
};

struct OperationResult {
  std::string name;
  std::string output_label_x = "output_x";
  std::string output_label_y = "output_y";
  bool has_alpha = false;
  float alpha = 0.0f;
  bool has_input_y = true;
  bool has_output_x = false;
  bool has_output_y = true;
  bool ok = false;
  std::vector<float> input_x;
  std::vector<float> input_y;
  std::vector<float> output_x;
  std::vector<float> output_y;
};

Options parse_args(int argc, char* const* argv, std::vector<char*>& nextlevel) {
  static constexpr const char* help_msg =
    "Usage: [options]\n\n"
    "Quick verifier for BLAS reference Level 1 kernels.\n\n"
    "Optional switches:\n"
    "  -k, --kernel_root            root containing BLAS Level 1 kernel directories\n"
    "      --host_results_path      markdown file for host reference outputs\n"
    "      --device_results_path    markdown file for device outputs\n"
    "  -t, --kernel_launch_timeout  timeout (in seconds) to wait for kernel completion\n"
    "  -d, --device_type            device type to use (sysemu, fake, silicon)\n"
    "  -n, --num_elements           number of elements in dummy vectors\n"
    "  -e, --epsilon                comparison tolerance for float results\n";

  static constexpr const char* short_opts = "k:t:d:n:e:h";
  static const std::vector<option> long_opts{
    {"kernel_root", required_argument, nullptr, 'k'},
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

  while ((ret = getopt_long(argc, argv, short_opts, long_opts.data(), &index)) != -1) {
    switch (ret) {
    case 'k':
      opts.kernel_root = optarg;
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
      std::cout << help_msg << GenericLauncher::help_msg << std::endl;
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

fs::path defaultKernelRoot() {
  if (const char* envRoot = std::getenv("ET_BLAS_KERNEL_ROOT")) {
    return envRoot;
  }
  return "/opt/et/kernels/blas/reference/fp32/level1";
}

void host_axpy(const std::vector<float>& x, std::vector<float>& y, float alpha) {
  for (size_t i = 0; i < x.size(); ++i) {
    y[i] = alpha * x[i] + y[i];
  }
}

void host_copy(const std::vector<float>& x, std::vector<float>& y) {
  std::copy(x.begin(), x.end(), y.begin());
}

void host_scal(std::vector<float>& x, float alpha) {
  for (auto& value : x) {
    value *= alpha;
  }
}

void host_swap(std::vector<float>& x, std::vector<float>& y) {
  for (size_t i = 0; i < x.size(); ++i) {
    std::swap(x[i], y[i]);
  }
}

float host_dot(const std::vector<float>& x, const std::vector<float>& y) {
  float result = 0.0f;
  for (size_t i = 0; i < x.size(); ++i) {
    result += x[i] * y[i];
  }
  return result;
}

float host_norm2(const std::vector<float>& x) {
  float result = 0.0f;
  for (float value : x) {
    result += value * value;
  }
  return std::sqrt(result);
}

float host_asum(const std::vector<float>& x) {
  float result = 0.0f;
  for (float value : x) {
    result += std::fabs(value);
  }
  return result;
}

bool nearlyEqual(const std::vector<float>& lhs, const std::vector<float>& rhs, double epsilon) {
  return std::equal(lhs.begin(), lhs.end(), rhs.begin(), [epsilon](float a, float b) {
    return std::fabs(static_cast<double>(a) - static_cast<double>(b)) <= epsilon;
  });
}

bool nearlyEqual(float lhs, float rhs, double epsilon) {
  return std::fabs(static_cast<double>(lhs) - static_cast<double>(rhs)) <= epsilon;
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

void writeResultsMarkdown(const fs::path& path, const std::string& side, const Options& opt,
                         const std::vector<OperationResult>& results) {
  if (path.has_parent_path()) {
    std::filesystem::create_directories(path.parent_path());
  }

  std::ofstream out(path);
  if (!out.is_open()) {
    throw std::runtime_error("Unable to open results file: " + path.string());
  }

  out << "# BLAS Level 1 " << side << " Results\n\n";
  out << "- device_type: `" << opt.device_type << "`\n";
  out << "- num_elements: `" << opt.num_elements << "`\n";
  out << "- epsilon: `" << opt.epsilon << "`\n";
  out << "- kernel_root: `" << opt.kernel_root.string() << "`\n\n";

  for (const auto& result : results) {
    out << "## " << result.name << "\n\n";
    out << "- status: `" << (result.ok ? "ok" : "mismatch") << "`\n";
    if (result.has_alpha) {
      out << "- alpha: `" << std::fixed << std::setprecision(6) << result.alpha << "`\n";
    }
    out << "\n";
    writeVector(out, "input_x", result.input_x);
    if (result.has_input_y) {
      writeVector(out, "input_y", result.input_y);
    }
    if (result.has_output_x) {
      writeVector(out, result.output_label_x, result.output_x);
    }
    if (result.has_output_y) {
      writeVector(out, result.output_label_y, result.output_y);
    }
  }
}

class ReferenceVerifierLauncher : public GenericLauncher {
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

OperationResult verifyAxpy(ReferenceVerifierLauncher& launcher, const Options& opt, const fs::path& kernelPath) {
  const float alpha = 3.0f;
  auto x = makeInputX(opt.num_elements);
  auto inputY = makeInputY(opt.num_elements);
  auto deviceY = inputY;
  auto expectedY = inputY;
  host_axpy(x, expectedY, alpha);

  auto kernelId = launcher.loadKernel(kernelPath.string());
  auto deviceX = launcher.allocateBytes(x.size() * sizeof(float));
  auto deviceYBuffer = launcher.allocateBytes(deviceY.size() * sizeof(float));

  launcher.copyHostToDevice(x.data(), deviceX, x.size() * sizeof(float));
  launcher.copyHostToDevice(deviceY.data(), deviceYBuffer, deviceY.size() * sizeof(float));

  AxpyArgs args{static_cast<uint64_t>(x.size()), reinterpret_cast<float*>(deviceX),
                reinterpret_cast<float*>(deviceYBuffer), alpha};

  launcher.kernelLaunch(kernelId, &args);
  launcher.copyDeviceToHost(deviceYBuffer, deviceY.data(), deviceY.size() * sizeof(float));
  launcher.waitKernelCompletion(std::chrono::seconds(opt.kernel_launch_timeout));

  OperationResult result;
  result.name = "axpy";
  result.has_alpha = true;
  result.alpha = alpha;
  result.input_x = x;
  result.input_y = inputY;
  result.output_y = deviceY;
  result.output_label_y = "output_y";
  result.ok = !launcher.checkKernelExecutionErrors() && nearlyEqual(expectedY, deviceY, opt.epsilon);

  launcher.freeBytes(deviceX);
  launcher.freeBytes(deviceYBuffer);
  launcher.unLoadKernel(kernelId);
  return result;
}

OperationResult verifyCopy(ReferenceVerifierLauncher& launcher, const Options& opt, const fs::path& kernelPath) {
  auto x = makeInputX(opt.num_elements);
  auto inputY = makeInputY(opt.num_elements);
  auto deviceY = inputY;
  auto expectedY = inputY;
  host_copy(x, expectedY);

  auto kernelId = launcher.loadKernel(kernelPath.string());
  auto deviceX = launcher.allocateBytes(x.size() * sizeof(float));
  auto deviceYBuffer = launcher.allocateBytes(deviceY.size() * sizeof(float));

  launcher.copyHostToDevice(x.data(), deviceX, x.size() * sizeof(float));
  launcher.copyHostToDevice(deviceY.data(), deviceYBuffer, deviceY.size() * sizeof(float));

  CopyArgs args{static_cast<uint64_t>(x.size()), reinterpret_cast<float*>(deviceX),
                reinterpret_cast<float*>(deviceYBuffer)};

  launcher.kernelLaunch(kernelId, &args);
  launcher.copyDeviceToHost(deviceYBuffer, deviceY.data(), deviceY.size() * sizeof(float));
  launcher.waitKernelCompletion(std::chrono::seconds(opt.kernel_launch_timeout));

  OperationResult result;
  result.name = "copy";
  result.input_x = x;
  result.input_y = inputY;
  result.output_y = deviceY;
  result.output_label_y = "output_y";
  result.ok = !launcher.checkKernelExecutionErrors() && nearlyEqual(expectedY, deviceY, opt.epsilon);

  launcher.freeBytes(deviceX);
  launcher.freeBytes(deviceYBuffer);
  launcher.unLoadKernel(kernelId);
  return result;
}

OperationResult verifyScal(ReferenceVerifierLauncher& launcher, const Options& opt, const fs::path& kernelPath) {
  const float alpha = -2.0f;
  auto inputX = makeInputX(opt.num_elements);
  auto deviceX = inputX;
  auto expectedX = inputX;
  host_scal(expectedX, alpha);

  auto kernelId = launcher.loadKernel(kernelPath.string());
  auto deviceXBuffer = launcher.allocateBytes(deviceX.size() * sizeof(float));

  launcher.copyHostToDevice(deviceX.data(), deviceXBuffer, deviceX.size() * sizeof(float));

  ScalArgs args{static_cast<uint64_t>(deviceX.size()), reinterpret_cast<float*>(deviceXBuffer), alpha};

  launcher.kernelLaunch(kernelId, &args);
  launcher.copyDeviceToHost(deviceXBuffer, deviceX.data(), deviceX.size() * sizeof(float));
  launcher.waitKernelCompletion(std::chrono::seconds(opt.kernel_launch_timeout));

  OperationResult result;
  result.name = "scal";
  result.has_alpha = true;
  result.alpha = alpha;
  result.has_input_y = false;
  result.has_output_x = true;
  result.has_output_y = false;
  result.output_label_x = "output_x";
  result.input_x = inputX;
  result.output_x = deviceX;
  result.ok = !launcher.checkKernelExecutionErrors() && nearlyEqual(expectedX, deviceX, opt.epsilon);

  launcher.freeBytes(deviceXBuffer);
  launcher.unLoadKernel(kernelId);
  return result;
}

OperationResult verifySwap(ReferenceVerifierLauncher& launcher, const Options& opt, const fs::path& kernelPath) {
  auto inputX = makeInputX(opt.num_elements);
  auto inputY = makeInputY(opt.num_elements);
  auto deviceX = inputX;
  auto deviceY = inputY;
  auto expectedX = inputX;
  auto expectedY = inputY;
  host_swap(expectedX, expectedY);

  auto kernelId = launcher.loadKernel(kernelPath.string());
  auto deviceXBuffer = launcher.allocateBytes(deviceX.size() * sizeof(float));
  auto deviceYBuffer = launcher.allocateBytes(deviceY.size() * sizeof(float));

  launcher.copyHostToDevice(deviceX.data(), deviceXBuffer, deviceX.size() * sizeof(float));
  launcher.copyHostToDevice(deviceY.data(), deviceYBuffer, deviceY.size() * sizeof(float));

  SwapArgs args{static_cast<uint64_t>(deviceX.size()), reinterpret_cast<float*>(deviceXBuffer),
                reinterpret_cast<float*>(deviceYBuffer)};

  launcher.kernelLaunch(kernelId, &args);
  launcher.copyDeviceToHost(deviceXBuffer, deviceX.data(), deviceX.size() * sizeof(float));
  launcher.copyDeviceToHost(deviceYBuffer, deviceY.data(), deviceY.size() * sizeof(float));
  launcher.waitKernelCompletion(std::chrono::seconds(opt.kernel_launch_timeout));

  OperationResult result;
  result.name = "swap";
  result.has_output_x = true;
  result.has_output_y = true;
  result.output_label_x = "output_x";
  result.output_label_y = "output_y";
  result.input_x = inputX;
  result.input_y = inputY;
  result.output_x = deviceX;
  result.output_y = deviceY;
  result.ok = !launcher.checkKernelExecutionErrors() && nearlyEqual(expectedX, deviceX, opt.epsilon) &&
              nearlyEqual(expectedY, deviceY, opt.epsilon);

  launcher.freeBytes(deviceXBuffer);
  launcher.freeBytes(deviceYBuffer);
  launcher.unLoadKernel(kernelId);
  return result;
}

OperationResult verifyDot(ReferenceVerifierLauncher& launcher, const Options& opt, const fs::path& kernelPath) {
  auto x = makeInputX(opt.num_elements);
  auto y = makeInputY(opt.num_elements);
  const float expectedResult = host_dot(x, y);
  std::vector<float> partials(opt.num_elements == 0 ? 1 : opt.num_elements, 0.0f);
  float deviceResult = 0.0f;

  auto kernelId = launcher.loadKernel(kernelPath.string());
  auto deviceX = launcher.allocateBytes(x.size() * sizeof(float));
  auto deviceY = launcher.allocateBytes(y.size() * sizeof(float));
  auto devicePartials = launcher.allocateBytes(partials.size() * sizeof(float));
  auto deviceResultBuffer = launcher.allocateBytes(sizeof(float));

  launcher.copyHostToDevice(x.data(), deviceX, x.size() * sizeof(float));
  launcher.copyHostToDevice(y.data(), deviceY, y.size() * sizeof(float));
  launcher.copyHostToDevice(partials.data(), devicePartials, partials.size() * sizeof(float));

  DotArgs args{static_cast<uint64_t>(x.size()), reinterpret_cast<float*>(deviceX), reinterpret_cast<float*>(deviceY),
               reinterpret_cast<float*>(devicePartials), reinterpret_cast<float*>(deviceResultBuffer)};

  launcher.kernelLaunch(kernelId, &args);
  launcher.copyDeviceToHost(deviceResultBuffer, &deviceResult, sizeof(float));
  launcher.waitKernelCompletion(std::chrono::seconds(opt.kernel_launch_timeout));

  OperationResult result;
  result.name = "dot";
  result.input_x = x;
  result.input_y = y;
  result.output_label_y = "result";
  result.output_y = {deviceResult};
  result.ok = !launcher.checkKernelExecutionErrors() && nearlyEqual(expectedResult, deviceResult, opt.epsilon);

  launcher.freeBytes(deviceX);
  launcher.freeBytes(deviceY);
  launcher.freeBytes(devicePartials);
  launcher.freeBytes(deviceResultBuffer);
  launcher.unLoadKernel(kernelId);
  return result;
}

OperationResult verifyNorm2(ReferenceVerifierLauncher& launcher, const Options& opt, const fs::path& kernelPath) {
  auto x = makeInputX(opt.num_elements);
  const float expectedResult = host_norm2(x);
  std::vector<float> partials(opt.num_elements == 0 ? 1 : opt.num_elements, 0.0f);
  float deviceResult = 0.0f;

  auto kernelId = launcher.loadKernel(kernelPath.string());
  auto deviceX = launcher.allocateBytes(x.size() * sizeof(float));
  auto devicePartials = launcher.allocateBytes(partials.size() * sizeof(float));
  auto deviceResultBuffer = launcher.allocateBytes(sizeof(float));

  launcher.copyHostToDevice(x.data(), deviceX, x.size() * sizeof(float));
  launcher.copyHostToDevice(partials.data(), devicePartials, partials.size() * sizeof(float));

  Norm2Args args{static_cast<uint64_t>(x.size()), reinterpret_cast<float*>(deviceX),
                 reinterpret_cast<float*>(devicePartials), reinterpret_cast<float*>(deviceResultBuffer)};

  launcher.kernelLaunch(kernelId, &args);
  launcher.copyDeviceToHost(deviceResultBuffer, &deviceResult, sizeof(float));
  launcher.waitKernelCompletion(std::chrono::seconds(opt.kernel_launch_timeout));

  OperationResult result;
  result.name = "norm2";
  result.has_input_y = false;
  result.input_x = x;
  result.output_label_y = "result";
  result.output_y = {deviceResult};
  result.ok = !launcher.checkKernelExecutionErrors() && nearlyEqual(expectedResult, deviceResult, opt.epsilon);

  launcher.freeBytes(deviceX);
  launcher.freeBytes(devicePartials);
  launcher.freeBytes(deviceResultBuffer);
  launcher.unLoadKernel(kernelId);
  return result;
}

OperationResult verifyAsum(ReferenceVerifierLauncher& launcher, const Options& opt, const fs::path& kernelPath) {
  auto x = makeInputX(opt.num_elements);
  const float expectedResult = host_asum(x);
  std::vector<float> partials(opt.num_elements == 0 ? 1 : opt.num_elements, 0.0f);
  float deviceResult = 0.0f;

  auto kernelId = launcher.loadKernel(kernelPath.string());
  auto deviceX = launcher.allocateBytes(x.size() * sizeof(float));
  auto devicePartials = launcher.allocateBytes(partials.size() * sizeof(float));
  auto deviceResultBuffer = launcher.allocateBytes(sizeof(float));

  launcher.copyHostToDevice(x.data(), deviceX, x.size() * sizeof(float));
  launcher.copyHostToDevice(partials.data(), devicePartials, partials.size() * sizeof(float));

  AsumArgs args{static_cast<uint64_t>(x.size()), reinterpret_cast<float*>(deviceX),
                reinterpret_cast<float*>(devicePartials), reinterpret_cast<float*>(deviceResultBuffer)};

  launcher.kernelLaunch(kernelId, &args);
  launcher.copyDeviceToHost(deviceResultBuffer, &deviceResult, sizeof(float));
  launcher.waitKernelCompletion(std::chrono::seconds(opt.kernel_launch_timeout));

  OperationResult result;
  result.name = "asum";
  result.has_input_y = false;
  result.input_x = x;
  result.output_label_y = "result";
  result.output_y = {deviceResult};
  result.ok = !launcher.checkKernelExecutionErrors() && nearlyEqual(expectedResult, deviceResult, opt.epsilon);

  launcher.freeBytes(deviceX);
  launcher.freeBytes(devicePartials);
  launcher.freeBytes(deviceResultBuffer);
  launcher.unLoadKernel(kernelId);
  return result;
}

} // namespace

int main(int argc, char** argv) {
  std::vector<char*> argvPendingToParse{argv[0]};
  Options opt = parse_args(argc, argv, argvPendingToParse);

  if (opt.kernel_root.empty()) {
    opt.kernel_root = defaultKernelRoot();
  }

  Config config{modeFromString(opt.device_type), 1};
  config.dump();

  ReferenceVerifierLauncher launcher(config, static_cast<int>(argvPendingToParse.size()), argvPendingToParse.data());
  launcher.initialize();

  const OperationResult deviceAxpy =
    verifyAxpy(launcher, opt, opt.kernel_root / "axpy" / "blas_axpy_reference_fp32.elf");
  const OperationResult deviceAsum =
    verifyAsum(launcher, opt, opt.kernel_root / "asum" / "blas_asum_reference_fp32.elf");
  const OperationResult deviceCopy =
    verifyCopy(launcher, opt, opt.kernel_root / "copy" / "blas_copy_reference_fp32.elf");
  const OperationResult deviceDot =
    verifyDot(launcher, opt, opt.kernel_root / "dot" / "blas_dot_reference_fp32.elf");
  const OperationResult deviceNorm2 =
    verifyNorm2(launcher, opt, opt.kernel_root / "norm2" / "blas_norm2_reference_fp32.elf");
  const OperationResult deviceScal =
    verifyScal(launcher, opt, opt.kernel_root / "scal" / "blas_scal_reference_fp32.elf");
  const OperationResult deviceSwap =
    verifySwap(launcher, opt, opt.kernel_root / "swap" / "blas_swap_reference_fp32.elf");

  launcher.tearDown();

  std::vector<OperationResult> hostResults;
  std::vector<OperationResult> deviceResults;

  hostResults.emplace_back(deviceAxpy);
  hostResults.back().output_y = makeInputY(opt.num_elements);
  host_axpy(hostResults.back().input_x, hostResults.back().output_y, hostResults.back().alpha);
  hostResults.back().ok = deviceAxpy.ok;

  hostResults.emplace_back(deviceAsum);
  hostResults.back().output_y = {host_asum(hostResults.back().input_x)};
  hostResults.back().ok = deviceAsum.ok;

  hostResults.emplace_back(deviceCopy);
  hostResults.back().output_y = makeInputY(opt.num_elements);
  host_copy(hostResults.back().input_x, hostResults.back().output_y);
  hostResults.back().ok = deviceCopy.ok;

  hostResults.emplace_back(deviceDot);
  hostResults.back().output_y = {host_dot(hostResults.back().input_x, hostResults.back().input_y)};
  hostResults.back().ok = deviceDot.ok;

  hostResults.emplace_back(deviceNorm2);
  hostResults.back().output_y = {host_norm2(hostResults.back().input_x)};
  hostResults.back().ok = deviceNorm2.ok;

  hostResults.emplace_back(deviceScal);
  hostResults.back().output_x = hostResults.back().input_x;
  host_scal(hostResults.back().output_x, hostResults.back().alpha);
  hostResults.back().ok = deviceScal.ok;

  hostResults.emplace_back(deviceSwap);
  hostResults.back().output_x = hostResults.back().input_x;
  hostResults.back().output_y = hostResults.back().input_y;
  host_swap(hostResults.back().output_x, hostResults.back().output_y);
  hostResults.back().ok = deviceSwap.ok;

  deviceResults.push_back(deviceAxpy);
  deviceResults.push_back(deviceAsum);
  deviceResults.push_back(deviceCopy);
  deviceResults.push_back(deviceDot);
  deviceResults.push_back(deviceNorm2);
  deviceResults.push_back(deviceScal);
  deviceResults.push_back(deviceSwap);

  writeResultsMarkdown(opt.host_results_path, "Host", opt, hostResults);
  writeResultsMarkdown(opt.device_results_path, "Device", opt, deviceResults);

  if (!deviceAxpy.ok || !deviceAsum.ok || !deviceCopy.ok || !deviceDot.ok || !deviceNorm2.ok ||
      !deviceScal.ok || !deviceSwap.ok) {
    std::cerr << "BLAS Level 1 reference verification failed" << std::endl;
    return 1;
  }

  std::cout << "BLAS Level 1 reference verification passed" << std::endl;
  return 0;
}
