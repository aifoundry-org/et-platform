//******************************************************************************
// Copyright (c) 2026 AIFoundry
// SPDX-License-Identifier: Apache-2.0
//------------------------------------------------------------------------------

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <getopt.h>
#include <iostream>
#include <string>
#include <vector>

#include "GenericLauncher.h"
#include "tima_unsigned_bits_kernel_arguments.h"

struct Options {
  fs::path kernel_path = "";
  int kernel_launch_timeout = 10;
  std::string device_type = "sysemu";
  uint64_t shire_mask = 0xffffffffULL;
  uint32_t probe_mode = TIMA_UNSIGNED_BITS_PROBE_THREAD0_ALL;
  uint32_t selected_relative_thread = 1;
  bool show_records = false;
};

Options parse_args(int argc, char* const* argv, std::vector<char*>& nextlevel) {
  static constexpr const char* help_msg =
      "Usage: [options]\n\n"
      "TensorIMA8A32 unsigned-bit probe.\n\n"
      "Required:\n"
      "  -k, --kernel_path               path to kernel ELF file.\n\n"
      "Optional:\n"
      "  -t, --kernel_launch_timeout     timeout in seconds.\n"
      "  -d, --device_type               device type (sysemu, fake, silicon).\n"
      "  -m, --shire_mask                shire mask in decimal or hex.\n"
      "  -p, --probe_mode                0=thread0 all, 1=thread1 selected, 2=thread1 all.\n"
      "  -r, --selected_relative_thread  relative thread for probe_mode=1.\n"
      "  -s, --show_records              print every completed record.\n";

  static constexpr const char* short_opts = "k:t:d:m:p:r:sh";
  static const std::vector<struct option> long_opts_vect{
      {"kernel_path", required_argument, nullptr, 'k'},
      {"kernel_launch_timeout", required_argument, nullptr, 't'},
      {"device_type", required_argument, nullptr, 'd'},
      {"shire_mask", required_argument, nullptr, 'm'},
      {"probe_mode", required_argument, nullptr, 'p'},
      {"selected_relative_thread", required_argument, nullptr, 'r'},
      {"show_records", no_argument, nullptr, 's'},
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
      opts.kernel_launch_timeout = std::atoi(optarg);
      break;
    case 'd':
      opts.device_type = optarg;
      break;
    case 'm':
      opts.shire_mask = std::stoull(optarg, nullptr, 0);
      break;
    case 'p':
      opts.probe_mode = static_cast<uint32_t>(std::stoul(optarg, nullptr, 0));
      break;
    case 'r':
      opts.selected_relative_thread = static_cast<uint32_t>(std::stoul(optarg, nullptr, 0));
      break;
    case 's':
      opts.show_records = true;
      break;
    case 'h':
      std::cout << help_msg << GenericLauncher::help_msg << std::endl;
      std::exit(0);
    case '?':
      nextlevel.emplace_back(argv[optind - 1]);
      break;
    default:
      std::cerr << "error: unknown option " << argv[optind - 1] << "\n";
      std::exit(1);
    }
  }

  return opts;
}

class TimaUnsignedBitsLauncher : public GenericLauncher {
public:
  TimaUnsignedBitsLauncher() = delete;
  using GenericLauncher::GenericLauncher;

  void prepareInput() {
    a_line_.fill(0);
    b_line_.fill(0);
    records_.assign(TIMA_UNSIGNED_BITS_MAX_RECORDS, {});

    a_line_[0] = 0xff;
    b_line_[0] = 0xfe;
  }

  void performDeviceAllocs() {
    deviceA_ = runtime_->mallocDevice(devices_[devIdx_], a_line_.size());
    deviceB_ = runtime_->mallocDevice(devices_[devIdx_], b_line_.size());
    deviceRecords_ = runtime_->mallocDevice(devices_[devIdx_], records_.size() * sizeof(TimaUnsignedBitsRecord));
  }

  void programHost2DevCopies() {
    runtime_->memcpyHostToDevice(defaultStreams_[devIdx_], reinterpret_cast<std::byte*>(a_line_.data()), deviceA_,
                                 a_line_.size());
    runtime_->memcpyHostToDevice(defaultStreams_[devIdx_], reinterpret_cast<std::byte*>(b_line_.data()), deviceB_,
                                 b_line_.size());
    runtime_->memcpyHostToDevice(defaultStreams_[devIdx_], reinterpret_cast<std::byte*>(records_.data()),
                                 deviceRecords_, records_.size() * sizeof(TimaUnsignedBitsRecord));
  }

