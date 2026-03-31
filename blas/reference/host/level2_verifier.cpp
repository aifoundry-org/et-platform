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
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

struct GemvArgs {
  char trans;
  int32_t m;
  int32_t n;
  float alpha;
  const float* a;
  int32_t lda;
  const float* x;
  int32_t incx;
  float beta;
  float* y;
  int32_t incy;
};

struct GbmvArgs {
  char trans;
  int32_t m;
  int32_t n;
  int32_t kl;
  int32_t ku;
  float alpha;
  const float* a;
  int32_t lda;
  const float* x;
  int32_t incx;
  float beta;
  float* y;
  int32_t incy;
};

struct GerArgs {
  int32_t m;
  int32_t n;
  float alpha;
  const float* x;
  int32_t incx;
  const float* y;
  int32_t incy;
  float* a;
  int32_t lda;
};

struct Options {
  fs::path kernel_root = "";
  fs::path host_results_path = "blas/reference/host_level2_results.md";
  fs::path device_results_path = "blas/reference/device_level2_results.md";
  int kernel_launch_timeout = 30;
  std::string device_type = "sysemu";
  double epsilon = 1.0e-5;
  std::string selected_case = "all";
};

struct VectorRecord {
  std::string label;
  std::vector<float> values;
};

struct MatrixRecord {
  std::string label;
  int rows = 0;
  int cols = 0;
  std::vector<float> values;
};

struct CaseResult {
  std::string name;
  std::string params;
  bool ok = false;
  std::vector<VectorRecord> vectors;
  std::vector<MatrixRecord> matrices;
};

struct VerificationPair {
  CaseResult host;
  CaseResult device;
};

