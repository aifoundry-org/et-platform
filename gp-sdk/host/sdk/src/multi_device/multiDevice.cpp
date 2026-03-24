//******************************************************************************
// Copyright (c) 2025 Ainekko, Co.
// SPDX-License-Identifier: Apache-2.0
//------------------------------------------------------------------------------

#include <cassert>
#include <numeric>
#include <cstdlib>
#include <getopt.h>
#include <string>
#include <cstdint>

#include "saxpy_kernel_arguments.h"
#include "GenericLauncher.h"

/* Place here all parameters accepted for this specific launcher. */
struct Options {

  fs::path kernel_path1 = "";
  fs::path kernel_path2 = "";
  int kernel_launch_timeout = 10;
  int num_launches = 1;
  std::string device_type = "sysemu";
  int nDevices = 1;
};

Options parse_args(int argc, char* const* argv, std::vector<char*>& nextlevel) {

  std::string launcherName = argv[0];
  static constexpr const char* help_msg =
    "Usage: [options] <trace>\n\n"
    "Launcher GP-SDK kernel.\n\n"
    "The following switches must be given:\n"
    "  -k, --kernel_path1             path to kernel elf file to execute.\n\n"
    "  -r, --kernel_path2             path to kernel elf file to execute.\n\n"
    "The following switches are optional:\n"
    "  -t, --kernel_launch_timeout   timeout (in seconds) to wait for kenelLaunch\n"
    "  -n, --num_launches            Number of times the kernel will be launched.\n"
    "  -d, --device_type             Device Type to be used (sysemu, fake,silicon.\n"
    "  -v, --nDevices                Number of devices to be used.\n";

  static constexpr const char* short_opts = "k:r:t:n:d:v:h";

  static const std::vector<struct option> long_opts_vect{{"kernel_path1", required_argument, nullptr, 'k'},
                                                         {"kernel_path2", required_argument, nullptr, 'r'},
                                                         {"kernel_launch_timeout", required_argument, nullptr, 't'},
                                                         {"num_launches", required_argument, nullptr, 'n'},
                                                         {"device_type", required_argument, nullptr, 'd'},
                                                         {"nDevices", required_argument, nullptr, 'v'},
                                                         {"help", no_argument, nullptr, 'h'},
                                                         {nullptr, 0, nullptr, 0}};

  Options opts;

  int ret = 0;
  int index = 0;
  opterr = 0;

  while ((ret = getopt_long(argc, argv, short_opts, long_opts_vect.data(), &index)) != -1) {
    switch (ret) {
    case 'k':
      opts.kernel_path1 = optarg;
      break;
    case 'r':
      opts.kernel_path2 = optarg;
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
    case 'v':
      opts.nDevices = atoi(optarg);
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

// Specific kernel lancuher class.
class MultiDevice : public GenericLauncher {
public:
  MultiDevice() = delete;
  using GenericLauncher::GenericLauncher;
  
  void prepareInputSapxy() {
    a_ = 3;
    std::iota(x_.begin(), x_.end(), 0);
    std::iota(y_.begin(), y_.end(), 100);
  }

  void performDeviceAllocsSaxpy(uint32_t devIdx_ = 0) {
    deviceX_ = runtime_->mallocDevice(devices_[devIdx_], x_.size() * sizeof(float));
    deviceY_ = runtime_->mallocDevice(devices_[devIdx_], y_.size() * sizeof(float));
  }

  void programHost2DevCopiesSaxpy(uint32_t devIdx_ = 0) {
    runtime_->memcpyHostToDevice(defaultStreams_[devIdx_], (std::byte*)x_.data(), deviceX_, x_.size() * sizeof(float));
    runtime_->memcpyHostToDevice(defaultStreams_[devIdx_], (std::byte*)y_.data(), deviceY_, y_.size() * sizeof(float));
  }

  void programDev2HostCopiesSaxpy(uint32_t devIdx_ = 0) {
    runtime_->memcpyDeviceToHost(defaultStreams_[devIdx_], deviceY_, (std::byte*)y_.data(), y_.size() * sizeof(float));
  }

  void freeDeviceAllocsSaxpy(uint32_t devIdx_ = 0) {
    runtime_->freeDevice(devices_[devIdx_], deviceX_);
    runtime_->freeDevice(devices_[devIdx_], deviceY_);
  }

  static constexpr size_t numElems_ = 256;
  float a_;
  std::vector<float> x_ = std::vector<float>(numElems_);
  std::vector<float> y_ = std::vector<float>(numElems_);

  std::byte* deviceX_;
  std::byte* deviceY_;
  std::vector<rt::KernelId> kernels_;
};


int main(int argc, char** argv) {

  std::vector<char*> argvPendingToParse{argv[0]};

  Options opt = parse_args(argc, argv, argvPendingToParse);
  Config config{modeFromString(opt.device_type), static_cast<size_t>(opt.nDevices)};
  config.dump();

  MultiDevice launcher(config, static_cast<int>(argvPendingToParse.size()), argvPendingToParse.data());
  launcher.initialize();

  if (modeFromString(opt.device_type) == Mode::PCIE) {
    assert(launcher.getNumDevices() >= opt.nDevices);
  }

  auto kernelId = launcher.loadKernel(opt.kernel_path1, 0);
  launcher.kernels_.emplace_back(kernelId);
  kernelId = launcher.loadKernel(opt.kernel_path2, 1);
  launcher.kernels_.emplace_back(kernelId);

  std::cout << "loadKernel --> " << opt.kernel_path1 << " with kernel_id=" << int(launcher.kernels_[0]) << std::endl;
  std::cout << "loadKernel --> " << opt.kernel_path2 << " with kernel_id=" << int(launcher.kernels_[1]) << std::endl;

  auto timeout = std::chrono::seconds(opt.kernel_launch_timeout);

  // assume saxpy as second elf to be loaded & prep data context for it.
  launcher.performDeviceAllocsSaxpy(1);
  launcher.prepareInputSapxy();
  launcher.programHost2DevCopiesSaxpy(1);
  
  KernelArguments kernelArgs;
  kernelArgs.numElements = launcher.x_.size();
  kernelArgs.x = (float*)launcher.deviceX_;
  kernelArgs.y = (float*)launcher.deviceY_;
  kernelArgs.a = launcher.a_;

  // Assuming first kernel requires no data context (e.g hello-world and the like).
  launcher.kernelLaunch(launcher.kernels_[0], (uint32_t)0);

  launcher.kernelLaunch(launcher.kernels_[1], &kernelArgs, nullptr, 0, (uint32_t)1);
  launcher.programDev2HostCopiesSaxpy(1);

  launcher.waitKernelCompletion(timeout, 0);
  launcher.dumpTracesToFile(0, launcher.kernels_[0], 0);

  launcher.waitKernelCompletion(timeout, 1);
  launcher.dumpTracesToFile(0, launcher.kernels_[1], 1);

  // kernelError_ and kernelAbort_ have to be catched asking to the specific device currently not allowed at runtime
  // side
  if(launcher.checkKernelExecutionErrors()) {
    std::cout << "Error on kernel id=" << int(launcher.kernels_[0]) << "or in kernel id=" << int(launcher.kernels_[1])
              << std::endl;
    return -1;
  }

  launcher.freeDeviceAllocsSaxpy(1);

  launcher.unLoadKernel(launcher.kernels_[0]);
  launcher.unLoadKernel(launcher.kernels_[1]);
  launcher.tearDown();

  return 0;
}
