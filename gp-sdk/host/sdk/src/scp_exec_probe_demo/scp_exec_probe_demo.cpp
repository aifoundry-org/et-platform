//******************************************************************************
// Copyright (c) 2026 Ainekko, Co.
// SPDX-License-Identifier: Apache-2.0
//------------------------------------------------------------------------------

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <getopt.h>
#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <elfio/elfio.hpp>

#include "GenericLauncher.h"
#include "gpsdk_scp_exec_probe.h"
#include "gpsdk_star_scratchpad.h"

namespace {

using gpsdk::examples::scp_exec_probe::KernelArguments;
using gpsdk::examples::scp_exec_probe::Result;

constexpr uint32_t kRiscv64RelocationType = 2U;
constexpr uint32_t kElfBaseOffset = 0x1000U;

struct Options {
  fs::path probe_kernel_path = "";
  fs::path stub_kernel_path = "";
  fs::path topology_probe_kernel = "";
  int kernel_launch_timeout = 30;
  std::string device_type = "silicon";
  uint32_t shire_mask = 0x200;
};

struct RelocatedImage {
  std::vector<std::byte> fileBytes;
  ELFIO::elfio elf;
  uint64_t targetBaseAddress = 0ULL;
  uint64_t entryAddress = 0ULL;
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

  void memcpyHostToAddress(const std::byte* hostAddress, uint64_t deviceAddress, size_t size, uint32_t deviceIdx = 0) {
    runtime_->memcpyHostToDevice(defaultStreams_[deviceIdx], hostAddress, reinterpret_cast<std::byte*>(deviceAddress),
                                 size);
  }

