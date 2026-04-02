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

struct SymmArgs {
  char side;
  char uplo;
  int32_t m;
  int32_t n;
  float alpha;
  const float* a;
  int32_t lda;
  const float* b;
  int32_t ldb;
  float beta;
  float* c;
  int32_t ldc;
};

struct SyrkArgs {
  char uplo;
  char trans;
  int32_t n;
  int32_t k;
  float alpha;
  const float* a;
  int32_t lda;
  float beta;
  float* c;
  int32_t ldc;
};

struct Syr2kArgs {
  char uplo;
  char trans;
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

struct TrmmArgs {
  char side;
  char uplo;
  char transa;
  char diag;
  int32_t m;
  int32_t n;
  float alpha;
  const float* a;
  int32_t lda;
  float* b;
  int32_t ldb;
};

struct TrsmArgs {
  char side;
  char uplo;
  char transa;
  char diag;
  int32_t m;
  int32_t n;
  float alpha;
  const float* a;
  int32_t lda;
  float* b;
  int32_t ldb;
};

struct Options {
  fs::path kernel_root = "";
  fs::path host_results_path = "blas/reference/host_level3_results.md";
  fs::path device_results_path = "blas/reference/device_level3_results.md";
  int kernel_launch_timeout = 30;
  std::string device_type = "sysemu";
  double epsilon = 1.0e-5;
  std::string selected_case = "all";
  int problem_dim = 4;
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
  std::vector<MatrixRecord> matrices;
};

struct VerificationPair {
  CaseResult host;
  CaseResult device;
};

Options parse_args(int argc, char* const* argv, std::vector<char*>& nextlevel) {
  static constexpr const char* help_msg =
    "Usage: [options]\n\n"
    "Quick verifier for BLAS reference Level 3 kernels.\n\n"
    "Optional switches:\n"
    "  -k, --kernel_root            root containing Level 3 kernel directories\n"
    "      --host_results_path      markdown file for host reference outputs\n"
    "      --device_results_path    markdown file for device outputs\n"
    "      --case                   case to run (all, gemm_nn, symm_lu, syrk_un,\n"
    "                               syr2k_lt, trmm_lunn, trsm_lunn)\n"
    "  -n, --problem_dim            logical matrix dimension used by the verifier\n"
    "  -t, --kernel_launch_timeout  timeout (in seconds) to wait for kernel completion\n"
    "  -d, --device_type            device type to use (sysemu, fake, silicon)\n"
    "  -e, --epsilon                comparison tolerance for float results\n";

  static constexpr const char* short_opts = "k:t:d:e:n:h";
  static const std::vector<option> long_opts{
    {"kernel_root", required_argument, nullptr, 'k'},
    {"host_results_path", required_argument, nullptr, 1000},
    {"device_results_path", required_argument, nullptr, 1001},
    {"case", required_argument, nullptr, 1002},
    {"problem_dim", required_argument, nullptr, 'n'},
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
    case 'n':
      opts.problem_dim = std::atoi(optarg);
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
  return "/opt/et/kernels/blas/reference/fp32/level3";
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

  return elfPath;
}

bool isTranspose(char trans) {
  return trans == 'T' || trans == 't' || trans == 'C' || trans == 'c';
}

bool isUpper(char uplo) {
  return uplo == 'U' || uplo == 'u';
}

bool isLeft(char side) {
  return side == 'L' || side == 'l';
}

bool isUnit(char diag) {
  return diag == 'U' || diag == 'u';
}

std::vector<float> makeDenseStorage(int rows, int cols, int ld, float base = 0.5f) {
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
      dense[row * cols + col] = storage[row + static_cast<size_t>(col) * ld];
    }
  }
  return dense;
}

std::vector<float> makeSymmetricStorage(int n, int ld, char uplo) {
  std::vector<float> storage(static_cast<size_t>(ld) * n, -999.0f);
  const bool upper = isUpper(uplo);
  for (int col = 0; col < n; ++col) {
    const int rowBegin = upper ? 0 : col;
    const int rowEnd = upper ? (col + 1) : n;
    for (int row = rowBegin; row < rowEnd; ++row) {
      storage[row + static_cast<size_t>(col) * ld] =
        1.0f + static_cast<float>(row) * 0.5f + static_cast<float>(col) * 0.125f;
    }
  }
  return storage;
}

