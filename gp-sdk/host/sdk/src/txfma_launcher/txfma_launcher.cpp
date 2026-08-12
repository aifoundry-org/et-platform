//******************************************************************************
// Copyright (c) 2025 Ainekko, Co.
// SPDX-License-Identifier: Apache-2.0
//------------------------------------------------------------------------------

#include <numeric>
#include <cstdlib>
#include <getopt.h>
#include <string>

#include "GenericLauncher.h"
#include "txfma_kernel_arguments.h"

/* Place here all parameters accepted for this specific launcher. */
struct Options {

  fs::path kernel_path = "";
  int kernel_launch_timeout = 10;
  int num_launches = 1;
  std::string device_type = "sysemu";
  uint32_t shire_mask = 0xFFFFFFFF;
  double epsilon = 0.0;
};

Options parse_args(int argc, char* const* argv, std::vector<char*>& nextlevel) {

  std::string launcherName = argv[0];
  static constexpr const char* help_msg =
    "Usage: [options] <trace>\n\n"
    "Launcher GP-SDK kernel.\n\n"
    "The following switches must be given:\n"
    "  -k, --kernel_path             path to kernel elf file to execute.\n\n"
    "The following switches are optional:\n"
    "  -t, --kernel_launch_timeout   timeout (in seconds) to wait for kenelLaunch\n"
    "  -n, --num_launches            Number of times the kernel will be launched.\n"
    "  -d, --device_type             Device Type to be used (sysemu, fake,silicon.\n"
    "  -m, --shire_mask              Shires the kernel will be assigned when executed.\n"
    "  -e, --epsilon                 Delta used for comparison between host and device.\n";

  static constexpr const char* short_opts = "k:t:n:d:m:e:h";

  static const std::vector<struct option> long_opts_vect{{"kernel_path", required_argument, nullptr, 'k'},
                                                         {"kernel_launch_timeout", required_argument, nullptr, 't'},
                                                         {"num_launches", required_argument, nullptr, 'n'},
                                                         {"device_type", required_argument, nullptr, 'd'},
                                                         {"shire_mask", required_argument, nullptr, 'm'},
                                                         {"epsilon", required_argument, nullptr, 'e'},
                                                         {"help", no_argument, nullptr, 'h'},
                                                         {nullptr, 0, nullptr, 0}};

  Options opts;

  int ret = 0;
  int index = 0;
  opterr = 0;

  while ((ret = getopt_long(argc, argv, short_opts, long_opts_vect.data(), &index)) != -1) {
    switch (ret) {
    case 'k':
      opts.kernel_path = optarg;
      break;
    case 't':
      opts.kernel_launch_timeout = atoi(optarg);
      break;
    case 'n':
      opts.num_launches = atoi(optarg);
      break;
    case 'd':
      opts.device_type = optarg;
      break;
    case 'm':
      opts.shire_mask = std::stoul(optarg, 0, 16);
      break;
    case 'e':
      opts.epsilon = atof(optarg);
      break;
    case 'h':
      std::cout << help_msg << GenericLauncher::help_msg << std::endl;
      exit(0);
    case '?':
      nextlevel.emplace_back(argv[optind - 1]);
      break;
    default:
      std::cout << "Error: Unknown option " << argv[optind - 1] << ". See " << argv[0] << " --help'.\n" << std::endl;
      exit(1);
    }
  }

  return opts;
}

void txfma(float* C, const float* A, const float* B, size_t M, size_t K, size_t N) {
  std::fill_n(C, M * N, 0.f);
  for (auto k = 0; k < K; ++k) {
    for (auto m = 0; m < M; ++m) {
      for (auto n = 0; n < N; ++n) {
        C[m * N + n] += A[m * K + k] * B[k * N + n];
      }
    }
  }
}

// Kernel launcher for TxFMA
class TxFma : public GenericLauncher {
public:
  TxFma() = delete;
  using GenericLauncher::GenericLauncher;

