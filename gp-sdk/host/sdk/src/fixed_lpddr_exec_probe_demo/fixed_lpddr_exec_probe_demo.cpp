//******************************************************************************
// Copyright (c) 2026 Ainekko, Co.
// SPDX-License-Identifier: Apache-2.0
//------------------------------------------------------------------------------

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <getopt.h>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <elfio/elfio.hpp>

#include "GenericLauncher.h"
#include "gpsdk_scp_exec_probe.h"

namespace {

using gpsdk::examples::scp_exec_probe::KernelArguments;
using gpsdk::examples::scp_exec_probe::Result;

struct Options {
  fs::path probe_kernel_path = "";
  int kernel_launch_timeout = 30;
  std::string device_type = "silicon";
  uint32_t shire_mask = 0x200;
};

struct KernelMetadata {
  uint64_t entryAddress = 0ULL;
  uint64_t loadBaseAddress = 0ULL;
  uint64_t imageBytes = 0ULL;
};

class Launcher : public GenericLauncher {
public:
  using GenericLauncher::GenericLauncher;

  std::byte* allocateDeviceBuffer(size_t size, uint32_t deviceIdx = 0) {
    return runtime_->mallocDevice(devices_.at(deviceIdx), size);
  }

  void freeDeviceBuffer(std::byte* deviceBuffer, uint32_t deviceIdx = 0) {
    runtime_->freeDevice(devices_.at(deviceIdx), deviceBuffer);
  }

  rt::DeviceProperties getDeviceProperties(uint32_t deviceIdx = 0) const {
    return runtime_->getDeviceProperties(devices_.at(deviceIdx));
  }

  template <typename T> void writeObject(std::byte* deviceAddress, const T& value, uint32_t deviceIdx = 0) {
    runtime_->memcpyHostToDevice(defaultStreams_[deviceIdx], reinterpret_cast<const std::byte*>(&value), deviceAddress,
                                 sizeof(T));
    if (!runtime_->waitForStream(defaultStreams_[deviceIdx], std::chrono::seconds(5))) {
      throw std::runtime_error("Timed out while writing device memory");
    }
  }

  template <typename T> T readObject(std::byte* deviceAddress, uint32_t deviceIdx = 0) {
    T value{};
    runtime_->memcpyDeviceToHost(defaultStreams_[deviceIdx], deviceAddress, reinterpret_cast<std::byte*>(&value),
                                 sizeof(T));
    if (!runtime_->waitForStream(defaultStreams_[deviceIdx], std::chrono::seconds(5))) {
      throw std::runtime_error("Timed out while reading device memory");
    }
    return value;
  }
};

Options parse_args(int argc, char* const* argv, std::vector<char*>& nextlevel) {
  static constexpr const char* help_msg =
    "Usage: [options]\n\n"
    "Load a fixed-address probe ELF through runtime_->loadCode(), then launch it on LPDDR-backed DRAM\n"
    "to test whether ETSOC1 can execute from a caller-chosen fixed DRAM subregion.\n\n"
    "Required:\n"
    "  -k, --probe_kernel_path       path to the fixed-address exec probe kernel elf file.\n\n"
    "Optional:\n"
    "  -t, --kernel_launch_timeout   timeout (in seconds) to wait for kernel completion.\n"
    "  -d, --device_type             device type to use (sysemu, fake, silicon).\n"
    "  -m, --shire_mask              single-bit shire mask for launch.\n";

  static constexpr const char* short_opts = "k:t:d:m:h";
  static const std::vector<struct option> long_opts_vect{{"probe_kernel_path", required_argument, nullptr, 'k'},
                                                         {"kernel_launch_timeout", required_argument, nullptr, 't'},
                                                         {"device_type", required_argument, nullptr, 'd'},
                                                         {"shire_mask", required_argument, nullptr, 'm'},
                                                         {"help", no_argument, nullptr, 'h'},
                                                         {nullptr, 0, nullptr, 0}};

  Options opts;
  int ret = 0;
  int index = 0;
  opterr = 0;

  while ((ret = getopt_long_only(argc, argv, short_opts, long_opts_vect.data(), &index)) != -1) {
    switch (ret) {
    case 'k':
      opts.probe_kernel_path = optarg;
      break;
    case 't':
      opts.kernel_launch_timeout = std::atoi(optarg);
      break;
    case 'd':
      opts.device_type = optarg;
      break;
    case 'm':
      opts.shire_mask = std::stoul(optarg, nullptr, 0);
      break;
    case 'h':
      std::cout << help_msg << GenericLauncher::help_msg << std::endl;
      std::exit(0);
    case '?':
      nextlevel.emplace_back(argv[optind - 1]);
      break;
    default:
      std::cout << "Error: Unknown option " << argv[optind - 1] << ". See " << argv[0] << " --help'.\n";
      std::exit(1);
    }
  }

  if (opts.probe_kernel_path.empty()) {
    std::cout << "Error: --probe_kernel_path is required.\n";
    std::exit(1);
  }

  if (__builtin_popcount(opts.shire_mask) != 1) {
    std::cout << "Error: --shire_mask must select exactly one shire.\n";
    std::exit(1);
  }

  return opts;
}

std::vector<std::byte> readFile(const fs::path& path) {
  std::ifstream input(path, std::ios::binary);
  if (!input) {
    throw std::runtime_error("Unable to open file: " + path.string());
  }

  input.seekg(0, std::ios::end);
  const auto size = input.tellg();
  input.seekg(0, std::ios::beg);
  std::vector<std::byte> bytes(static_cast<size_t>(size));
  input.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  if (!input) {
    throw std::runtime_error("Unable to read file: " + path.string());
  }
  return bytes;
}

KernelMetadata inspectKernel(const fs::path& path) {
  const auto bytes = readFile(path);
  std::stringstream elfStream;
  elfStream.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  elfStream.seekg(0, std::ios::beg);

  ELFIO::elfio elf;
  if (!elf.load(elfStream)) {
    throw std::runtime_error("Unable to parse ELF: " + path.string());
  }

  KernelMetadata metadata;
  metadata.entryAddress = elf.get_entry();
  metadata.loadBaseAddress = std::numeric_limits<uint64_t>::max();
  uint64_t highestAddress = 0ULL;

  for (const auto& segment : elf.segments) {
    if (segment->get_type() != PT_LOAD) {
      continue;
    }
    metadata.loadBaseAddress = std::min<uint64_t>(metadata.loadBaseAddress, segment->get_physical_address());
    highestAddress = std::max<uint64_t>(highestAddress, segment->get_physical_address() + segment->get_memory_size());
  }

  if (metadata.loadBaseAddress == std::numeric_limits<uint64_t>::max()) {
    throw std::runtime_error("ELF has no PT_LOAD segments: " + path.string());
  }

  for (const auto& section : elf.sections) {
    if (section->get_type() != SHT_SYMTAB) {
      continue;
    }
    ELFIO::symbol_section_accessor symbols(elf, section);
    for (ELFIO::Elf_Xword idx = 0; idx < symbols.get_symbols_num(); ++idx) {
      std::string name;
      ELFIO::Elf64_Addr value = 0;
      ELFIO::Elf_Xword size = 0;
      unsigned char bind = 0;
      unsigned char type = 0;
      ELFIO::Elf_Half sectionIndex = 0;
      unsigned char other = 0;
      symbols.get_symbol(idx, name, value, size, bind, type, sectionIndex, other);
      if (name.find("entryPoint") != std::string::npos) {
        metadata.entryAddress = value;
        break;
      }
    }
  }

  metadata.imageBytes = highestAddress - metadata.loadBaseAddress;
  return metadata;
}

} // namespace