std::vector<float> makeTriangularStorage(int n, int ld, char uplo, char diag) {
  std::vector<float> storage(static_cast<size_t>(ld) * n, 0.0f);
  const bool upper = isUpper(uplo);
  const bool unit = isUnit(diag);
  for (int col = 0; col < n; ++col) {
    const int rowBegin = upper ? 0 : col;
    const int rowEnd = upper ? (col + 1) : n;
    for (int row = rowBegin; row < rowEnd; ++row) {
      if (unit && row == col) {
        storage[row + static_cast<size_t>(col) * ld] = 1.0f;
      } else if (row == col) {
        // Keep triangular solves well-conditioned across larger dimensions.
        storage[row + static_cast<size_t>(col) * ld] =
          static_cast<float>(4 * n) + 1.0f + static_cast<float>(row) * 0.25f;
      } else {
        storage[row + static_cast<size_t>(col) * ld] =
          0.05f + static_cast<float>(row) * 0.001f + static_cast<float>(col) * 0.0005f;
      }
    }
  }
  return storage;
}

float generalElement(const std::vector<float>& storage, int ld, int row, int col, bool transpose) {
  return transpose ? storage[col + static_cast<size_t>(row) * ld] : storage[row + static_cast<size_t>(col) * ld];
}

float symmetricElement(const std::vector<float>& storage, int ld, int row, int col, char uplo) {
  if (isUpper(uplo)) {
    return row <= col ? storage[row + static_cast<size_t>(col) * ld]
                      : storage[col + static_cast<size_t>(row) * ld];
  }
  return row >= col ? storage[row + static_cast<size_t>(col) * ld]
                    : storage[col + static_cast<size_t>(row) * ld];
}

std::vector<float> denseFromSymmetricStorage(const std::vector<float>& storage, int n, int ld, char uplo) {
  std::vector<float> dense(static_cast<size_t>(n) * n);
  for (int row = 0; row < n; ++row) {
    for (int col = 0; col < n; ++col) {
      dense[row * n + col] = symmetricElement(storage, ld, row, col, uplo);
    }
  }
  return dense;
}

float triangularElement(const std::vector<float>& storage, int ld, int row, int col, char uplo, char transa, char diag) {
  if (isUnit(diag) && row == col) {
    return 1.0f;
  }
  const bool upper = isUpper(uplo);
  const bool transpose = isTranspose(transa);
  if (!transpose) {
    if (upper && row > col) {
      return 0.0f;
    }
    if (!upper && row < col) {
      return 0.0f;
    }
    return storage[row + static_cast<size_t>(col) * ld];
  }
  if (upper && row < col) {
    return 0.0f;
  }
  if (!upper && row > col) {
    return 0.0f;
  }
  return storage[col + static_cast<size_t>(row) * ld];
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
      c[row + static_cast<size_t>(col) * ldc] = alpha * sum + beta * c[row + static_cast<size_t>(col) * ldc];
    }
  }
}

void hostSymm(char side, char uplo, int m, int n, float alpha, const std::vector<float>& a, int lda,
              const std::vector<float>& b, int ldb, float beta, std::vector<float>& c, int ldc) {
  const bool left = isLeft(side);
  for (int col = 0; col < n; ++col) {
    for (int row = 0; row < m; ++row) {
      float sum = 0.0f;
      if (left) {
        for (int inner = 0; inner < m; ++inner) {
          sum += symmetricElement(a, lda, row, inner, uplo) * b[inner + static_cast<size_t>(col) * ldb];
        }
      } else {
        for (int inner = 0; inner < n; ++inner) {
          sum += b[row + static_cast<size_t>(inner) * ldb] * symmetricElement(a, lda, inner, col, uplo);
        }
      }
      c[row + static_cast<size_t>(col) * ldc] = alpha * sum + beta * c[row + static_cast<size_t>(col) * ldc];
    }
  }
}

void hostSyrk(char uplo, char trans, int n, int k, float alpha, const std::vector<float>& a, int lda, float beta,
              std::vector<float>& c, int ldc) {
  const bool transpose = isTranspose(trans);
  const bool upper = isUpper(uplo);
  for (int col = 0; col < n; ++col) {
    const int rowBegin = upper ? 0 : col;
    const int rowEnd = upper ? (col + 1) : n;
    for (int row = rowBegin; row < rowEnd; ++row) {
      float sum = 0.0f;
      for (int inner = 0; inner < k; ++inner) {
        sum += generalElement(a, lda, row, inner, transpose) * generalElement(a, lda, col, inner, transpose);
      }
      c[row + static_cast<size_t>(col) * ldc] = alpha * sum + beta * c[row + static_cast<size_t>(col) * ldc];
    }
  }
}