  void prepareInput() {
    std::iota(A_.begin(), A_.end(), 0);
    std::iota(B_.begin(), B_.end(), 100);
  }

  void performDeviceAllocs() {
    deviceA_ = runtime_->mallocDevice(devices_[devIdx_], A_.size() * sizeof(float));
    deviceB_ = runtime_->mallocDevice(devices_[devIdx_], B_.size() * sizeof(float));
    deviceC_ = runtime_->mallocDevice(devices_[devIdx_], C_.size() * sizeof(float));
  }

  void programHost2DevCopies() {
    runtime_->memcpyHostToDevice(defaultStreams_[devIdx_], (std::byte*)A_.data(), deviceA_, A_.size() * sizeof(float));
    runtime_->memcpyHostToDevice(defaultStreams_[devIdx_], (std::byte*)B_.data(), deviceB_, B_.size() * sizeof(float));
  }

  void programDev2HostCopies() {
    runtime_->memcpyDeviceToHost(defaultStreams_[devIdx_], deviceC_, (std::byte*)C_.data(), C_.size() * sizeof(float));
  }

  void freeDeviceAllocs() {
    runtime_->freeDevice(devices_[devIdx_], deviceA_);
    runtime_->freeDevice(devices_[devIdx_], deviceB_);
    runtime_->freeDevice(devices_[devIdx_], deviceC_);
  }

  static constexpr uint32_t aRows = 14;
  static constexpr uint32_t aCols = 16;

  static constexpr uint32_t bRows = 16;
  static constexpr uint32_t bCols = 16;

  static constexpr uint32_t cRows = 14;
  static constexpr uint32_t cCols = 16;

  std::vector<float> A_ = std::vector<float>(aRows * aCols);
  std::vector<float> B_ = std::vector<float>(bRows * bCols);
  std::vector<float> C_ = std::vector<float>(cRows * cCols);
  std::byte* deviceA_;
  std::byte* deviceB_;
  std::byte* deviceC_;
};

int main(int argc, char** argv) {

  static constexpr int32_t numThreads = 1024;

  std::vector<char*> argvPendingToParse{argv[0]};

  Options opt = parse_args(argc, argv, argvPendingToParse);
  Config config{modeFromString(opt.device_type), 1};
  config.dump();

  TxFma launcher(config, static_cast<int>(argvPendingToParse.size()), argvPendingToParse.data());
  launcher.initialize();
  auto kernelId = launcher.loadKernel(opt.kernel_path);
  launcher.performDeviceAllocs();
  launcher.prepareInput();

  launcher.programHost2DevCopies();

  // prep kernel args object
  Matrix A{launcher.aRows, launcher.aCols, (float*)launcher.deviceA_};
  Matrix B{launcher.bRows, launcher.bCols, (float*)launcher.deviceB_};
  Matrix C{launcher.cRows, launcher.cCols, (float*)launcher.deviceC_};

  KernelArguments kernelArgs;
  kernelArgs.A = A;
  kernelArgs.B = B;
  kernelArgs.C = C;

  launcher.kernelLaunch(kernelId, &kernelArgs, nullptr, 0, 0, opt.shire_mask);

  launcher.programDev2HostCopies();
  auto timeout = std::chrono::seconds(opt.kernel_launch_timeout);
  launcher.waitKernelCompletion(timeout);
  launcher.dumpTracesToFile();

  if(launcher.checkKernelExecutionErrors()) {
    return -1;
  }

  launcher.freeDeviceAllocs();
  launcher.unLoadKernel(kernelId);
  launcher.tearDown();

  // Check results
  std::vector<float> c2 = launcher.C_;
  txfma(c2.data(), launcher.A_.data(), launcher.B_.data(), launcher.aRows, launcher.aCols, launcher.bCols);
  if (!std::equal(c2.begin(), c2.end(), launcher.C_.begin(),
                  [=](float host, float dev) { return std::abs(host - dev) <= opt.epsilon; })) {
    std::cerr << "error: TXFMA host/device results do not match" << std::endl;
    return 1;
  }

  return 0;
}
