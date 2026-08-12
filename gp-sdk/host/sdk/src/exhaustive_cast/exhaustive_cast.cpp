//******************************************************************************
// Copyright (c) 2025 Ainekko, Co.
// SPDX-License-Identifier: Apache-2.0
//------------------------------------------------------------------------------

#include <bits/stdc++.h>
#include <cstdlib>
#include <getopt.h>
#include <random>
#include <string>

#include "GenericLauncher.h"
#include "exhaustive_cast_arguments.h"

/* Place here all parameters accepted for this specific launcher. */
struct Options {

  fs::path kernel_path = "";
  int kernel_launch_timeout = 10;
  int num_launches = 1;
  std::string device_type = "sysemu";
  uint32_t shire_mask = 0xFFFFFFFF;
  CastType cast_type = int64_tToFloat;
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
    "  -d, --device_type             Device Type to be used (sysemu, fake, silicon).\n"
    "  -m, --shire_mask              Shires the kernel will be assigned when executed.\n"
    "  -c, --cast_type               Cast type to be done (1 , 2, 3, 4, 5, 6, 7, 8).\n";

  static constexpr const char* short_opts = "k:t:n:d:m:c:h";

  static const std::vector<struct option> long_opts_vect{{"kernel_path", required_argument, nullptr, 'k'},
                                                         {"kernel_launch_timeout", required_argument, nullptr, 't'},
                                                         {"num_launches", required_argument, nullptr, 'n'},
                                                         {"device_type", required_argument, nullptr, 'd'},
                                                         {"shire_mask", required_argument, nullptr, 'm'},
                                                         {"cast_type", required_argument, nullptr, 'c'},
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
    case 'c':
      opts.cast_type = static_cast<CastType>(atoi(optarg));
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

// Specific kernel launcher class.
class exhaustive_cast : public GenericLauncher {
public:
  exhaustive_cast() = delete;
  using GenericLauncher::GenericLauncher;

  void prepareInput(CastType cast_type) {
    std::minstd_rand simple_rand;
    simple_rand.seed(0);
    switch (cast_type) {
    case floatToInt64_t:               // Float to int64_t
    case floatToUint64_t:              // Float to uint64_t
    case floatToInt32_t:               // Float to int32_t
    case floatToUint32_t:              // Float to uint32_t
      devIn_.a[0] = 0.52F;
      devIn_.a[1] = 123456789.987654F; // May lose precision
      devIn_.a[2] = 123.4567F;         // Rounded Float
      devIn_.a[3] = 0.0F;              // Zero Float
      devIn_.a[4] = 0.0/0.0;           // Not a Number Float
      devIn_.a[5] = 1.0/0;             // Positive Infinity Float
      devIn_.a[6] = -1.0/0;            // Negative Infinity Float
      for (int i = 7; i < numElements; i++) {
        devIn_.a[i] = (float)numElements / ((float)simple_rand());
      }
#ifdef EXHAUSTIVE_CAST_VERIFICATION
      for (int i = 0; i < numElements; i++) {
        hostIn_.a[i] = devIn_.a[i];
      }
#endif
      break;
    case int64_tToFloat: // int64_t to float
      devIn_.b[0] = -125ll;
      devIn_.b[1] = 0x7FFFFFFFFFFFFFFFLL;
      devIn_.b[2] = 0x8000000000000000LL;
      for (int i = 3; i < numElements; i++) {
        devIn_.b[i] = (int64_t)simple_rand();
      }
#ifdef EXHAUSTIVE_CAST_VERIFICATION
      for (int i = 0; i < numElements; i++) {
        hostIn_.b[i] = devIn_.b[i];
      }
#endif
      break;
    case uint64_tToFloat: // uint64_t to float
      devIn_.c[0] = 125;
      devIn_.c[1] = 0xFFFFFFFFFFFFFFFFLLU;
      devIn_.c[2] = 0;
      for (int i = 3; i < numElements; i++) {
        devIn_.c[i] = simple_rand();
      }
#ifdef EXHAUSTIVE_CAST_VERIFICATION
      for (int i = 0; i < numElements; i++) {
        hostIn_.c[i] = devIn_.c[i];
      }
#endif
      break;
    case int32_tToFloat: // int32_t to float
      devIn_.d[0] = 125;
      devIn_.d[1] = 2147483647;
      devIn_.d[2] = -2147483648;
      for (int i = 3; i < numElements; i++) {
        devIn_.d[i] = (int32_t)simple_rand();
      }
#ifdef EXHAUSTIVE_CAST_VERIFICATION
      for (int i = 0; i < numElements; i++) {
        hostIn_.d[i] = devIn_.d[i];
      }
#endif
      break;
    case uint32_tToFloat: // uint32_t to float
      devIn_.e[0] = 125;
      devIn_.e[1] = 4294967295;
      devIn_.e[2] = 0;
      for (int i = 3; i < numElements; i++) {
        devIn_.e[i] = (uint32_t)simple_rand();
      }
#ifdef EXHAUSTIVE_CAST_VERIFICATION
      for (int i = 0; i < numElements; i++) {
        hostIn_.e[i] = devIn_.e[i];
      }
#endif
      break;
    default:
      break;
    }
  }

  void performDeviceAllocs() {
    deviceIn_ = runtime_->mallocDevice(devices_[devIdx_], numElements * sizeof(uint64_t));
    deviceOut_ = runtime_->mallocDevice(devices_[devIdx_], numElements * sizeof(uint64_t));
  }

  void programHost2DevCopies() {
    runtime_->memcpyHostToDevice(defaultStreams_[devIdx_], (std::byte*)&devIn_, deviceIn_,
                                 numElements * sizeof(uint64_t));
  }

  void programDev2HostCopies() {
    runtime_->memcpyDeviceToHost(defaultStreams_[devIdx_], deviceOut_, (std::byte*)&devOut_,
                                 numElements * sizeof(uint64_t));
  }

  void freeDeviceAllocs() {
    runtime_->freeDevice(devices_[devIdx_], deviceIn_);
    runtime_->freeDevice(devices_[devIdx_], deviceOut_);
  }
#ifdef EXHAUSTIVE_CAST_VERIFICATION
  bool cmpInt64_t(const int64_t* dev, const int64_t* host, const float* in) const {
    for (uint64_t i = 0; i < numElements; i++) {
      /* Handle NaN and inf as host (x86) generates 0x8000000000000000 when NaN or inf is casted to int64_t */
      if ((std::isnan(in[i]) || std::isinf(in[i]) && in[i] > 0) && dev[i] == 0x7FFFFFFF) {
        continue;
      }
      /* Handle -inf as host (x86) generates 0x8000000000000000 when -inf is casted to int64_t */
      if ((std::isinf(in[i]) && in[i] < 0) && dev[i] == 0xffffffff80000000) {
        continue;
      }
      if (dev[i] != host[i]) {
        return false;
      }
    }
    return true;
  }

  bool cmpUint64_t(const uint64_t* dev, const uint64_t* host, const float* in) const {
    for (uint64_t i = 0; i < numElements; i++) {
      /* Handle NaN as host (x86) generates 0x8000000000000000 when NaN is casted to uint64_t */
      if ((std::isnan(in[i])) && dev[i] == 0xffffffffffffffff) {
        continue;
      }
      /* Handle inf as host (x86) generates 0x0 when inf is casted to uint64_t */
      if ((std::isinf(in[i]) && in[i] > 0) && dev[i] == 0xffffffffffffffff) {
        continue;
      }
      /* Handle -inf as host (x86) generates 0x8000000000000000 when -inf is casted to uint64_t */
      if ((std::isinf(in[i]) && in[i] < 0) && dev[i] == 0x0) {
        continue;
      }
      if (dev[i] != host[i]) {
        return false;
      }
    }
    return true;
  }

  bool cmpInt32_t(const int32_t* dev, const int32_t* host, const float* in) const {
    for (uint64_t i = 0; i < numElements; i++) {
      /* Handle NaN and inf as host (x86) generates 0x80000000 when NaN or inf is casted to int32_t */
      if ((std::isnan(in[i]) || std::isinf(in[i]) && in[i] > 0) && dev[i] == 0x7FFFFFFF) {
        continue;
      }
      if (dev[i] != host[i]) {
        return false;
      }
    }
    return true;
  }

  bool cmpUint32_t(const uint32_t* dev, const uint32_t* host, const float* in) const {
    for (uint64_t i = 0; i < numElements; i++) {
      /* Handle NaN and inf as host (x86) generates 0x0 when NaN or inf is casted to uint32_t */
      if ((std::isnan(in[i]) || std::isinf(in[i]) && in[i] > 0) && dev[i] == 0xFFFFFFFF && host[i] == 0x0) {
        continue;
      }
      if (dev[i] != host[i]) {
        return false;
      }
    }
    return true;
  }

  bool cmpFloat(const float* dev, const float* host) const {
    for (uint64_t i = 0; i < numElements; i++) {
      if (dev[i] != host[i]) {
        return false;
      }
    }
    return true;
  }

  void int64ToFloat() {
    for (uint64_t i = 0; i < numElements; i++) {
      hostOut_.a[i] = (float)hostIn_.b[i];
    }
    return;
  }

  void uint64ToFloat() {
    for (uint64_t i = 0; i < numElements; i++) {
      hostOut_.a[i] = (float)hostIn_.c[i];
    }
    return;
  }

  void int32ToFloat() {
    for (uint64_t i = 0; i < numElements; i++) {
      hostOut_.a[i] = (float)hostIn_.d[i];
    }
    return;
  }

  void uint32ToFloat() {
    for (uint64_t i = 0; i < numElements; i++) {
      hostOut_.a[i] = (float)hostIn_.e[i];
    }
    return;
  }

  void floatToInt64() {
    for (uint64_t i = 0; i < numElements; i++) {
      hostOut_.b[i] = (int64_t)hostIn_.a[i];
    }
    return;
  }

  void floatToUint64() {
    for (uint64_t i = 0; i < numElements; i++) {
      hostOut_.c[i] = (uint64_t)hostIn_.a[i];
    }
    return;
  }

  void floatToInt32() {
    for (uint64_t i = 0; i < numElements; i++) {
      hostOut_.d[i] = (int32_t)hostIn_.a[i];
    }
    return;
  }

  void floatToUint32() {
    for (uint64_t i = 0; i < numElements; i++) {
      hostOut_.e[i] = (uint32_t)hostIn_.a[i];
    }
    return;
  }

  bool verify(CastType cast_type) {
    bool ret;
    switch (cast_type) {
    case floatToInt64_t: // Float to int64_t
      floatToInt64();
      ret = cmpInt64_t((int64_t*)&devOut_, (int64_t*)&hostOut_, (float*)&devIn_);
      break;
    case floatToUint64_t: // Float to uint64_t
      floatToUint64();
      ret = cmpUint64_t((uint64_t*)&devOut_, (uint64_t*)&hostOut_, (float*)&devIn_);
      break;
    case floatToInt32_t: // Float to int32_t
      floatToInt32();
      ret = cmpInt32_t((int32_t*)&devOut_, (int32_t*)&hostOut_, (float*)&devIn_);
      break;
    case floatToUint32_t: // Float to uint32_t
      floatToUint32();
      ret = cmpUint32_t((uint32_t*)&devOut_, (uint32_t*)&hostOut_, (float*)&devIn_);
      break;
    case int64_tToFloat: // int64_t to float
      int64ToFloat();
      ret = cmpFloat((float*)&devOut_, (float*)&hostOut_);
      break;
    case uint64_tToFloat: // uint64_t to float
      uint64ToFloat();
      ret = cmpFloat((float*)&devOut_, (float*)&hostOut_);
      break;
    case int32_tToFloat: // int32_t to float
      int32ToFloat();
      ret = cmpFloat((float*)&devOut_, (float*)&hostOut_);
      break;
    case uint32_tToFloat: // uint32_t to float
      uint32ToFloat();
      ret = cmpFloat((float*)&devOut_, (float*)&hostOut_);
      break;
    default:
      ret = false;
      break;
    }
    return ret;
  }
  dataContainer hostIn_;
  dataContainer hostOut_;
#endif

  dataContainer devIn_;
  dataContainer devOut_;
  std::byte* deviceIn_;
  std::byte* deviceOut_;
};

int main(int argc, char** argv) {
  int ret = 0;
  std::vector<char*> argvPendingToParse{argv[0]};

  Options opt = parse_args(argc, argv, argvPendingToParse);
  Config config{modeFromString(opt.device_type), 1};

  exhaustive_cast launcher(config, static_cast<int>(argvPendingToParse.size()), argvPendingToParse.data());
  launcher.initialize();
  auto kernelId = launcher.loadKernel(opt.kernel_path);
  launcher.prepareInput(opt.cast_type);
  launcher.performDeviceAllocs();

  for (size_t i = 0; i < opt.num_launches; i++) {
    launcher.programHost2DevCopies();

    KernelArguments kernelArgs;
    kernelArgs.cast_type = opt.cast_type;

    kernelArgs.in = (dataContainer*)launcher.deviceIn_;
    kernelArgs.out = (dataContainer*)launcher.deviceOut_;

    launcher.kernelLaunch(kernelId, &kernelArgs, nullptr, 0, 0, opt.shire_mask);
    launcher.programDev2HostCopies();
    auto timeout = std::chrono::seconds(opt.kernel_launch_timeout);
    launcher.waitKernelCompletion(timeout);
    launcher.dumpTracesToFile(i);

    if(launcher.checkKernelExecutionErrors()) {
      return -1;
    }
  }

#ifdef EXHAUSTIVE_CAST_VERIFICATION
  if (launcher.verify(opt.cast_type)) {
    std::cout << "Passed\n";
  } else {
    std::cerr << "error: Exhaustive Cast host/device results do not match" << std::endl;
    ret = 1;
  }
#endif

  launcher.freeDeviceAllocs();
  launcher.unLoadKernel(kernelId);
  launcher.tearDown();

  return ret;
}