void hostSyr2k(char uplo, char trans, int n, int k, float alpha, const std::vector<float>& a, int lda,
               const std::vector<float>& b, int ldb, float beta, std::vector<float>& c, int ldc) {
  const bool transpose = isTranspose(trans);
  const bool upper = isUpper(uplo);
  for (int col = 0; col < n; ++col) {
    const int rowBegin = upper ? 0 : col;
    const int rowEnd = upper ? (col + 1) : n;
    for (int row = rowBegin; row < rowEnd; ++row) {
      float sum = 0.0f;
      for (int inner = 0; inner < k; ++inner) {
        sum += generalElement(a, lda, row, inner, transpose) * generalElement(b, ldb, col, inner, transpose) +
               generalElement(b, ldb, row, inner, transpose) * generalElement(a, lda, col, inner, transpose);
      }
      c[row + static_cast<size_t>(col) * ldc] = alpha * sum + beta * c[row + static_cast<size_t>(col) * ldc];
    }
  }
}

void hostTrmm(char side, char uplo, char transa, char diag, int m, int n, float alpha, const std::vector<float>& a,
              int lda, std::vector<float>& b, int ldb) {
  const std::vector<float> inputB = b;
  const bool left = isLeft(side);
  if (left) {
    for (int col = 0; col < n; ++col) {
      for (int row = 0; row < m; ++row) {
        float sum = 0.0f;
        for (int inner = 0; inner < m; ++inner) {
          sum += triangularElement(a, lda, row, inner, uplo, transa, diag) * inputB[inner + static_cast<size_t>(col) * ldb];
        }
        b[row + static_cast<size_t>(col) * ldb] = alpha * sum;
      }
    }
  } else {
    for (int col = 0; col < n; ++col) {
      for (int row = 0; row < m; ++row) {
        float sum = 0.0f;
        for (int inner = 0; inner < n; ++inner) {
          sum += inputB[row + static_cast<size_t>(inner) * ldb] * triangularElement(a, lda, inner, col, uplo, transa, diag);
        }
        b[row + static_cast<size_t>(col) * ldb] = alpha * sum;
      }
    }
  }
}