int main(int argc, char** argv) {
  std::vector<char*> argvPendingToParse{argv[0]};
  const Options opt = parse_args(argc, argv, argvPendingToParse);

  Config config{modeFromString(opt.device_type), 1};
  config.dump();

  Launcher launcher(config, static_cast<int>(argvPendingToParse.size()), const_cast<char**>(argvPendingToParse.data()),
                    false);
  std::byte* resultBuffer = nullptr;
  bool probeLoaded = false;
  rt::KernelId probeKernelId{};

  try {
    launcher.initialize();

    const auto props = launcher.getDeviceProperties();
    const uint64_t dramBase = props.localDRAMBaseAddress_;
    const uint64_t dramEnd = props.localDRAMBaseAddress_ + props.localDRAMSize_;
    const auto metadata = inspectKernel(opt.probe_kernel_path);

    if ((metadata.loadBaseAddress < dramBase) || ((metadata.loadBaseAddress + metadata.imageBytes) > dramEnd)) {
      std::ostringstream oss;
      oss << "Fixed-address ELF range [0x" << std::hex << metadata.loadBaseAddress << ",0x"
          << (metadata.loadBaseAddress + metadata.imageBytes) << ") is outside device DRAM [0x" << dramBase << ",0x"
          << dramEnd << ")";
      throw std::runtime_error(oss.str());
    }

    resultBuffer = launcher.allocateDeviceBuffer(sizeof(Result));
    launcher.writeObject(resultBuffer, Result{});

    probeKernelId = launcher.loadKernel(opt.probe_kernel_path.string());
    probeLoaded = true;

    KernelArguments args;
    args.resultAddress = reinterpret_cast<uint64_t>(resultBuffer);
    args.expectedEntryAddress = metadata.entryAddress;

    launcher.kernelLaunch(probeKernelId, &args, nullptr, 0, 0, opt.shire_mask);
    launcher.waitKernelCompletion(std::chrono::seconds(opt.kernel_launch_timeout));

    if (launcher.checkKernelExecutionErrors()) {
      std::cout << "Fixed LPDDR exec probe failed: launch reported a kernel error before success could be observed.\n";
      launcher.freeDeviceBuffer(resultBuffer);
      launcher.unLoadKernel(probeKernelId);
      launcher.tearDown();
      return 1;
    }

    const auto result = launcher.readObject<Result>(resultBuffer);
    std::cout << "Local DRAM base: 0x" << std::hex << dramBase << "\n";
    std::cout << "Fixed ELF load base: 0x" << metadata.loadBaseAddress << "\n";
    std::cout << "Expected entry address: 0x" << result.expectedEntryAddress << "\n";
    std::cout << "Observed entry address: 0x" << result.observedEntryAddress << std::dec << "\n";

    const bool passed = (result.magic == gpsdk::examples::scp_exec_probe::kResultMagic) && (result.started == 1U) &&
                        (result.completed == 1U) && (result.observedEntryAddress == result.expectedEntryAddress);

    if (passed) {
      std::cout << "Fixed LPDDR exec probe passed.\n";
    } else {
      std::cout << "Fixed LPDDR exec probe failed: result mismatch.\n";
    }

    launcher.freeDeviceBuffer(resultBuffer);
    launcher.unLoadKernel(probeKernelId);
    launcher.tearDown();
    return passed ? 0 : 1;
  } catch (const std::exception& ex) {
    std::cout << "Fixed LPDDR exec probe failed unexpectedly: " << ex.what() << "\n";
    if ((resultBuffer != nullptr) && (launcher.getNumDevices() != 0U)) {
      launcher.freeDeviceBuffer(resultBuffer);
    }
    if (probeLoaded) {
      launcher.unLoadKernel(probeKernelId);
    }
    launcher.tearDown();
    return 1;
  }
}
