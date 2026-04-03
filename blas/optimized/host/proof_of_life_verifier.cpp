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

struct AxpyArgs {
  uint64_t numElements;
  const float* x;
  float* y;
  float alpha;
} __attribute__((packed));

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

struct GemmArgs {
  char transa;
  char transb;
  int32_t m;
  int32_t n;
  int32_t k;
  float alpha;
  const float* a;
  int32_t lda;
  const float* b;
  int32_t ldb;
  float beta;
  float* c;
  int32_t ldc;
};

struct Options {
  fs::path kernel_root = "";
  fs::path host_results_path = "blas/optimized/proof_of_life_host.md";
  fs::path device_results_path = "blas/optimized/proof_of_life_device.md";
  int kernel_launch_timeout = 30;
  std::string device_type = "silicon";
  int saxpy_num_elements = 259;
  int gemv_m = 17;
  int gemv_n = 19;
  int gemm_m = 17;
  int gemm_n = 15;
  int gemm_k = 13;
  double epsilon = 1.0e-5;
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
  fs::path kernel_artifact;
  bool ok = false;
  std::vector<VectorRecord> vectors;
  std::vector<MatrixRecord> matrices;
};

Options parseArgs(int argc, char* const* argv, std::vector<char*>& nextlevel) {
  static constexpr const char* helpMsg =
    "Usage: [options]\n\n"
    "Proof-of-life verifier for optimized SAXPY/SGEMV/SGEMM kernels.\n\n"
    "Optional switches:\n"
    "  -k, --kernel_root            root containing optimized fp32 kernels\n"
    "      --host_results_path      markdown file for host reference outputs\n"
    "      --device_results_path    markdown file for device outputs\n"
    "  -t, --kernel_launch_timeout  timeout (in seconds) to wait for kernel completion\n"
    "  -d, --device_type            device type to use (sysemu, fake, silicon)\n"
    "      --saxpy_num_elements     vector length for SAXPY\n"
    "      --gemv_m                 matrix rows for GEMV\n"
    "      --gemv_n                 matrix columns for GEMV\n"
    "      --gemm_m                 output rows for GEMM\n"
    "      --gemm_n                 output columns for GEMM\n"
    "      --gemm_k                 reduction dimension for GEMM\n"
    "  -e, --epsilon                comparison tolerance for float results\n";

  static constexpr const char* shortOpts = "k:t:d:e:h";
  static const std::vector<option> longOpts{
    {"kernel_root", required_argument, nullptr, 'k'},
    {"host_results_path", required_argument, nullptr, 1000},
    {"device_results_path", required_argument, nullptr, 1001},
    {"kernel_launch_timeout", required_argument, nullptr, 't'},
    {"device_type", required_argument, nullptr, 'd'},
    {"saxpy_num_elements", required_argument, nullptr, 1002},
    {"gemv_m", required_argument, nullptr, 1003},
    {"gemv_n", required_argument, nullptr, 1004},
    {"gemm_m", required_argument, nullptr, 1005},
    {"gemm_n", required_argument, nullptr, 1006},
    {"gemm_k", required_argument, nullptr, 1007},
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
    case 1002:
      opts.saxpy_num_elements = std::atoi(optarg);
      break;
    case 1003:
      opts.gemv_m = std::atoi(optarg);
      break;
    case 1004:
      opts.gemv_n = std::atoi(optarg);
      break;
    case 1005:
      opts.gemm_m = std::atoi(optarg);
      break;
    case 1006:
      opts.gemm_n = std::atoi(optarg);
      break;
    case 1007:
      opts.gemm_k = std::atoi(optarg);
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

fs::path defaultKernelRoot() {
  if (const char* envRoot = std::getenv("ET_BLAS_OPTIMIZED_KERNEL_ROOT")) {
    return envRoot;
  }
  return "/opt/et/kernels/blas/optimized/fp32";
}

fs::path resolveKernelArtifact(const fs::path& kernelDir, const std::string& kernelBaseName) {
  const auto dbgPath = kernelDir / (kernelBaseName + ".elf_dbg");
  if (std::filesystem::exists(dbgPath)) {
    return dbgPath;
  }

  const auto elfPath = kernelDir / (kernelBaseName + ".elf");
  if (std::filesystem::exists(elfPath)) {
    return elfPath;
  }

  return dbgPath;
}

bool nearlyEqual(float lhs, float rhs, double epsilon) {
  const double diff = std::fabs(static_cast<double>(lhs) - static_cast<double>(rhs));
  const double scale = std::max({1.0, std::fabs(static_cast<double>(lhs)), std::fabs(static_cast<double>(rhs))});
  return diff <= epsilon * scale;
}

bool nearlyEqual(const std::vector<float>& lhs, const std::vector<float>& rhs, double epsilon) {
  return std::equal(lhs.begin(), lhs.end(), rhs.begin(), [epsilon](float a, float b) {
    return nearlyEqual(a, b, epsilon);
  });
}

std::vector<float> makeVector(int size, float base, float step) {
  std::vector<float> values(static_cast<size_t>(size));
  for (int i = 0; i < size; ++i) {
    values[static_cast<size_t>(i)] = base + static_cast<float>(i) * step;
  }
  return values;
}

std::vector<float> makeDenseStorage(int rows, int cols, int ld, float base) {
  std::vector<float> storage(static_cast<size_t>(ld) * cols, -999.0f);
  for (int col = 0; col < cols; ++col) {
    for (int row = 0; row < rows; ++row) {
      storage[row + static_cast<size_t>(col) * ld] =
        base + static_cast<float>(row) * 0.75f + static_cast<float>(col) * 0.25f;
    }
  }
  return storage;
}

std::vector<float> denseFromColumnMajor(const std::vector<float>& storage, int rows, int cols, int ld) {
  std::vector<float> dense(static_cast<size_t>(rows) * cols);
  for (int row = 0; row < rows; ++row) {
    for (int col = 0; col < cols; ++col) {
      dense[static_cast<size_t>(row) * cols + col] = storage[row + static_cast<size_t>(col) * ld];
    }
  }
  return dense;
}

bool isTranspose(char trans) {
  return trans == 'T' || trans == 't' || trans == 'C' || trans == 'c';
}

int startIndex(int length, int inc) {
  return inc >= 0 ? 0 : (length - 1) * (-inc);
}

float generalElement(const std::vector<float>& storage, int ld, int row, int col, bool transpose) {
  return transpose ? storage[col + static_cast<size_t>(row) * ld] : storage[row + static_cast<size_t>(col) * ld];
}

void hostAxpy(const std::vector<float>& x, std::vector<float>& y, float alpha) {
  for (size_t i = 0; i < x.size(); ++i) {
    y[i] = alpha * x[i] + y[i];
  }
}

void hostGemv(char trans, int m, int n, float alpha, const std::vector<float>& a, int lda,
              const std::vector<float>& x, int incx, float beta, std::vector<float>& y, int incy) {
  const bool transpose = isTranspose(trans);
  const int outputCount = transpose ? n : m;
  const int reductionCount = transpose ? m : n;
  const int xStart = startIndex(reductionCount, incx);
  const int yStart = startIndex(outputCount, incy);

  for (int outputIndex = 0; outputIndex < outputCount; ++outputIndex) {
    float sum = 0.0f;
    if (!transpose) {
      for (int column = 0; column < n; ++column) {
        sum += a[outputIndex + static_cast<size_t>(column) * lda] * x[xStart + column * incx];
      }
    } else {
      for (int row = 0; row < m; ++row) {
        sum += a[static_cast<size_t>(row) + outputIndex * lda] * x[xStart + row * incx];
      }
    }
    float& yValue = y[yStart + outputIndex * incy];
    yValue = alpha * sum + beta * yValue;
  }
}

void hostGemm(char transa, char transb, int m, int n, int k, float alpha, const std::vector<float>& a, int lda,
              const std::vector<float>& b, int ldb, float beta, std::vector<float>& c, int ldc) {
  const bool transA = isTranspose(transa);
  const bool transB = isTranspose(transb);
  for (int col = 0; col < n; ++col) {
    for (int row = 0; row < m; ++row) {
      float sum = 0.0f;
      for (int inner = 0; inner < k; ++inner) {
        sum += generalElement(a, lda, row, inner, transA) * generalElement(b, ldb, inner, col, transB);
      }
      float& cValue = c[row + static_cast<size_t>(col) * ldc];
      cValue = alpha * sum + beta * cValue;
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
  out << "| row | col | value |\n";
  out << "| ---: | ---: | ---: |\n";
  out << std::fixed << std::setprecision(6);
  for (int row = 0; row < record.rows; ++row) {
    for (int col = 0; col < record.cols; ++col) {
      out << "| " << row << " | " << col << " | "
          << record.values[static_cast<size_t>(row) * record.cols + col] << " |\n";
    }
  }
  out << "\n";
}

void writeResults(const fs::path& path, const std::string& side, const Options& opt,
                  const std::vector<CaseResult>& cases) {
  if (path.has_parent_path()) {
    std::filesystem::create_directories(path.parent_path());
  }

  std::ofstream out(path);
  if (!out.is_open()) {
    throw std::runtime_error("Unable to open results file: " + path.string());
  }

  out << "# Optimized BLAS Proof Of Life " << side << " Results\n\n";
  out << "- device_type: `" << opt.device_type << "`\n";
  out << "- kernel_root: `" << opt.kernel_root.string() << "`\n";
  out << "- epsilon: `" << opt.epsilon << "`\n\n";

  for (const auto& result : cases) {
    out << "## " << result.name << "\n\n";
    out << "- params: `" << result.params << "`\n";
    out << "- kernel_artifact: `" << result.kernel_artifact.string() << "`\n";
    out << "- status: `" << (result.ok ? "ok" : "mismatch") << "`\n\n";
    for (const auto& vector : result.vectors) {
      writeVector(out, vector);
    }
    for (const auto& matrix : result.matrices) {
      writeMatrix(out, matrix);
    }
  }
}

class ProofLauncher : public GenericLauncher {
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

CaseResult verifySaxpy(ProofLauncher& launcher, const Options& opt) {
  CaseResult result;
  result.name = "saxpy";
  result.params = "n=" + std::to_string(opt.saxpy_num_elements);
  result.kernel_artifact = resolveKernelArtifact(opt.kernel_root / "level1" / "axpy", "blas_axpy_optimized_fp32");

  const float alpha = 2.5f;
  auto x = makeVector(opt.saxpy_num_elements, 1.0f, 0.25f);
  auto yInitial = makeVector(opt.saxpy_num_elements, 100.0f, -0.5f);
  auto deviceY = yInitial;
  auto expectedY = yInitial;
  hostAxpy(x, expectedY, alpha);

  auto kernelId = launcher.loadKernel(result.kernel_artifact.string());
  auto deviceX = launcher.allocateBytes(x.size() * sizeof(float));
  auto deviceYBuffer = launcher.allocateBytes(deviceY.size() * sizeof(float));
  launcher.copyHostToDevice(x.data(), deviceX, x.size() * sizeof(float));
  launcher.copyHostToDevice(deviceY.data(), deviceYBuffer, deviceY.size() * sizeof(float));

  AxpyArgs args{static_cast<uint64_t>(x.size()), reinterpret_cast<float*>(deviceX),
                reinterpret_cast<float*>(deviceYBuffer), alpha};
  launcher.kernelLaunch(kernelId, &args);
  launcher.waitKernelCompletion(std::chrono::seconds(opt.kernel_launch_timeout));
  launcher.copyDeviceToHost(deviceYBuffer, deviceY.data(), deviceY.size() * sizeof(float));
  launcher.waitKernelCompletion(std::chrono::seconds(opt.kernel_launch_timeout));

  result.ok = !launcher.checkKernelExecutionErrors() && nearlyEqual(expectedY, deviceY, opt.epsilon);
  result.vectors.push_back({"input_x", x});
  result.vectors.push_back({"input_y", yInitial});
  result.vectors.push_back({"output_y", deviceY});

  launcher.freeBytes(deviceX);
  launcher.freeBytes(deviceYBuffer);
  launcher.unLoadKernel(kernelId);
  return result;
}

CaseResult verifySgemv(ProofLauncher& launcher, const Options& opt) {
  CaseResult result;
  result.name = "sgemv_n";
  result.params = "m=" + std::to_string(opt.gemv_m) + ",n=" + std::to_string(opt.gemv_n);
  result.kernel_artifact = resolveKernelArtifact(opt.kernel_root / "level2" / "gemv", "blas_gemv_optimized_fp32");

  const int lda = opt.gemv_m;
  const float alpha = 1.75f;
  const float beta = -0.5f;
  auto a = makeDenseStorage(opt.gemv_m, opt.gemv_n, lda, 0.5f);
  auto x = makeVector(opt.gemv_n, 1.0f, 0.2f);
  auto yInitial = makeVector(opt.gemv_m, 5.0f, -0.1f);
  auto deviceY = yInitial;
  auto expectedY = yInitial;
  hostGemv('N', opt.gemv_m, opt.gemv_n, alpha, a, lda, x, 1, beta, expectedY, 1);

  auto kernelId = launcher.loadKernel(result.kernel_artifact.string());
  auto deviceA = launcher.allocateBytes(a.size() * sizeof(float));
  auto deviceX = launcher.allocateBytes(x.size() * sizeof(float));
  auto deviceYBuffer = launcher.allocateBytes(deviceY.size() * sizeof(float));
  launcher.copyHostToDevice(a.data(), deviceA, a.size() * sizeof(float));
  launcher.copyHostToDevice(x.data(), deviceX, x.size() * sizeof(float));
  launcher.copyHostToDevice(deviceY.data(), deviceYBuffer, deviceY.size() * sizeof(float));

  GemvArgs args{'N', opt.gemv_m, opt.gemv_n, alpha, reinterpret_cast<float*>(deviceA), lda,
                reinterpret_cast<float*>(deviceX), 1, beta, reinterpret_cast<float*>(deviceYBuffer), 1};
  launcher.kernelLaunch(kernelId, &args);
  launcher.waitKernelCompletion(std::chrono::seconds(opt.kernel_launch_timeout));
  launcher.copyDeviceToHost(deviceYBuffer, deviceY.data(), deviceY.size() * sizeof(float));
  launcher.waitKernelCompletion(std::chrono::seconds(opt.kernel_launch_timeout));

  result.ok = !launcher.checkKernelExecutionErrors() && nearlyEqual(expectedY, deviceY, opt.epsilon);
  result.vectors.push_back({"input_x", x});
  result.vectors.push_back({"input_y", yInitial});
  result.vectors.push_back({"output_y", deviceY});
  result.matrices.push_back({"input_a", opt.gemv_m, opt.gemv_n, denseFromColumnMajor(a, opt.gemv_m, opt.gemv_n, lda)});

  launcher.freeBytes(deviceA);
  launcher.freeBytes(deviceX);
  launcher.freeBytes(deviceYBuffer);
  launcher.unLoadKernel(kernelId);
  return result;
}

CaseResult verifySgemm(ProofLauncher& launcher, const Options& opt) {
  CaseResult result;
  result.name = "sgemm_nn";
  result.params = "m=" + std::to_string(opt.gemm_m) + ",n=" + std::to_string(opt.gemm_n) +
                  ",k=" + std::to_string(opt.gemm_k);
  result.kernel_artifact = resolveKernelArtifact(opt.kernel_root / "level3" / "gemm", "blas_gemm_optimized_fp32");

  const int lda = opt.gemm_m;
  const int ldb = opt.gemm_k;
  const int ldc = opt.gemm_m;
  const float alpha = 1.25f;
  const float beta = -0.25f;
  auto a = makeDenseStorage(opt.gemm_m, opt.gemm_k, lda, 0.75f);
  auto b = makeDenseStorage(opt.gemm_k, opt.gemm_n, ldb, 1.25f);
  auto cInitial = makeDenseStorage(opt.gemm_m, opt.gemm_n, ldc, 2.0f);
  auto deviceC = cInitial;
  auto expectedC = cInitial;
  hostGemm('N', 'N', opt.gemm_m, opt.gemm_n, opt.gemm_k, alpha, a, lda, b, ldb, beta, expectedC, ldc);

  auto kernelId = launcher.loadKernel(result.kernel_artifact.string());
  auto deviceA = launcher.allocateBytes(a.size() * sizeof(float));
  auto deviceB = launcher.allocateBytes(b.size() * sizeof(float));
  auto deviceCBuffer = launcher.allocateBytes(deviceC.size() * sizeof(float));
  launcher.copyHostToDevice(a.data(), deviceA, a.size() * sizeof(float));
  launcher.copyHostToDevice(b.data(), deviceB, b.size() * sizeof(float));
  launcher.copyHostToDevice(deviceC.data(), deviceCBuffer, deviceC.size() * sizeof(float));

  GemmArgs args{'N', 'N', opt.gemm_m, opt.gemm_n, opt.gemm_k, alpha, reinterpret_cast<float*>(deviceA), lda,
                reinterpret_cast<float*>(deviceB), ldb, beta, reinterpret_cast<float*>(deviceCBuffer), ldc};
  launcher.kernelLaunch(kernelId, &args);
  launcher.waitKernelCompletion(std::chrono::seconds(opt.kernel_launch_timeout));
  launcher.copyDeviceToHost(deviceCBuffer, deviceC.data(), deviceC.size() * sizeof(float));
  launcher.waitKernelCompletion(std::chrono::seconds(opt.kernel_launch_timeout));

  result.ok = !launcher.checkKernelExecutionErrors() && nearlyEqual(expectedC, deviceC, opt.epsilon);
  result.matrices.push_back({"input_a", opt.gemm_m, opt.gemm_k, denseFromColumnMajor(a, opt.gemm_m, opt.gemm_k, lda)});
  result.matrices.push_back({"input_b", opt.gemm_k, opt.gemm_n, denseFromColumnMajor(b, opt.gemm_k, opt.gemm_n, ldb)});
  result.matrices.push_back({"input_c", opt.gemm_m, opt.gemm_n, denseFromColumnMajor(cInitial, opt.gemm_m, opt.gemm_n, ldc)});
  result.matrices.push_back({"output_c", opt.gemm_m, opt.gemm_n, denseFromColumnMajor(deviceC, opt.gemm_m, opt.gemm_n, ldc)});

  launcher.freeBytes(deviceA);
  launcher.freeBytes(deviceB);
  launcher.freeBytes(deviceCBuffer);
  launcher.unLoadKernel(kernelId);
  return result;
}

} // namespace

int main(int argc, char** argv) {
  std::vector<char*> argvPendingToParse{argv[0]};
  Options opt = parseArgs(argc, argv, argvPendingToParse);

  if (opt.kernel_root.empty()) {
    opt.kernel_root = defaultKernelRoot();
  }

  Config config{modeFromString(opt.device_type), 1};
  config.dump();

  ProofLauncher launcher(config, static_cast<int>(argvPendingToParse.size()), argvPendingToParse.data());
  launcher.initialize();

  const CaseResult deviceSaxpy = verifySaxpy(launcher, opt);
  const CaseResult deviceSgemv = verifySgemv(launcher, opt);
  const CaseResult deviceSgemm = verifySgemm(launcher, opt);

  launcher.tearDown();

  std::vector<CaseResult> hostResults;
  std::vector<CaseResult> deviceResults;
  hostResults.push_back(deviceSaxpy);
  hostResults.push_back(deviceSgemv);
  hostResults.push_back(deviceSgemm);
  deviceResults.push_back(deviceSaxpy);
  deviceResults.push_back(deviceSgemv);
  deviceResults.push_back(deviceSgemm);

  writeResults(opt.host_results_path, "Host", opt, hostResults);
  writeResults(opt.device_results_path, "Device", opt, deviceResults);

  if (!deviceSaxpy.ok || !deviceSgemv.ok || !deviceSgemm.ok) {
    std::cerr << "Optimized BLAS proof-of-life verification failed" << std::endl;
    return 1;
  }

  std::cout << "Optimized BLAS proof-of-life verification passed" << std::endl;
  return 0;
}