void hostTrsm(char side, char uplo, char transa, char diag, int m, int n, float alpha, const std::vector<float>& a,
              int lda, std::vector<float>& b, int ldb) {
  const bool left = isLeft(side);
  const bool effectiveUpper = isTranspose(transa) ? !isUpper(uplo) : isUpper(uplo);
  for (int col = 0; col < n; ++col) {
    for (int row = 0; row < m; ++row) {
      b[row + static_cast<size_t>(col) * ldb] *= alpha;
    }
  }

  if (left) {
    const int rowStart = effectiveUpper ? (m - 1) : 0;
    const int rowStop = effectiveUpper ? -1 : m;
    const int rowStep = effectiveUpper ? -1 : 1;
    for (int row = rowStart; row != rowStop; row += rowStep) {
      if (!isUnit(diag)) {
        const float diagonal = triangularElement(a, lda, row, row, uplo, transa, diag);
        for (int col = 0; col < n; ++col) {
          b[row + static_cast<size_t>(col) * ldb] /= diagonal;
        }
      }
      if (effectiveUpper) {
        for (int inner = 0; inner < row; ++inner) {
          const float coeff = triangularElement(a, lda, inner, row, uplo, transa, diag);
          if (coeff == 0.0f) {
            continue;
          }
          for (int col = 0; col < n; ++col) {
            b[inner + static_cast<size_t>(col) * ldb] -= coeff * b[row + static_cast<size_t>(col) * ldb];
          }
        }
      } else {
        for (int inner = row + 1; inner < m; ++inner) {
          const float coeff = triangularElement(a, lda, inner, row, uplo, transa, diag);
          if (coeff == 0.0f) {
            continue;
          }
          for (int col = 0; col < n; ++col) {
            b[inner + static_cast<size_t>(col) * ldb] -= coeff * b[row + static_cast<size_t>(col) * ldb];
          }
        }
      }
    }
  } else {
    const int colStart = effectiveUpper ? 0 : (n - 1);
    const int colStop = effectiveUpper ? n : -1;
    const int colStep = effectiveUpper ? 1 : -1;
    for (int col = colStart; col != colStop; col += colStep) {
      if (!isUnit(diag)) {
        const float diagonal = triangularElement(a, lda, col, col, uplo, transa, diag);
        for (int row = 0; row < m; ++row) {
          b[row + static_cast<size_t>(col) * ldb] /= diagonal;
        }
      }
      if (effectiveUpper) {
        for (int inner = col + 1; inner < n; ++inner) {
          const float coeff = triangularElement(a, lda, col, inner, uplo, transa, diag);
          if (coeff == 0.0f) {
            continue;
          }
          for (int row = 0; row < m; ++row) {
            b[row + static_cast<size_t>(inner) * ldb] -= b[row + static_cast<size_t>(col) * ldb] * coeff;
          }
        }
      } else {
        for (int inner = 0; inner < col; ++inner) {
          const float coeff = triangularElement(a, lda, col, inner, uplo, transa, diag);
          if (coeff == 0.0f) {
            continue;
          }
          for (int row = 0; row < m; ++row) {
            b[row + static_cast<size_t>(inner) * ldb] -= b[row + static_cast<size_t>(col) * ldb] * coeff;
          }
        }
      }
    }
  }
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

  out << "# BLAS Level 3 " << side << " Results\n\n";
  out << "- device_type: `" << opt.device_type << "`\n";
  out << "- epsilon: `" << opt.epsilon << "`\n";
  out << "- case: `" << opt.selected_case << "`\n";
  out << "- kernel_root: `" << opt.kernel_root.string() << "`\n\n";
  for (const auto& result : results) {
    out << "## " << result.name << "\n\n";
    out << "- status: `" << (result.ok ? "ok" : "mismatch") << "`\n";
    out << "- params: `" << result.params << "`\n\n";
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
    runtime_->memcpyHostToDevice(defaultStreams_.at(deviceIdx), reinterpret_cast<const std::byte*>(src), dst, bytes);
  }

  void copyDeviceToHost(const std::byte* src, void* dst, size_t bytes, uint32_t deviceIdx = 0) {
    runtime_->memcpyDeviceToHost(defaultStreams_.at(deviceIdx), src, reinterpret_cast<std::byte*>(dst), bytes);
  }
};

VerificationPair verifyGemmCase(ReferenceVerifierLauncher& launcher, const Options& opt) {
  const int m = opt.problem_dim;
  const int n = opt.problem_dim;
  const int k = opt.problem_dim;
  const int lda = m + 1;
  const int ldb = k + 1;
  const int ldc = m + 1;
  const float alpha = 1.25f;
  const float beta = -0.5f;
  auto aStorage = makeDenseStorage(m, k, lda, 0.75f);
  auto bStorage = makeDenseStorage(k, n, ldb, 1.25f);
  auto hostCStorage = makeDenseStorage(m, n, ldc, -0.25f);
  auto deviceCStorage = hostCStorage;

  hostGemm('N', 'N', m, n, k, alpha, aStorage, lda, bStorage, ldb, beta, hostCStorage, ldc);

  auto kernelId =
    launcher.loadKernel(resolveKernelArtifact(opt.kernel_root / "gemm", "blas_gemm_reference_fp32").string());
  auto deviceA = launcher.allocateBytes(aStorage.size() * sizeof(float));
  auto deviceB = launcher.allocateBytes(bStorage.size() * sizeof(float));
  auto deviceC = launcher.allocateBytes(deviceCStorage.size() * sizeof(float));
  launcher.copyHostToDevice(aStorage.data(), deviceA, aStorage.size() * sizeof(float));
  launcher.copyHostToDevice(bStorage.data(), deviceB, bStorage.size() * sizeof(float));
  launcher.copyHostToDevice(deviceCStorage.data(), deviceC, deviceCStorage.size() * sizeof(float));
  GemmArgs args{'N', 'N', m, n, k, alpha, reinterpret_cast<float*>(deviceA), lda, reinterpret_cast<float*>(deviceB), ldb,
                beta, reinterpret_cast<float*>(deviceC), ldc};
  launcher.kernelLaunch(kernelId, &args);
  launcher.waitKernelCompletion(std::chrono::seconds(opt.kernel_launch_timeout));
  launcher.copyDeviceToHost(deviceC, deviceCStorage.data(), deviceCStorage.size() * sizeof(float));
  launcher.waitKernelCompletion(std::chrono::seconds(opt.kernel_launch_timeout));
  const bool ok = !launcher.checkKernelExecutionErrors() &&
                  nearlyEqual(denseFromColumnMajor(hostCStorage, m, n, ldc), denseFromColumnMajor(deviceCStorage, m, n, ldc), opt.epsilon);
  launcher.freeBytes(deviceA);
  launcher.freeBytes(deviceB);
  launcher.freeBytes(deviceC);
  launcher.unLoadKernel(kernelId);

  VerificationPair result;
  result.host.name = "gemm_nn";
  result.host.params =
    "transa=N, transb=N, m=" + std::to_string(m) + ", n=" + std::to_string(n) + ", k=" + std::to_string(k);
  result.host.ok = ok;
  result.host.matrices.push_back({"input_a", m, k, denseFromColumnMajor(aStorage, m, k, lda)});
  result.host.matrices.push_back({"input_b", k, n, denseFromColumnMajor(bStorage, k, n, ldb)});
  result.host.matrices.push_back({"output_c", m, n, denseFromColumnMajor(hostCStorage, m, n, ldc)});
  result.device = result.host;
  result.device.matrices.back().values = denseFromColumnMajor(deviceCStorage, m, n, ldc);
  return result;
}

