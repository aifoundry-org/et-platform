//******************************************************************************
// Copyright (c) 2025 Ainekko, Co.
// SPDX-License-Identifier: Apache-2.0
//------------------------------------------------------------------------------

#include <iostream>
#include <numeric>
#include <cstdlib>
#include <getopt.h>
#include <string>
#include <cstdint>

#include "saxpy_kernel_arguments.h"
#include "GenericLauncher.h"

/* Place here all parameters accepted for this specific launcher. */
struct Options {

  fs::path kernel_path = "";
  int kernel_launch_timeout = 10;
  int num_launches = 1;
  std::string device_type = "sysemu";
  int launch_mult = 1;
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
    "  -l, --launch_mult             Number of times the kernel is executed for each launch.\n"
    "  -e, --epsilon                 Delta used for comparison between host and device.\n";

  static constexpr const char* short_opts = "k:t:n:d:l:e:h";

  static const std::vector<struct option> long_opts_vect{{"kernel_path", required_argument, nullptr, 'k'},
                                                         {"kernel_launch_timeout", required_argument, nullptr, 't'},
                                                         {"num_launches", required_argument, nullptr, 'n'},
                                                         {"device_type", required_argument, nullptr, 'd'},
                                                         {"launch_mult", required_argument, nullptr, 'l'},
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
    case 'l':
      opts.launch_mult = atoi(optarg);
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

// Host-side implementation of a SAXPY kernel (for checking purposes)
template <class InputIt, class OutputIt, class VT = typename InputIt::value_type>
OutputIt saxpy(InputIt first, InputIt last, OutputIt d_first, VT a) {
  for (; first != last; ++first) {
    *d_first = a * *first + *d_first;
    ++d_first;
  }
  return d_first;
}

// Specific kernel launcher class.
class Saxpy : public GenericLauncher {
public:
  Saxpy() = delete;
  using GenericLauncher::GenericLauncher;

  void prepareInput() {
    a_ = 3;
    std::iota(x_.begin(), x_.end(), 0);
    std::iota(y_.begin(), y_.end(), 100);
  }

  void performDeviceAllocs() {
    deviceX_ = runtime_->mallocDevice(devices_[devIdx_], x_.size() * sizeof(float));
    deviceY_ = runtime_->mallocDevice(devices_[devIdx_], y_.size() * sizeof(float));
  }

  void programHost2DevCopies() {
    runtime_->memcpyHostToDevice(defaultStreams_[devIdx_], (std::byte*)x_.data(), deviceX_, x_.size() * sizeof(float));
    runtime_->memcpyHostToDevice(defaultStreams_[devIdx_], (std::byte*)y_.data(), deviceY_, y_.size() * sizeof(float));
  }

  void programDev2HostCopies() {
    runtime_->memcpyDeviceToHost(defaultStreams_[devIdx_], deviceY_, (std::byte*)y_.data(), y_.size() * sizeof(float));
  }

  void freeDeviceAllocs() {
    runtime_->freeDevice(devices_[devIdx_], deviceX_);
    runtime_->freeDevice(devices_[devIdx_], deviceY_);
  }

  static constexpr size_t numElems_ = 256;
  float a_;
  std::vector<float> x_ = std::vector<float>(numElems_);
  std::vector<float> y_ = std::vector<float>(numElems_);
  std::byte* deviceX_;
  std::byte* deviceY_;
};

int main(int argc, char** argv) {

  std::vector<char*> argvPendingToParse{argv[0]};

  Options opt = parse_args(argc, argv, argvPendingToParse);

  Config config{modeFromString(opt.device_type), 1};
  config.dump();

  Saxpy launcher(config, static_cast<int>(argvPendingToParse.size()), argvPendingToParse.data());
  launcher.initialize();
  auto kernelId = launcher.loadKernel(opt.kernel_path);
  launcher.performDeviceAllocs();
  launcher.prepareInput();

  // Copy original values to check them later
  std::vector<float> x2 = launcher.x_;
  std::vector<float> y2 = launcher.y_;

  for (size_t i = 0; i < opt.num_launches; i++) {
    launcher.programHost2DevCopies();

    KernelArguments kernelArgs;
    kernelArgs.numElements = launcher.x_.size();
    kernelArgs.x = (float*)launcher.deviceX_;
    kernelArgs.y = (float*)launcher.deviceY_;
    kernelArgs.a = launcher.a_;

    launcher.kernelLaunch(kernelId, &kernelArgs);
    launcher.programDev2HostCopies();
    auto timeout = std::chrono::seconds(opt.kernel_launch_timeout);
    launcher.waitKernelCompletion(timeout);
    launcher.dumpTracesToFile(i);

    if(launcher.checkKernelExecutionErrors()) {
      return -1;
    }
  }

  launcher.freeDeviceAllocs();
  launcher.unLoadKernel(kernelId);
  launcher.tearDown();

  // Check kernel results
  for (size_t i = 0; i < opt.num_launches * opt.launch_mult; ++i) {
    saxpy(x2.begin(), x2.end(), y2.begin(), launcher.a_);
  }
  if (!std::equal(y2.begin(), y2.end(), launcher.y_.begin(),
                  [=](float host, float dev) { return std::abs(host - dev) <= opt.epsilon; })) {
    std::cerr << "error: SAXPY host/device results do not match" << std::endl;
    return 1;
  }

  return 0;
}