  void launchFromAddress(rt::KernelId kernelId, const std::byte* params, size_t paramsSize, uint64_t codeStartAddress,
                         uint64_t launchShireMask, uint64_t computeShireMask, int activeNeighborhood,
                         bool erbiumSimEnabled, const gpsdk::star_scratchpad::ClusterSelection* selection,
                         gpsdk::star_scratchpad::ClusterLayout layout, uint32_t deviceIdx = 0) {
    rt::KernelLaunchOptions options;
    options.setShireMask(launchShireMask);
    options.setBarrier(true);
    options.setFlushL3(false);
    options.setCodeStartAddress(reinterpret_cast<std::byte*>(codeStartAddress));

    std::vector<std::byte> wrappedParams;
    const std::byte* launchParams = params;
    size_t launchParamsSize = paramsSize;

    if ((activeNeighborhood >= 0) || (selection != nullptr)) {
      gpsdk::launch::RuntimeArgsHeader header;
      header.computeShireMask = computeShireMask;
      if (activeNeighborhood >= 0) {
        header.flags |= gpsdk::launch::kLaunchFlagSingleNeighborhoodPerShire;
        header.activeNeighborhood = static_cast<uint8_t>(activeNeighborhood);
      }
      if (selection != nullptr) {
        switch (layout) {
        case gpsdk::star_scratchpad::ClusterLayout::Star:
          header.flags |= gpsdk::launch::kLaunchFlagScratchpadStarCluster;
          break;
        case gpsdk::star_scratchpad::ClusterLayout::Block:
          header.flags |= gpsdk::launch::kLaunchFlagScratchpadBlockCluster;
          break;
        case gpsdk::star_scratchpad::ClusterLayout::NestedStar:
          header.flags |= gpsdk::launch::kLaunchFlagScratchpadNestedStarCluster;
          break;
        }
        header.effectiveCenterShire = static_cast<uint8_t>(selection->effectiveCenterShire);
        header.scratchpadRelayCount = selection->relayCount;
        header.scratchpadAuxiliaryCount = selection->auxiliaryCount;
        std::copy(selection->relayShires.begin(), selection->relayShires.end(), header.scratchpadRelayShires);
        std::copy(selection->auxiliaryShires.begin(), selection->auxiliaryShires.end(), header.scratchpadAuxiliaryShires);
      }
      if (erbiumSimEnabled) {
        header.flags |= gpsdk::launch::kLaunchFlagErbiumSim;
      }
      header.payloadSize = static_cast<uint32_t>(paramsSize);

      wrappedParams.resize(sizeof(header) + paramsSize);
      std::memcpy(wrappedParams.data(), &header, sizeof(header));
      if (paramsSize != 0U) {
        std::memcpy(wrappedParams.data() + sizeof(header), params, paramsSize);
      }

      launchParams = wrappedParams.data();
      launchParamsSize = wrappedParams.size();
    }

    runtime_->kernelLaunch(defaultStreams_[deviceIdx], kernelId, launchParams, launchParamsSize, options);
  }
};

Options parse_args(int argc, char* const* argv, std::vector<char*>& nextlevel) {
  static constexpr const char* help_msg =
    "Usage: [options]\n\n"
    "Stage a probe ELF into scratchpad, then launch with an overridden code_start_address\n"
    "to test whether SCP-resident code is executable.\n\n"
    "Required:\n"
    "  -k, --probe_kernel_path       path to the SCP exec probe kernel elf file.\n"
    "  -s, --stub_kernel_path        path to the failing stub kernel elf file.\n\n"
    "Optional:\n"
    "  -t, --kernel_launch_timeout   timeout (in seconds) to wait for kernel completion.\n"
    "  -d, --device_type             device type to use (sysemu, fake, silicon).\n"
    "  -m, --shire_mask              single-bit requested center shire mask.\n";

  static constexpr const char* short_opts = "k:s:t:d:m:h";
  static const std::vector<struct option> long_opts_vect{{"probe_kernel_path", required_argument, nullptr, 'k'},
                                                         {"stub_kernel_path", required_argument, nullptr, 's'},
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
    case 's':
      opts.stub_kernel_path = optarg;
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

  if (opts.probe_kernel_path.empty() || opts.stub_kernel_path.empty()) {
    std::cout << "Error: --probe_kernel_path and --stub_kernel_path are required.\n";
    std::exit(1);
  }

  if (__builtin_popcount(opts.shire_mask) != 1) {
    std::cout << "Error: --shire_mask must select exactly one center shire.\n";
    std::exit(1);
  }

  return opts;
}

bool hasArgument(const std::vector<char*>& args, const char* target) {
  for (const char* arg : args) {
    if ((arg != nullptr) && (std::strcmp(arg, target) == 0)) {
      return true;
    }
  }
  return false;
}

std::optional<std::string> getArgumentValue(const std::vector<char*>& args, const char* prefix) {
  const size_t prefixLen = std::strlen(prefix);
  for (const char* arg : args) {
    if ((arg != nullptr) && (std::strncmp(arg, prefix, prefixLen) == 0)) {
      return std::string(arg + prefixLen);
    }
  }
  return std::nullopt;
}

gpsdk::star_scratchpad::ClusterLayout getRequestedClusterLayout(const std::vector<char*>& args) {
  static constexpr char starFlag[] = "--scratchpad_star";
  static constexpr char blockFlag[] = "--scratchpad_block";
  static constexpr char nestedFlag[] = "--scratchpad_nested_star";

  const bool star = hasArgument(args, starFlag);
  const bool block = hasArgument(args, blockFlag);
  const bool nested = hasArgument(args, nestedFlag);
  const auto count = static_cast<uint32_t>(star) + static_cast<uint32_t>(block) + static_cast<uint32_t>(nested);
  if (count != 1U) {
    throw std::runtime_error("Pass exactly one of --scratchpad_star, --scratchpad_block, or --scratchpad_nested_star");
  }

  if (nested) {
    return gpsdk::star_scratchpad::ClusterLayout::NestedStar;
  }
  return block ? gpsdk::star_scratchpad::ClusterLayout::Block : gpsdk::star_scratchpad::ClusterLayout::Star;
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

void relocateProbeImage(std::byte* runtimeBaseAddress, ELFIO::elfio& elf, std::byte* elfContents,
                        ELFIO::Elf64_Addr elfBaseAddr) {
  for (const auto& section : elf.sections) {
    if (section->get_type() != SHT_RELA) {
      continue;
    }
    if (section->get_name().find(".rela.debug") != std::string::npos) {
      continue;
    }

    ELFIO::relocation_section_accessor relocSec(elf, section);
    for (ELFIO::Elf_Xword i = 0; i < relocSec.get_entries_num(); ++i) {
      ELFIO::Elf64_Addr offset;
      ELFIO::Elf_Word symbolIndex;
      ELFIO::Elf_Word type;
      ELFIO::Elf_Sxword addend;
      relocSec.get_entry(i, offset, symbolIndex, type, addend);
      if (type != kRiscv64RelocationType) {
        continue;
      }
      auto* targetPtr = reinterpret_cast<uint64_t*>(elfContents + offset - elfBaseAddr + kElfBaseOffset);
      *targetPtr += reinterpret_cast<uint64_t>(runtimeBaseAddress) - elfBaseAddr + kElfBaseOffset;
    }
  }
}

std::tuple<ELFIO::Elf64_Addr, size_t> getElfBaseAddr(const ELFIO::elfio& elf) {
  ELFIO::Elf64_Addr elfBaseAddr = std::numeric_limits<ELFIO::Elf64_Addr>::max();
  auto extraSize = 0UL;
  for (const auto& segment : elf.segments) {
    if (segment->get_type() != PT_LOAD) {
      continue;
    }
    if (segment->get_physical_address() < elfBaseAddr) {
      elfBaseAddr = segment->get_physical_address();
    }
    extraSize += segment->get_memory_size() - segment->get_file_size();
  }
  return {elfBaseAddr, extraSize};
}

RelocatedImage prepareRelocatedImage(const fs::path& probePath, uint64_t targetBaseAddress) {
  RelocatedImage image;
  image.fileBytes = readFile(probePath);
  std::stringstream elfStream;
  elfStream.write(reinterpret_cast<const char*>(image.fileBytes.data()), static_cast<std::streamsize>(image.fileBytes.size()));
  elfStream.seekg(0, std::ios::beg);
  if (!image.elf.load(elfStream)) {
    throw std::runtime_error("Unable to parse ELF: " + probePath.string());
  }

  const auto [elfBaseAddr, extraSize] = getElfBaseAddr(image.elf);
  relocateProbeImage(reinterpret_cast<std::byte*>(targetBaseAddress), image.elf, image.fileBytes.data(), elfBaseAddr);

  uint64_t maxOffset = 0ULL;
  for (const auto& segment : image.elf.segments) {
    if (segment->get_type() != PT_LOAD) {
      continue;
    }
    maxOffset = std::max<uint64_t>(maxOffset, segment->get_offset() + segment->get_memory_size());
  }

  image.targetBaseAddress = targetBaseAddress;
  image.entryAddress = targetBaseAddress + (image.elf.get_entry() - elfBaseAddr);
  image.imageBytes = maxOffset;
  return image;
}

void stageRelocatedImage(Launcher& launcher, const RelocatedImage& image, uint32_t deviceIdx = 0) {
  for (const auto& segment : image.elf.segments) {
    if (segment->get_type() != PT_LOAD) {
      continue;
    }

    const auto offset = segment->get_offset();
    const auto fileSize = segment->get_file_size();
    const auto memSize = segment->get_memory_size();
    if (memSize == 0U) {
      continue;
    }

    std::vector<std::byte> buffer(memSize);
    std::memcpy(buffer.data(), image.fileBytes.data() + offset, fileSize);
    if (memSize > fileSize) {
      std::memset(buffer.data() + fileSize, 0, memSize - fileSize);
    }
    launcher.memcpyHostToAddress(buffer.data(), image.targetBaseAddress + offset, memSize, deviceIdx);
    launcher.waitKernelCompletion(std::chrono::seconds(5), deviceIdx);
  }
}

} // namespace

int main(int argc, char** argv) {
  std::vector<char*> argvPendingToParse{argv[0]};
  const Options opt = parse_args(argc, argv, argvPendingToParse);

  const auto layout = getRequestedClusterLayout(argvPendingToParse);
  const int activeNeighborhood = [&]() {
    const auto value = getArgumentValue(argvPendingToParse, "--active_neighborhood=");
    return value.has_value() ? std::stoi(*value) : -1;
  }();
  const bool erbiumSimEnabled = hasArgument(argvPendingToParse, "--erbium_sim");

  Config config{modeFromString(opt.device_type), 1};
  config.dump();

  Launcher launcher(config, static_cast<int>(argvPendingToParse.size()), const_cast<char**>(argvPendingToParse.data()),
                    false);
  std::byte* resultBuffer = nullptr;
  bool stubLoaded = false;
  rt::KernelId stubKernelId{};

  try {
    launcher.initialize();
    const auto selection = launcher.resolveScratchpadClusterSelection(opt.shire_mask, layout);
    if (!selection.valid() || (selection.auxiliaryCount == 0U)) {
      throw std::runtime_error("Unable to resolve scratchpad cluster selection");
    }

    const uint32_t execShire = selection.auxiliaryShires[0];
    const uint64_t execBaseAddress = gpsdk::star_scratchpad::poolShardBaseAddress(execShire);
    const auto image = prepareRelocatedImage(opt.probe_kernel_path, execBaseAddress);
    if (image.imageBytes > gpsdk::star_scratchpad::kPoolBytesPerAuxShire) {
      throw std::runtime_error("Probe image does not fit in a single 2 MiB scratchpad shard");
    }
    stageRelocatedImage(launcher, image);

    resultBuffer = launcher.allocateDeviceBuffer(sizeof(Result));
    launcher.writeObject(resultBuffer, Result{});

    stubKernelId = launcher.loadKernel(opt.stub_kernel_path.string());
    stubLoaded = true;

    KernelArguments args;
    args.resultAddress = reinterpret_cast<uint64_t>(resultBuffer);
    args.expectedEntryAddress = image.entryAddress;

    launcher.launchFromAddress(stubKernelId, reinterpret_cast<const std::byte*>(&args), sizeof(args), image.entryAddress,
                               selection.launchedShireMask, selection.computeShireMask, activeNeighborhood,
                               erbiumSimEnabled, &selection, layout);
    launcher.waitKernelCompletion(std::chrono::seconds(opt.kernel_launch_timeout));

    if (launcher.checkKernelExecutionErrors()) {
      std::cout << "SCP exec probe failed: launch reported a kernel error before success could be observed.\n";
      launcher.freeDeviceBuffer(resultBuffer);
      launcher.unLoadKernel(stubKernelId);
      launcher.tearDown();
      return 1;
    }

    const auto result = launcher.readObject<Result>(resultBuffer);
    std::cout << "Exec shire: " << execShire << "\n";
    std::cout << "Exec base address: 0x" << std::hex << execBaseAddress << std::dec << "\n";
    std::cout << "Expected entry address: 0x" << std::hex << result.expectedEntryAddress << "\n";
    std::cout << "Observed entry address: 0x" << result.observedEntryAddress << std::dec << "\n";

    const bool passed = (result.magic == gpsdk::examples::scp_exec_probe::kResultMagic) && (result.started == 1U) &&
                        (result.completed == 1U) && (result.observedEntryAddress == result.expectedEntryAddress);

    if (passed) {
      std::cout << "SCP exec probe passed.\n";
    } else {
      std::cout << "SCP exec probe failed: result mismatch.\n";
    }

    launcher.freeDeviceBuffer(resultBuffer);
    launcher.unLoadKernel(stubKernelId);
    launcher.tearDown();
    return passed ? 0 : 1;
  } catch (const std::exception& ex) {
    std::cout << "SCP exec probe failed unexpectedly: " << ex.what() << "\n";
    if ((resultBuffer != nullptr) && (launcher.getNumDevices() != 0U)) {
      launcher.freeDeviceBuffer(resultBuffer);
    }
    if (stubLoaded) {
      launcher.unLoadKernel(stubKernelId);
    }
    launcher.tearDown();
    return 1;
  }
}