VerificationPair verifySymmCase(ReferenceVerifierLauncher& launcher, const Options& opt) {
  const int m = opt.problem_dim;
  const int n = opt.problem_dim;
  const int lda = m + 1;
  const int ldb = m + 1;
  const int ldc = m + 1;
  const float alpha = -0.75f;
  const float beta = 0.5f;
  auto aStorage = makeSymmetricStorage(m, lda, 'U');
  auto bStorage = makeDenseStorage(m, n, ldb, 0.25f);
  auto hostCStorage = makeDenseStorage(m, n, ldc, -1.0f);
  auto deviceCStorage = hostCStorage;
  hostSymm('L', 'U', m, n, alpha, aStorage, lda, bStorage, ldb, beta, hostCStorage, ldc);

  auto kernelId =
    launcher.loadKernel(resolveKernelArtifact(opt.kernel_root / "symm", "blas_symm_reference_fp32").string());
  auto deviceA = launcher.allocateBytes(aStorage.size() * sizeof(float));
  auto deviceB = launcher.allocateBytes(bStorage.size() * sizeof(float));
  auto deviceC = launcher.allocateBytes(deviceCStorage.size() * sizeof(float));
  launcher.copyHostToDevice(aStorage.data(), deviceA, aStorage.size() * sizeof(float));
  launcher.copyHostToDevice(bStorage.data(), deviceB, bStorage.size() * sizeof(float));
  launcher.copyHostToDevice(deviceCStorage.data(), deviceC, deviceCStorage.size() * sizeof(float));
  SymmArgs args{'L', 'U', m, n, alpha, reinterpret_cast<float*>(deviceA), lda, reinterpret_cast<float*>(deviceB), ldb,
                beta, reinterpret_cast<float*>(deviceC), ldc};
  launcher.kernelLaunch(kernelId, &args);
  launcher.waitKernelCompletion(std::chrono::seconds(opt.kernel_launch_timeout));
  launcher.copyDeviceToHost(deviceC, deviceCStorage.data(), deviceCStorage.size() * sizeof(float));
  launcher.waitKernelCompletion(std::chrono::seconds(opt.kernel_launch_timeout));
  const bool ok = !launcher.checkKernelExecutionErrors() &&
                  nearlyEqual(denseFromColumnMajor(hostCStorage, m, n, ldc), denseFromColumnMajor(deviceCStorage, m, n, ldc), opt.epsilon);
  launcher.freeBytes(deviceA);
  launcher.freeBytes(deviceB);
  launcher.freeBytes(deviceC);
  launcher.unLoadKernel(kernelId);

  VerificationPair result;
  result.host.name = "symm_lu";
  result.host.params = "side=L, uplo=U, m=" + std::to_string(m) + ", n=" + std::to_string(n);
  result.host.ok = ok;
  result.host.matrices.push_back({"input_a", m, m, denseFromSymmetricStorage(aStorage, m, lda, 'U')});
  result.host.matrices.push_back({"input_b", m, n, denseFromColumnMajor(bStorage, m, n, ldb)});
  result.host.matrices.push_back({"output_c", m, n, denseFromColumnMajor(hostCStorage, m, n, ldc)});
  result.device = result.host;
  result.device.matrices.back().values = denseFromColumnMajor(deviceCStorage, m, n, ldc);
  return result;
}