  void programDev2HostCopies() {
    runtime_->memcpyDeviceToHost(defaultStreams_[devIdx_], deviceRecords_,
                                 reinterpret_cast<std::byte*>(records_.data()),
                                 records_.size() * sizeof(TimaUnsignedBitsRecord));
  }

  void freeDeviceAllocs() {
    runtime_->freeDevice(devices_[devIdx_], deviceA_);
    runtime_->freeDevice(devices_[devIdx_], deviceB_);
    runtime_->freeDevice(devices_[devIdx_], deviceRecords_);
  }

  std::array<uint8_t, 64> a_line_{};
  std::array<uint8_t, 64> b_line_{};
  std::vector<TimaUnsignedBitsRecord> records_{TIMA_UNSIGNED_BITS_MAX_RECORDS};
  std::byte* deviceA_ = nullptr;
  std::byte* deviceB_ = nullptr;
  std::byte* deviceRecords_ = nullptr;
};

static bool matches_prm(const TimaUnsignedBitsRecord& record) {
  return record.result_no_unsigned == 2 && record.result_bit21 == -510 &&
         record.result_bit22 == -254 && record.result_bit21_bit22 == 64770;
}

static bool matches_swapped(const TimaUnsignedBitsRecord& record) {
  return record.result_no_unsigned == 2 && record.result_bit21 == -254 &&
         record.result_bit22 == -510 && record.result_bit21_bit22 == 64770;
}

static bool matches_zero_result(const TimaUnsignedBitsRecord& record) {
  return record.result_no_unsigned == 0 && record.result_bit21 == 0 &&
         record.result_bit22 == 0 && record.result_bit21_bit22 == 0;
}

static bool helper_matches_contract(const TimaUnsignedBitsRecord& record) {
  return record.helper_no_unsigned == 2 && record.helper_tenb_unsigned == -254 &&
         record.helper_tena_unsigned == -510 && record.helper_both_unsigned == 64770;
}

static bool helper_matches_current_swap(const TimaUnsignedBitsRecord& record) {
  return record.helper_no_unsigned == 2 && record.helper_tenb_unsigned == -510 &&
         record.helper_tena_unsigned == -254 && record.helper_both_unsigned == 64770;
}

static bool helper_matches_zero_result(const TimaUnsignedBitsRecord& record) {
  return record.helper_no_unsigned == 0 && record.helper_tenb_unsigned == 0 &&
         record.helper_tena_unsigned == 0 && record.helper_both_unsigned == 0;
}

static void print_record(const TimaUnsignedBitsRecord& record) {
  std::cout << std::hex << "hart=" << record.hart_id << " rel=" << record.relative_thread_id
            << " shire=" << record.shire_id << " minion=" << record.minion_id
            << std::dec << " thread=" << record.thread_id << " status=" << record.status
            << " results={none:" << record.result_no_unsigned << ", bit21:" << record.result_bit21
            << ", bit22:" << record.result_bit22 << ", both:" << record.result_bit21_bit22 << "}"
            << " helper={none:" << record.helper_no_unsigned << ", tenb:" << record.helper_tenb_unsigned
            << ", tena:" << record.helper_tena_unsigned << ", both:" << record.helper_both_unsigned << "}\n";
}

