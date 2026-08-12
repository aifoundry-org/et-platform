//******************************************************************************
// Copyright (c) 2025 Ainekko, Co.
// SPDX-License-Identifier: Apache-2.0
//------------------------------------------------------------------------------

#include <iostream>
#include <numeric>
#include <cstdlib>
#include <getopt.h>
#include <string>

#include "matmul_args.h"
#include "GenericLauncher.h"

static constexpr size_t i_size = 1024;
static constexpr size_t j_size = 512;
static constexpr size_t k_size = 512;
static constexpr size_t ph_offset = 0x202000 / sizeof(float);

/* Place here all parameters accepted for this specific launcher. */
struct Options {

  fs::path kernel_path = "";
  int kernel_launch_timeout = 30;
  int num_launches = 1;
  std::string device_type = "sysemu";
  uint64_t shire_mask = 0xFFFFFFFF;
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
    "  -m, --shire_mask              Shires the kernel will be assigned when executed.\n";

  static constexpr const char* short_opts = "k:t:n:d:m:h";

  static const std::vector<struct option> long_opts_vect{{"kernel_path", required_argument, nullptr, 'k'},
                                                         {"kernel_launch_timeout", required_argument, nullptr, 't'},
                                                         {"num_launches", required_argument, nullptr, 'n'},
                                                         {"device_type", required_argument, nullptr, 'd'},
                                                         {"shire_mask", required_argument, nullptr, 'm'},
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
      opts.shire_mask = std::stoull(optarg, 0, 16);
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
// template <class InputIt, class OutputIt, class VT = typename InputIt::value_type>
// OutputIt saxpy(InputIt first, InputIt last, OutputIt d_first, VT a) {
//   for (; first != last; ++first) {
//     *d_first = a * *first + *d_first;
//     ++d_first;
//   }
//   return d_first;
// }

// Specific kernel launcher class.
class MatmulLauncher : public GenericLauncher {
public:
  MatmulLauncher() = delete;
  using GenericLauncher::GenericLauncher;

  void prepareInput() {
    // std::iota(B_.begin(), B_.end(), 0);
    // for (auto ptr = PH_.begin(); ptr < PH_.begin() + (numElems_); ptr++) {
    //     *ptr = 1.0f;
    // }
  }

  void performDeviceAllocs() {
    deviceB_ = runtime_->mallocDevice(devices_[devIdx_], B_.size() * sizeof(float));
    devicePH_ = runtime_->mallocDevice(devices_[devIdx_], PH_.size() * sizeof(float));
  }

  void programHost2DevCopies() {
    runtime_->memcpyHostToDevice(defaultStreams_[devIdx_], (std::byte*)B_.data(), deviceB_, B_.size() * sizeof(float));
    runtime_->memcpyHostToDevice(defaultStreams_[devIdx_], (std::byte*)PH_.data(), devicePH_, PH_.size() * sizeof(float));
  }

  void programDev2HostCopies() {
    runtime_->memcpyDeviceToHost(defaultStreams_[devIdx_], devicePH_, (std::byte*)PH_.data(), PH_.size() * sizeof(float));
  }

  void freeDeviceAllocs() {
    runtime_->freeDevice(devices_[devIdx_], deviceB_);
    runtime_->freeDevice(devices_[devIdx_], devicePH_);
  }

  static constexpr size_t padding = 1024*1024;
  std::vector<float> B_ = std::vector<float>((k_size * j_size) + padding, 1.0f);
  std::vector<float> PH_ = std::vector<float>((i_size * k_size + ph_offset) * 2 , 1.0f); // While we dont know how big the matrix is, multiply by another 4

  std::byte* deviceB_;
  std::byte* devicePH_;
};

int main(int argc, char** argv) {
  std::vector<char*> argvPendingToParse{argv[0]};
  Options opt = parse_args(argc, argv, argvPendingToParse);

  Config config{modeFromString(opt.device_type), 1};
  config.dump();

  MatmulLauncher launcher(config, static_cast<int>(argvPendingToParse.size()), argvPendingToParse.data());
  launcher.initialize();
  auto kernelId = launcher.loadKernel(opt.kernel_path);
  launcher.performDeviceAllocs();
  launcher.prepareInput();

  // Copy original values to check them later
  std::vector<float> B2 = launcher.B_;
  std::vector<float> PH2 = launcher.PH_;

  for (size_t i = 0; i < opt.num_launches; i++) {
    launcher.programHost2DevCopies();

    kernelArguments kernelArgs;
    kernelArgs.B = (void *)launcher.deviceB_;
    kernelArgs.PH = (void*)launcher.devicePH_;

    launcher.kernelLaunch(kernelId, &kernelArgs, /*stack address*/ nullptr, /*stack size*/ 0, /*device Id*/ 0,
                          /*shire mask*/ opt.shire_mask);
    launcher.programDev2HostCopies();
    auto timeout = std::chrono::seconds(opt.kernel_launch_timeout);
    launcher.waitKernelCompletion(timeout);
    launcher.dumpTracesToFile(i);

    if (launcher.checkKernelExecutionErrors()) {
      return -1;
    }
  }

  launcher.freeDeviceAllocs();
  launcher.unLoadKernel(kernelId);
  launcher.tearDown();

  // Check kernel results

  float* C =  (float *) &PH2[ph_offset];
  for (int i = 0; i < i_size; i++) {
    for (int j = 0; j < j_size; j++) {
      for (int k = 0; k < k_size; k++) {
        C[i * j_size + j] += PH2[i * k_size + k] * B2[k * j_size + j];
       
      }
      if (i < 2 && j < 2)
        std::cout << C[i * j_size + j] << " host - device " << launcher.PH_[ph_offset + (i * j_size + j)] << std::endl;
    }
  }

  size_t num1 = 0;
  size_t num0 = 0;
  size_t wth = 0;
  for (auto x = launcher.PH_.begin(); x < launcher.PH_.end(); x++){
    if (*x == 1.0f) {
      num1++;
    } else if (*x == 0.0f) {
      num0++;
    } else {
      wth++;
    }
  }
  std::cerr << "stats - ones:" << num1 << " zeros:" << num0 << " other: " << wth << std::endl;

  // if (!std::equal(PH2.begin(), PH2.end(), launcher.PH_.begin(),
  //                 [=](float host, float dev) { return std::abs(host - dev) <= 0.001f; })) {
  //   std::cerr << "error: MATMUL host/device results do not match" << std::endl;
  //   return 1;
  // }

  return 0;
}