VerificationPair verifySyrkCase(ReferenceVerifierLauncher& launcher, const Options& opt) {
  const int n = opt.problem_dim;
  const int k = opt.problem_dim;
  const int lda = n + 1;
  const int ldc = n + 1;
  const float alpha = 1.0f;
  const float beta = -0.5f;
  auto aStorage = makeDenseStorage(n, k, lda, 0.75f);
  auto hostCStorage = makeSymmetricStorage(n, ldc, 'U');
  auto deviceCStorage = hostCStorage;
  hostSyrk('U', 'N', n, k, alpha, aStorage, lda, beta, hostCStorage, ldc);

  auto kernelId =
    launcher.loadKernel(resolveKernelArtifact(opt.kernel_root / "syrk", "blas_syrk_reference_fp32").string());
  auto deviceA = launcher.allocateBytes(aStorage.size() * sizeof(float));
  auto deviceC = launcher.allocateBytes(deviceCStorage.size() * sizeof(float));
  launcher.copyHostToDevice(aStorage.data(), deviceA, aStorage.size() * sizeof(float));
  launcher.copyHostToDevice(deviceCStorage.data(), deviceC, deviceCStorage.size() * sizeof(float));
  SyrkArgs args{'U', 'N', n, k, alpha, reinterpret_cast<float*>(deviceA), lda, beta, reinterpret_cast<float*>(deviceC), ldc};
  launcher.kernelLaunch(kernelId, &args);
  launcher.waitKernelCompletion(std::chrono::seconds(opt.kernel_launch_timeout));
  launcher.copyDeviceToHost(deviceC, deviceCStorage.data(), deviceCStorage.size() * sizeof(float));
  launcher.waitKernelCompletion(std::chrono::seconds(opt.kernel_launch_timeout));
  const bool ok = !launcher.checkKernelExecutionErrors() &&
                  nearlyEqual(denseFromSymmetricStorage(hostCStorage, n, ldc, 'U'),
                              denseFromSymmetricStorage(deviceCStorage, n, ldc, 'U'), opt.epsilon);
  launcher.freeBytes(deviceA);
  launcher.freeBytes(deviceC);
  launcher.unLoadKernel(kernelId);

  VerificationPair result;
  result.host.name = "syrk_un";
  result.host.params = "uplo=U, trans=N, n=" + std::to_string(n) + ", k=" + std::to_string(k);
  result.host.ok = ok;
  result.host.matrices.push_back({"input_a", n, k, denseFromColumnMajor(aStorage, n, k, lda)});
  result.host.matrices.push_back({"output_c", n, n, denseFromSymmetricStorage(hostCStorage, n, ldc, 'U')});
  result.device = result.host;
  result.device.matrices.back().values = denseFromSymmetricStorage(deviceCStorage, n, ldc, 'U');
  return result;
}