static int analyze_records(const std::vector<TimaUnsignedBitsRecord>& records, bool show_records) {
  size_t marked = 0;
  size_t complete = 0;
  size_t prm = 0;
  size_t swapped = 0;
  size_t zero = 0;
  size_t other = 0;
  size_t helper_contract = 0;
  size_t helper_swapped = 0;
  size_t helper_zero = 0;
  size_t helper_other = 0;

  for (const auto& record : records) {
    if (record.magic != TIMA_UNSIGNED_BITS_MAGIC) {
      continue;
    }
    if (record.status == TIMA_UNSIGNED_BITS_MARKED_NO_TENSOR) {
      ++marked;
      if (show_records) {
        print_record(record);
      }
      continue;
    }
    if (record.status != TIMA_UNSIGNED_BITS_PROBE_COMPLETE) {
      continue;
    }

    ++complete;
    if (matches_prm(record)) {
      ++prm;
    } else if (matches_swapped(record)) {
      ++swapped;
    } else if (matches_zero_result(record)) {
      ++zero;
    } else {
      ++other;
    }

    if (helper_matches_contract(record)) {
      ++helper_contract;
    } else if (helper_matches_current_swap(record)) {
      ++helper_swapped;
    } else if (helper_matches_zero_result(record)) {
      ++helper_zero;
    } else {
      ++helper_other;
    }

    if (show_records) {
      print_record(record);
    }
  }

  std::cout << std::dec << "TensorIMA8A32 unsigned-bit probe summary:\n"
            << "  marked thread1 records: " << marked << "\n"
            << "  completed tensor probes: " << complete << "\n"
            << "  PRM mapping matches: " << prm << "\n"
            << "  swapped mapping matches: " << swapped << "\n"
            << "  zero/inactive results: " << zero << "\n"
            << "  other results: " << other << "\n"
            << "  helper contract matches: " << helper_contract << "\n"
            << "  helper current-swap matches: " << helper_swapped << "\n"
            << "  helper zero/inactive results: " << helper_zero << "\n"
            << "  helper other results: " << helper_other << "\n"
            << "Expected raw PRM mapping for A=0xff, B=0xfe is none=2, bit21(A unsigned)=-510, "
               "bit22(B unsigned)=-254, both=64770.\n"
            << "Expected tensor_fma() contract mapping is none=2, tenb_unsigned=-254, "
               "tena_unsigned=-510, both=64770.\n";

  if (complete == 0) {
    std::cerr << "error: no tensor probe records completed\n";
    return 2;
  }
  if (prm == 0) {
    std::cerr << "error: no completed tensor probe matched the PRM mapping\n";
    return 1;
  }
  if (swapped != 0 || other != 0) {
    std::cerr << "error: TensorIMA8A32 unsigned-bit mapping produced swapped or unexpected nonzero results\n";
    return 1;
  }
  if (helper_contract == 0) {
    std::cerr << "error: no tensor_fma() helper probe matched the documented argument contract\n";
    return 1;
  }
  if (helper_swapped != 0 || helper_other != 0) {
    std::cerr << "error: tensor_fma() helper produced swapped or unexpected nonzero results\n";
    return 1;
  }
  return 0;
}

int main(int argc, char** argv) {
  std::vector<char*> argvPendingToParse{argv[0]};
  Options opt = parse_args(argc, argv, argvPendingToParse);

  Config config{modeFromString(opt.device_type), 1};
  config.dump();
  std::cout << "Probe mode: " << opt.probe_mode << "\n";
  std::cout << "Shire mask: 0x" << std::hex << opt.shire_mask << std::dec << "\n";

  TimaUnsignedBitsLauncher launcher(config, static_cast<int>(argvPendingToParse.size()), argvPendingToParse.data());
  launcher.initialize();
  auto kernelId = launcher.loadKernel(opt.kernel_path);
  launcher.performDeviceAllocs();
  launcher.prepareInput();
  launcher.programHost2DevCopies();

  KernelArguments kernelArgs;
  kernelArgs.a_line = reinterpret_cast<const uint8_t*>(launcher.deviceA_);
  kernelArgs.b_line = reinterpret_cast<const uint8_t*>(launcher.deviceB_);
  kernelArgs.records = reinterpret_cast<TimaUnsignedBitsRecord*>(launcher.deviceRecords_);
  kernelArgs.max_records = static_cast<uint32_t>(launcher.records_.size());
  kernelArgs.probe_mode = opt.probe_mode;
  kernelArgs.selected_relative_thread = opt.selected_relative_thread;

  launcher.kernelLaunch(kernelId, &kernelArgs, nullptr, 0, 0, opt.shire_mask);
  launcher.programDev2HostCopies();
  launcher.waitKernelCompletion(std::chrono::seconds(opt.kernel_launch_timeout));
  launcher.dumpTracesToFile();

  if (launcher.checkKernelExecutionErrors()) {
    return -1;
  }

  int result = analyze_records(launcher.records_, opt.show_records);

  launcher.freeDeviceAllocs();
  launcher.unLoadKernel(kernelId);
  launcher.tearDown();

  return result;
}