Options parse_args(int argc, char* const* argv, std::vector<char*>& nextlevel) {
  static constexpr const char* help_msg =
    "Usage: [options]\n\n"
    "Quick verifier for BLAS reference Level 2 kernels.\n\n"
    "Optional switches:\n"
    "  -k, --kernel_root            root containing gemv/gbmv/ger kernel directories\n"
    "      --host_results_path      markdown file for host reference outputs\n"
    "      --device_results_path    markdown file for device outputs\n"
    "      --case                   case to run (all, gemv_n, gemv_t, gbmv_n, gbmv_t, ger)\n"
    "  -t, --kernel_launch_timeout  timeout (in seconds) to wait for kernel completion\n"
    "  -d, --device_type            device type to use (sysemu, fake, silicon)\n"
    "  -e, --epsilon                comparison tolerance for float results\n";

  static constexpr const char* short_opts = "k:t:d:e:h";
  static const std::vector<option> long_opts{
    {"kernel_root", required_argument, nullptr, 'k'},
    {"host_results_path", required_argument, nullptr, 1000},
    {"device_results_path", required_argument, nullptr, 1001},
    {"case", required_argument, nullptr, 1002},
    {"kernel_launch_timeout", required_argument, nullptr, 't'},
    {"device_type", required_argument, nullptr, 'd'},
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
    case 1002:
      opts.selected_case = optarg;
      break;
    case 't':
      opts.kernel_launch_timeout = std::atoi(optarg);
      break;
    case 'd':
      opts.device_type = optarg;
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
  return "/opt/et/kernels/blas/reference/fp32/level2";
}

bool isTranspose(char trans) {
  return trans == 'T' || trans == 't' || trans == 'C' || trans == 'c';
}

int startIndex(int length, int inc) {
  return inc >= 0 ? 0 : (length - 1) * (-inc);
}

std::vector<float> makeLogicalVector(int length, float base, float step) {
  std::vector<float> values(length);
  for (int i = 0; i < length; ++i) {
    values[i] = base + step * static_cast<float>(i);
  }
  return values;
}

std::vector<float> makeStridedStorage(const std::vector<float>& logical, int inc, float fillValue = -777.0f) {
  const int storageSize = logical.empty() ? 1 : 1 + (static_cast<int>(logical.size()) - 1) * std::abs(inc);
  std::vector<float> storage(storageSize, fillValue);
  const int base = startIndex(static_cast<int>(logical.size()), inc);
  for (size_t i = 0; i < logical.size(); ++i) {
    storage[base + static_cast<int>(i) * inc] = logical[i];
  }
  return storage;
}

std::vector<float> logicalFromStorage(const std::vector<float>& storage, int length, int inc) {
  std::vector<float> logical(length);
  const int base = startIndex(length, inc);
  for (int i = 0; i < length; ++i) {
    logical[i] = storage[base + i * inc];
  }
  return logical;
}

std::vector<float> makeDenseStorage(int rows, int cols, int lda) {
  std::vector<float> storage(static_cast<size_t>(lda) * cols, -999.0f);
  for (int col = 0; col < cols; ++col) {
    for (int row = 0; row < rows; ++row) {
      storage[row + static_cast<size_t>(col) * lda] =
        0.5f + static_cast<float>(row) * 0.75f + static_cast<float>(col) * 0.25f;
    }
  }
  return storage;
}

std::vector<float> denseFromColumnMajor(const std::vector<float>& storage, int rows, int cols, int lda) {
  std::vector<float> dense(static_cast<size_t>(rows) * cols);
  for (int row = 0; row < rows; ++row) {
    for (int col = 0; col < cols; ++col) {
      dense[row * cols + col] = storage[row + static_cast<size_t>(col) * lda];
    }
  }
  return dense;
}

std::vector<float> makeBandStorage(int m, int n, int kl, int ku, int lda) {
  std::vector<float> storage(static_cast<size_t>(lda) * n, -999.0f);
  for (int col = 0; col < n; ++col) {
    const int rowBegin = std::max(0, col - ku);
    const int rowEnd = std::min(m, col + kl + 1);
    for (int row = rowBegin; row < rowEnd; ++row) {
      const int bandRow = ku + row - col;
      storage[bandRow + static_cast<size_t>(col) * lda] =
        1.0f + static_cast<float>(row) * 0.5f + static_cast<float>(col) * 0.125f;
    }
  }
  return storage;
}

std::vector<float> denseFromBand(const std::vector<float>& storage, int m, int n, int kl, int ku, int lda) {
  std::vector<float> dense(static_cast<size_t>(m) * n, 0.0f);
  for (int col = 0; col < n; ++col) {
    const int rowBegin = std::max(0, col - ku);
    const int rowEnd = std::min(m, col + kl + 1);
    for (int row = rowBegin; row < rowEnd; ++row) {
      const int bandRow = ku + row - col;
      dense[row * n + col] = storage[bandRow + static_cast<size_t>(col) * lda];
    }
  }
  return dense;
}

bool nearlyEqual(const std::vector<float>& lhs, const std::vector<float>& rhs, double epsilon) {
  return std::equal(lhs.begin(), lhs.end(), rhs.begin(), [epsilon](float a, float b) {
    return std::fabs(static_cast<double>(a) - static_cast<double>(b)) <= epsilon;
  });
}

void hostGemv(char trans, int m, int n, float alpha, const std::vector<float>& a, int lda,
              const std::vector<float>& x, int incx, float beta, std::vector<float>& y, int incy) {
  const bool transpose = isTranspose(trans);
  const int outputCount = transpose ? n : m;
  const int reductionCount = transpose ? m : n;
  const int xBase = startIndex(reductionCount, incx);
  const int yBase = startIndex(outputCount, incy);

  for (int outputIndex = 0; outputIndex < outputCount; ++outputIndex) {
    float sum = 0.0f;
    if (!transpose) {
      for (int col = 0; col < n; ++col) {
        sum += a[outputIndex + static_cast<size_t>(col) * lda] * x[xBase + col * incx];
      }
    } else {
      for (int row = 0; row < m; ++row) {
        sum += a[row + static_cast<size_t>(outputIndex) * lda] * x[xBase + row * incx];
      }
    }
    float& yValue = y[yBase + outputIndex * incy];
    yValue = alpha * sum + beta * yValue;
  }
}

void hostGbmv(char trans, int m, int n, int kl, int ku, float alpha, const std::vector<float>& a, int lda,
              const std::vector<float>& x, int incx, float beta, std::vector<float>& y, int incy) {
  const bool transpose = isTranspose(trans);
  const int outputCount = transpose ? n : m;
  const int reductionCount = transpose ? m : n;
  const int xBase = startIndex(reductionCount, incx);
  const int yBase = startIndex(outputCount, incy);

  for (int outputIndex = 0; outputIndex < outputCount; ++outputIndex) {
    float sum = 0.0f;
    if (!transpose) {
      const int row = outputIndex;
      const int colBegin = std::max(0, row - kl);
      const int colEnd = std::min(n, row + ku + 1);
      for (int col = colBegin; col < colEnd; ++col) {
        const int bandRow = ku + row - col;
        sum += a[bandRow + static_cast<size_t>(col) * lda] * x[xBase + col * incx];
      }
    } else {
      const int col = outputIndex;
      const int rowBegin = std::max(0, col - ku);
      const int rowEnd = std::min(m, col + kl + 1);
      for (int row = rowBegin; row < rowEnd; ++row) {
        const int bandRow = ku + row - col;
        sum += a[bandRow + static_cast<size_t>(col) * lda] * x[xBase + row * incx];
      }
    }
    float& yValue = y[yBase + outputIndex * incy];
    yValue = alpha * sum + beta * yValue;
  }
}

void hostGer(int m, int n, float alpha, const std::vector<float>& x, int incx, const std::vector<float>& y, int incy,
             std::vector<float>& a, int lda) {
  const int xBase = startIndex(m, incx);
  const int yBase = startIndex(n, incy);
  for (int col = 0; col < n; ++col) {
    const float yValue = y[yBase + col * incy];
    for (int row = 0; row < m; ++row) {
      a[row + static_cast<size_t>(col) * lda] += alpha * x[xBase + row * incx] * yValue;
    }
  }
}

void writeVector(std::ofstream& out, const VectorRecord& record) {
  out << "### " << record.label << "\n\n";
  out << "| index | value |\n";
  out << "| ---: | ---: |\n";
  out << std::fixed << std::setprecision(6);
  for (size_t i = 0; i < record.values.size(); ++i) {
    out << "| " << i << " | " << record.values[i] << " |\n";
  }
  out << "\n";
}

void writeMatrix(std::ofstream& out, const MatrixRecord& record) {
  out << "### " << record.label << "\n\n";
  out << "| row/col |";
  for (int col = 0; col < record.cols; ++col) {
    out << " " << col << " |";
  }
  out << "\n| ---: |";
  for (int col = 0; col < record.cols; ++col) {
    out << " ---: |";
  }
  out << "\n";
  out << std::fixed << std::setprecision(6);
  for (int row = 0; row < record.rows; ++row) {
    out << "| " << row << " |";
    for (int col = 0; col < record.cols; ++col) {
      out << " " << record.values[row * record.cols + col] << " |";
    }
    out << "\n";
  }
  out << "\n";
}

void writeResultsMarkdown(const fs::path& path, const std::string& side, const Options& opt,
                         const std::vector<CaseResult>& results) {
  if (path.has_parent_path()) {
    std::filesystem::create_directories(path.parent_path());
  }

  std::ofstream out(path);
  if (!out.is_open()) {
    throw std::runtime_error("Unable to open results file: " + path.string());
  }

  out << "# BLAS Level 2 " << side << " Results\n\n";
  out << "- device_type: `" << opt.device_type << "`\n";
  out << "- epsilon: `" << opt.epsilon << "`\n";
  out << "- case: `" << opt.selected_case << "`\n";
  out << "- kernel_root: `" << opt.kernel_root.string() << "`\n\n";

  for (const auto& result : results) {
    out << "## " << result.name << "\n\n";
    out << "- status: `" << (result.ok ? "ok" : "mismatch") << "`\n";
    out << "- params: `" << result.params << "`\n\n";
    for (const auto& vector : result.vectors) {
      writeVector(out, vector);
    }
    for (const auto& matrix : result.matrices) {
      writeMatrix(out, matrix);
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

VerificationPair verifyGemvCase(ReferenceVerifierLauncher& launcher, const Options& opt, char trans) {
  const int m = 4;
  const int n = 5;
  const int lda = 6;
  const int incx = trans == 'N' ? 1 : 2;
  const int incy = 1;
  const float alpha = 1.5f;
  const float beta = -0.5f;

  const auto inputALogical = denseFromColumnMajor(makeDenseStorage(m, n, lda), m, n, lda);
  const auto inputAStorage = makeDenseStorage(m, n, lda);
  const auto inputXLogical = makeLogicalVector(trans == 'N' ? n : m, 1.0f, 0.5f);
  const auto inputYLogical = makeLogicalVector(trans == 'N' ? m : n, -2.0f, 0.25f);
  auto hostAStorage = inputAStorage;
  auto hostXStorage = makeStridedStorage(inputXLogical, incx);
  auto hostYStorage = makeStridedStorage(inputYLogical, incy);
  auto deviceYStorage = hostYStorage;

  hostGemv(trans, m, n, alpha, hostAStorage, lda, hostXStorage, incx, beta, hostYStorage, incy);

  auto kernelId = launcher.loadKernel((opt.kernel_root / "gemv" / "blas_gemv_reference_fp32.elf").string());
  auto deviceA = launcher.allocateBytes(inputAStorage.size() * sizeof(float));
  auto deviceX = launcher.allocateBytes(hostXStorage.size() * sizeof(float));
  auto deviceY = launcher.allocateBytes(deviceYStorage.size() * sizeof(float));

  launcher.copyHostToDevice(inputAStorage.data(), deviceA, inputAStorage.size() * sizeof(float));
  launcher.copyHostToDevice(hostXStorage.data(), deviceX, hostXStorage.size() * sizeof(float));
  launcher.copyHostToDevice(deviceYStorage.data(), deviceY, deviceYStorage.size() * sizeof(float));

  GemvArgs args{trans, m, n, alpha, reinterpret_cast<float*>(deviceA), lda, reinterpret_cast<float*>(deviceX), incx,
                beta, reinterpret_cast<float*>(deviceY), incy};
  launcher.kernelLaunch(kernelId, &args);
  launcher.copyDeviceToHost(deviceY, deviceYStorage.data(), deviceYStorage.size() * sizeof(float));
  launcher.waitKernelCompletion(std::chrono::seconds(opt.kernel_launch_timeout));

  const std::string caseName = trans == 'N' ? "gemv_n" : "gemv_t";
  std::ostringstream params;
  params << "trans=" << trans << ", m=" << m << ", n=" << n << ", lda=" << lda << ", incx=" << incx
         << ", incy=" << incy << ", alpha=" << alpha << ", beta=" << beta;
  const std::string paramString = params.str();
  const std::vector<float> hostOutputY =
    logicalFromStorage(hostYStorage, static_cast<int>(inputYLogical.size()), incy);
  const std::vector<float> deviceOutputY =
    logicalFromStorage(deviceYStorage, static_cast<int>(inputYLogical.size()), incy);
  const bool ok = !launcher.checkKernelExecutionErrors() && nearlyEqual(hostOutputY, deviceOutputY, opt.epsilon);

  launcher.freeBytes(deviceA);
  launcher.freeBytes(deviceX);
  launcher.freeBytes(deviceY);
  launcher.unLoadKernel(kernelId);

  VerificationPair result;
  result.host.name = caseName;
  result.host.params = paramString;
  result.host.ok = ok;
  result.host.vectors.push_back({"input_x", inputXLogical});
  result.host.vectors.push_back({"input_y", inputYLogical});
  result.host.vectors.push_back({"output_y", hostOutputY});
  result.host.matrices.push_back({"input_a", m, n, inputALogical});

  result.device.name = caseName;
  result.device.params = paramString;
  result.device.ok = ok;
  result.device.vectors.push_back({"input_x", inputXLogical});
  result.device.vectors.push_back({"input_y", inputYLogical});
  result.device.vectors.push_back({"output_y", deviceOutputY});
  result.device.matrices.push_back({"input_a", m, n, inputALogical});
  return result;
}

VerificationPair verifyGbmvCase(ReferenceVerifierLauncher& launcher, const Options& opt, char trans) {
  const int m = 5;
  const int n = 6;
  const int kl = 1;
  const int ku = 2;
  const int lda = 5;
  const int incx = trans == 'N' ? 1 : 2;
  const int incy = trans == 'N' ? 2 : 1;
  const float alpha = 0.75f;
  const float beta = 1.25f;

  const auto inputAStorage = makeBandStorage(m, n, kl, ku, lda);
  const auto inputALogical = denseFromBand(inputAStorage, m, n, kl, ku, lda);
  const auto inputXLogical = makeLogicalVector(trans == 'N' ? n : m, 0.5f, 0.375f);
  const auto inputYLogical = makeLogicalVector(trans == 'N' ? m : n, 1.0f, -0.25f);
  auto hostXStorage = makeStridedStorage(inputXLogical, incx);
  auto hostYStorage = makeStridedStorage(inputYLogical, incy);
  auto deviceYStorage = hostYStorage;

  hostGbmv(trans, m, n, kl, ku, alpha, inputAStorage, lda, hostXStorage, incx, beta, hostYStorage, incy);

  auto kernelId = launcher.loadKernel((opt.kernel_root / "gbmv" / "blas_gbmv_reference_fp32.elf").string());
  auto deviceA = launcher.allocateBytes(inputAStorage.size() * sizeof(float));
  auto deviceX = launcher.allocateBytes(hostXStorage.size() * sizeof(float));
  auto deviceY = launcher.allocateBytes(deviceYStorage.size() * sizeof(float));

  launcher.copyHostToDevice(inputAStorage.data(), deviceA, inputAStorage.size() * sizeof(float));
  launcher.copyHostToDevice(hostXStorage.data(), deviceX, hostXStorage.size() * sizeof(float));
  launcher.copyHostToDevice(deviceYStorage.data(), deviceY, deviceYStorage.size() * sizeof(float));

  GbmvArgs args{trans, m, n, kl, ku, alpha, reinterpret_cast<float*>(deviceA), lda,
                reinterpret_cast<float*>(deviceX), incx, beta, reinterpret_cast<float*>(deviceY), incy};
  launcher.kernelLaunch(kernelId, &args);
  launcher.copyDeviceToHost(deviceY, deviceYStorage.data(), deviceYStorage.size() * sizeof(float));
  launcher.waitKernelCompletion(std::chrono::seconds(opt.kernel_launch_timeout));

  const std::string caseName = trans == 'N' ? "gbmv_n" : "gbmv_t";
  std::ostringstream params;
  params << "trans=" << trans << ", m=" << m << ", n=" << n << ", kl=" << kl << ", ku=" << ku
         << ", lda=" << lda << ", incx=" << incx << ", incy=" << incy << ", alpha=" << alpha
         << ", beta=" << beta;
  const std::string paramString = params.str();
  const std::vector<float> hostOutputY =
    logicalFromStorage(hostYStorage, static_cast<int>(inputYLogical.size()), incy);
  const std::vector<float> deviceOutputY =
    logicalFromStorage(deviceYStorage, static_cast<int>(inputYLogical.size()), incy);
  const bool ok = !launcher.checkKernelExecutionErrors() && nearlyEqual(hostOutputY, deviceOutputY, opt.epsilon);

  launcher.freeBytes(deviceA);
  launcher.freeBytes(deviceX);
  launcher.freeBytes(deviceY);
  launcher.unLoadKernel(kernelId);

  VerificationPair result;
  result.host.name = caseName;
  result.host.params = paramString;
  result.host.ok = ok;
  result.host.vectors.push_back({"input_x", inputXLogical});
  result.host.vectors.push_back({"input_y", inputYLogical});
  result.host.vectors.push_back({"output_y", hostOutputY});
  result.host.matrices.push_back({"input_a", m, n, inputALogical});

  result.device.name = caseName;
  result.device.params = paramString;
  result.device.ok = ok;
  result.device.vectors.push_back({"input_x", inputXLogical});
  result.device.vectors.push_back({"input_y", inputYLogical});
  result.device.vectors.push_back({"output_y", deviceOutputY});
  result.device.matrices.push_back({"input_a", m, n, inputALogical});
  return result;
}

VerificationPair verifyGerCase(ReferenceVerifierLauncher& launcher, const Options& opt) {
  const int m = 4;
  const int n = 5;
  const int lda = 6;
  const int incx = 2;
  const int incy = 1;
  const float alpha = -0.75f;

  const auto inputAStorage = makeDenseStorage(m, n, lda);
  const auto inputALogical = denseFromColumnMajor(inputAStorage, m, n, lda);
  const auto inputXLogical = makeLogicalVector(m, 0.25f, 0.5f);
  const auto inputYLogical = makeLogicalVector(n, 1.5f, -0.125f);
  auto hostAStorage = inputAStorage;
  auto deviceAStorage = inputAStorage;
  auto hostXStorage = makeStridedStorage(inputXLogical, incx);
  auto hostYStorage = makeStridedStorage(inputYLogical, incy);

  hostGer(m, n, alpha, hostXStorage, incx, hostYStorage, incy, hostAStorage, lda);

  auto kernelId = launcher.loadKernel((opt.kernel_root / "ger" / "blas_ger_reference_fp32.elf").string());
  auto deviceA = launcher.allocateBytes(deviceAStorage.size() * sizeof(float));
  auto deviceX = launcher.allocateBytes(hostXStorage.size() * sizeof(float));
  auto deviceY = launcher.allocateBytes(hostYStorage.size() * sizeof(float));

  launcher.copyHostToDevice(deviceAStorage.data(), deviceA, deviceAStorage.size() * sizeof(float));
  launcher.copyHostToDevice(hostXStorage.data(), deviceX, hostXStorage.size() * sizeof(float));
  launcher.copyHostToDevice(hostYStorage.data(), deviceY, hostYStorage.size() * sizeof(float));

  GerArgs args{m, n, alpha, reinterpret_cast<float*>(deviceX), incx, reinterpret_cast<float*>(deviceY), incy,
               reinterpret_cast<float*>(deviceA), lda};
  launcher.kernelLaunch(kernelId, &args);
  launcher.copyDeviceToHost(deviceA, deviceAStorage.data(), deviceAStorage.size() * sizeof(float));
  launcher.waitKernelCompletion(std::chrono::seconds(opt.kernel_launch_timeout));

  std::ostringstream params;
  params << "m=" << m << ", n=" << n << ", lda=" << lda << ", incx=" << incx << ", incy=" << incy
         << ", alpha=" << alpha;
  const std::string paramString = params.str();
  const std::vector<float> hostOutputA = denseFromColumnMajor(hostAStorage, m, n, lda);
  const std::vector<float> deviceOutputA = denseFromColumnMajor(deviceAStorage, m, n, lda);
  const bool ok =
    !launcher.checkKernelExecutionErrors() && nearlyEqual(hostOutputA, deviceOutputA, opt.epsilon);

  launcher.freeBytes(deviceA);
  launcher.freeBytes(deviceX);
  launcher.freeBytes(deviceY);
  launcher.unLoadKernel(kernelId);

  VerificationPair result;
  result.host.name = "ger";
  result.host.params = paramString;
  result.host.ok = ok;
  result.host.vectors.push_back({"input_x", inputXLogical});
  result.host.vectors.push_back({"input_y", inputYLogical});
  result.host.matrices.push_back({"input_a", m, n, inputALogical});
  result.host.matrices.push_back({"output_a", m, n, hostOutputA});

  result.device.name = "ger";
  result.device.params = paramString;
  result.device.ok = ok;
  result.device.vectors.push_back({"input_x", inputXLogical});
  result.device.vectors.push_back({"input_y", inputYLogical});
  result.device.matrices.push_back({"input_a", m, n, inputALogical});
  result.device.matrices.push_back({"output_a", m, n, deviceOutputA});
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

  std::vector<CaseResult> hostResults;
  std::vector<CaseResult> deviceResults;

  const auto appendCase = [&](VerificationPair pair) {
    hostResults.push_back(std::move(pair.host));
    deviceResults.push_back(std::move(pair.device));
  };

  if (opt.selected_case == "all" || opt.selected_case == "gemv_n") {
    appendCase(verifyGemvCase(launcher, opt, 'N'));
  }
  if (opt.selected_case == "all" || opt.selected_case == "gemv_t") {
    appendCase(verifyGemvCase(launcher, opt, 'T'));
  }
  if (opt.selected_case == "all" || opt.selected_case == "gbmv_n") {
    appendCase(verifyGbmvCase(launcher, opt, 'N'));
  }
  if (opt.selected_case == "all" || opt.selected_case == "gbmv_t") {
    appendCase(verifyGbmvCase(launcher, opt, 'T'));
  }
  if (opt.selected_case == "all" || opt.selected_case == "ger") {
    appendCase(verifyGerCase(launcher, opt));
  }

  launcher.tearDown();

  if (deviceResults.empty()) {
    std::cerr << "Unknown case: " << opt.selected_case << std::endl;
    return 1;
  }

  writeResultsMarkdown(opt.host_results_path, "Host", opt, hostResults);
  writeResultsMarkdown(opt.device_results_path, "Device", opt, deviceResults);

  const bool ok = std::all_of(deviceResults.begin(), deviceResults.end(), [](const CaseResult& result) {
    return result.ok;
  });
  if (!ok) {
    std::cerr << "BLAS Level 2 reference verification failed" << std::endl;
    return 1;
  }

  std::cout << "BLAS Level 2 reference verification passed" << std::endl;
  return 0;
}