VerificationPair verifySyr2kCase(ReferenceVerifierLauncher& launcher, const Options& opt) {
  const int n = opt.problem_dim;
  const int k = opt.problem_dim;
  const int lda = k + 1;
  const int ldb = k + 1;
  const int ldc = n + 1;
  const float alpha = 0.625f;
  const float beta = 0.25f;
  auto aStorage = makeDenseStorage(k, n, lda, 0.5f);
  auto bStorage = makeDenseStorage(k, n, ldb, 1.0f);
  auto hostCStorage = makeSymmetricStorage(n, ldc, 'L');
  auto deviceCStorage = hostCStorage;
  hostSyr2k('L', 'T', n, k, alpha, aStorage, lda, bStorage, ldb, beta, hostCStorage, ldc);

  auto kernelId =
    launcher.loadKernel(resolveKernelArtifact(opt.kernel_root / "syr2k", "blas_syr2k_reference_fp32").string());
  auto deviceA = launcher.allocateBytes(aStorage.size() * sizeof(float));
  auto deviceB = launcher.allocateBytes(bStorage.size() * sizeof(float));
  auto deviceC = launcher.allocateBytes(deviceCStorage.size() * sizeof(float));
  launcher.copyHostToDevice(aStorage.data(), deviceA, aStorage.size() * sizeof(float));
  launcher.copyHostToDevice(bStorage.data(), deviceB, bStorage.size() * sizeof(float));
  launcher.copyHostToDevice(deviceCStorage.data(), deviceC, deviceCStorage.size() * sizeof(float));
  Syr2kArgs args{'L', 'T', n, k, alpha, reinterpret_cast<float*>(deviceA), lda, reinterpret_cast<float*>(deviceB), ldb,
                 beta, reinterpret_cast<float*>(deviceC), ldc};
  launcher.kernelLaunch(kernelId, &args);
  launcher.waitKernelCompletion(std::chrono::seconds(opt.kernel_launch_timeout));
  launcher.copyDeviceToHost(deviceC, deviceCStorage.data(), deviceCStorage.size() * sizeof(float));
  launcher.waitKernelCompletion(std::chrono::seconds(opt.kernel_launch_timeout));
  const bool ok = !launcher.checkKernelExecutionErrors() &&
                  nearlyEqual(denseFromSymmetricStorage(hostCStorage, n, ldc, 'L'),
                              denseFromSymmetricStorage(deviceCStorage, n, ldc, 'L'), opt.epsilon);
  launcher.freeBytes(deviceA);
  launcher.freeBytes(deviceB);
  launcher.freeBytes(deviceC);
  launcher.unLoadKernel(kernelId);

  VerificationPair result;
  result.host.name = "syr2k_lt";
  result.host.params = "uplo=L, trans=T, n=" + std::to_string(n) + ", k=" + std::to_string(k);
  result.host.ok = ok;
  result.host.matrices.push_back({"input_a", k, n, denseFromColumnMajor(aStorage, k, n, lda)});
  result.host.matrices.push_back({"input_b", k, n, denseFromColumnMajor(bStorage, k, n, ldb)});
  result.host.matrices.push_back({"output_c", n, n, denseFromSymmetricStorage(hostCStorage, n, ldc, 'L')});
  result.device = result.host;
  result.device.matrices.back().values = denseFromSymmetricStorage(deviceCStorage, n, ldc, 'L');
  return result;
}

VerificationPair verifyTrmmCase(ReferenceVerifierLauncher& launcher, const Options& opt) {
  const int m = opt.problem_dim;
  const int n = opt.problem_dim;
  const int lda = m + 1;
  const int ldb = m + 1;
  const float alpha = 0.75f;
  auto aStorage = makeTriangularStorage(m, lda, 'U', 'N');
  auto hostBStorage = makeDenseStorage(m, n, ldb, -0.25f);
  auto deviceBStorage = hostBStorage;
  hostTrmm('L', 'U', 'N', 'N', m, n, alpha, aStorage, lda, hostBStorage, ldb);

  auto kernelId =
    launcher.loadKernel(resolveKernelArtifact(opt.kernel_root / "trmm", "blas_trmm_reference_fp32").string());
  auto deviceA = launcher.allocateBytes(aStorage.size() * sizeof(float));
  auto deviceB = launcher.allocateBytes(deviceBStorage.size() * sizeof(float));
  launcher.copyHostToDevice(aStorage.data(), deviceA, aStorage.size() * sizeof(float));
  launcher.copyHostToDevice(deviceBStorage.data(), deviceB, deviceBStorage.size() * sizeof(float));
  TrmmArgs args{'L', 'U', 'N', 'N', m, n, alpha, reinterpret_cast<float*>(deviceA), lda, reinterpret_cast<float*>(deviceB), ldb};
  launcher.kernelLaunch(kernelId, &args);
  launcher.waitKernelCompletion(std::chrono::seconds(opt.kernel_launch_timeout));
  launcher.copyDeviceToHost(deviceB, deviceBStorage.data(), deviceBStorage.size() * sizeof(float));
  launcher.waitKernelCompletion(std::chrono::seconds(opt.kernel_launch_timeout));
  const bool ok = !launcher.checkKernelExecutionErrors() &&
                  nearlyEqual(denseFromColumnMajor(hostBStorage, m, n, ldb), denseFromColumnMajor(deviceBStorage, m, n, ldb), opt.epsilon);
  launcher.freeBytes(deviceA);
  launcher.freeBytes(deviceB);
  launcher.unLoadKernel(kernelId);

  VerificationPair result;
  result.host.name = "trmm_lunn";
  result.host.params =
    "side=L, uplo=U, transa=N, diag=N, m=" + std::to_string(m) + ", n=" + std::to_string(n);
  result.host.ok = ok;
  result.host.matrices.push_back({"input_a", m, m, denseFromColumnMajor(aStorage, m, m, lda)});
  result.host.matrices.push_back({"output_b", m, n, denseFromColumnMajor(hostBStorage, m, n, ldb)});
  result.device = result.host;
  result.device.matrices.back().values = denseFromColumnMajor(deviceBStorage, m, n, ldb);
  return result;
}

VerificationPair verifyTrsmCase(ReferenceVerifierLauncher& launcher, const Options& opt) {
  const int m = opt.problem_dim;
  const int n = opt.problem_dim;
  const int lda = m + 1;
  const int ldb = m + 1;
  const float alpha = 1.0f;
  auto aStorage = makeTriangularStorage(m, lda, 'U', 'N');
  auto hostBStorage = makeDenseStorage(m, n, ldb, 0.5f);
  auto deviceBStorage = hostBStorage;
  hostTrsm('L', 'U', 'N', 'N', m, n, alpha, aStorage, lda, hostBStorage, ldb);

  auto kernelId =
    launcher.loadKernel(resolveKernelArtifact(opt.kernel_root / "trsm", "blas_trsm_reference_fp32").string());
  auto deviceA = launcher.allocateBytes(aStorage.size() * sizeof(float));
  auto deviceB = launcher.allocateBytes(deviceBStorage.size() * sizeof(float));
  launcher.copyHostToDevice(aStorage.data(), deviceA, aStorage.size() * sizeof(float));
  launcher.copyHostToDevice(deviceBStorage.data(), deviceB, deviceBStorage.size() * sizeof(float));
  TrsmArgs args{'L', 'U', 'N', 'N', m, n, alpha, reinterpret_cast<float*>(deviceA), lda, reinterpret_cast<float*>(deviceB), ldb};
  launcher.kernelLaunch(kernelId, &args);
  launcher.waitKernelCompletion(std::chrono::seconds(opt.kernel_launch_timeout));
  launcher.copyDeviceToHost(deviceB, deviceBStorage.data(), deviceBStorage.size() * sizeof(float));
  launcher.waitKernelCompletion(std::chrono::seconds(opt.kernel_launch_timeout));
  const bool ok = !launcher.checkKernelExecutionErrors() &&
                  nearlyEqual(denseFromColumnMajor(hostBStorage, m, n, ldb), denseFromColumnMajor(deviceBStorage, m, n, ldb), opt.epsilon);
  launcher.freeBytes(deviceA);
  launcher.freeBytes(deviceB);
  launcher.unLoadKernel(kernelId);

  VerificationPair result;
  result.host.name = "trsm_lunn";
  result.host.params =
    "side=L, uplo=U, transa=N, diag=N, m=" + std::to_string(m) + ", n=" + std::to_string(n);
  result.host.ok = ok;
  result.host.matrices.push_back({"input_a", m, m, denseFromColumnMajor(aStorage, m, m, lda)});
  result.host.matrices.push_back({"output_b", m, n, denseFromColumnMajor(hostBStorage, m, n, ldb)});
  result.device = result.host;
  result.device.matrices.back().values = denseFromColumnMajor(deviceBStorage, m, n, ldb);
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

  if (opt.selected_case == "all" || opt.selected_case == "gemm_nn") {
    appendCase(verifyGemmCase(launcher, opt));
  }
  if (opt.selected_case == "all" || opt.selected_case == "symm_lu") {
    appendCase(verifySymmCase(launcher, opt));
  }
  if (opt.selected_case == "all" || opt.selected_case == "syrk_un") {
    appendCase(verifySyrkCase(launcher, opt));
  }
  if (opt.selected_case == "all" || opt.selected_case == "syr2k_lt") {
    appendCase(verifySyr2kCase(launcher, opt));
  }
  if (opt.selected_case == "all" || opt.selected_case == "trmm_lunn") {
    appendCase(verifyTrmmCase(launcher, opt));
  }
  if (opt.selected_case == "all" || opt.selected_case == "trsm_lunn") {
    appendCase(verifyTrsmCase(launcher, opt));
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
    std::cerr << "BLAS Level 3 reference verification failed" << std::endl;
    return 1;
  }

  std::cout << "BLAS Level 3 reference verification passed" << std::endl;
  return 0;
}
