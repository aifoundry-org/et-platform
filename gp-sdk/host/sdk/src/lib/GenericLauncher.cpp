//******************************************************************************
// Copyright (c) 2025 Ainekko, Co.
// SPDX-License-Identifier: Apache-2.0
//------------------------------------------------------------------------------

#include <cassert>
#include <algorithm>
#include <esperanto/et-trace/encoder.h>
#include <fstream>
#include <iostream>
#include <iterator>
#include <cstring>
#include <runtime/DeviceLayerFake.h>
#include <runtime/Types.h>
#include <sw-sysemu/SysEmuOptions.h>

#include <chrono>
#include <iomanip>
#include <getopt.h>
#include <limits>
#include <optional>
#include <set>
#include <sstream>
#include <stdio.h>
#include <thread>
#include <tuple>
#include <vector>

#include "GenericLauncher.h"
#include "gpsdk_star_scratchpad.h"
#include "gpsdk_topology_probe.h"

// Trace Buffer realted constants.
constexpr size_t kTraceBytesPerHart = 4096;
constexpr size_t kNumHarts = 2048;
constexpr size_t kTraceBufferSize = kTraceBytesPerHart * kNumHarts;

struct GenericLauncher::InferredTopology {
  uint64_t activeComputeShireMask = 0ULL;
  std::array<std::array<uint16_t, gpsdk::topology_probe::kVisibleComputeShires>,
             gpsdk::topology_probe::kVisibleComputeShires>
    averageCycles = {};
  std::array<gpsdk::star_scratchpad::ClusterSelection, gpsdk::topology_probe::kVisibleComputeShires> nestedSelections = {};
  std::array<bool, gpsdk::topology_probe::kVisibleComputeShires> hasNestedSelection = {};
  std::filesystem::path cachePath;
};

namespace {
std::string getEtSdkHome() {
  if (const char* etSdkHome = std::getenv("ET_SDK_HOME")) {
    return etSdkHome;
  }
  return "/opt/et";
}

bool kernelTracesEnabled() {
  if (const char* disable = std::getenv("ET_DISABLE_KERNEL_TRACES")) {
    if (disable[0] == '1' || disable[0] == 'y' || disable[0] == 'Y' || disable[0] == 't' || disable[0] == 'T') {
      return false;
    }
  }
  if (const char* enable = std::getenv("ET_ENABLE_KERNEL_TRACES")) {
    if (enable[0] == '0' || enable[0] == 'n' || enable[0] == 'N' || enable[0] == 'f' || enable[0] == 'F') {
      return false;
    }
  }
  return true;
}

bool debugLoadPhasesEnabled() {
  if (const char* value = std::getenv("GPSDK_DEBUG_LOAD_PHASES")) {
    switch (value[0]) {
    case '0':
    case 'n':
    case 'N':
    case 'f':
    case 'F':
      return false;
    default:
      return true;
    }
  }
  return false;
}

bool skipLoadWaitEnabled() {
  if (const char* value = std::getenv("GPSDK_SKIP_LOAD_WAIT")) {
    switch (value[0]) {
    case '0':
    case 'n':
    case 'N':
    case 'f':
    case 'F':
      return false;
    default:
      return true;
    }
  }
  return false;
}

std::chrono::milliseconds getLoadQuiesceDelay() {
  if (const char* value = std::getenv("GPSDK_LOAD_QUIESCE_MS")) {
    try {
      return std::chrono::milliseconds(std::max(0, std::stoi(value)));
    } catch (...) {
    }
  }
  return std::chrono::milliseconds(1000);
}

constexpr uint32_t kTopologyCacheVersion = 1U;
constexpr uint32_t kTopologyNeighborCount = 4U;
constexpr uint32_t kTopologyLeafCountPerRelay = 2U;

std::string formatMask(uint64_t mask) {
  std::ostringstream oss;
  oss << "0x" << std::hex << mask;
  return oss.str();
}

std::filesystem::path defaultTopologyCacheDir() {
  if (const char* home = std::getenv("HOME")) {
    return std::filesystem::path(home) / ".cache" / "et" / "gpsdk";
  }
  return std::filesystem::temp_directory_path() / "gpsdk-topology";
}

struct MatrixRowOrdering {
  std::array<uint32_t, gpsdk::topology_probe::kVisibleComputeShires> nodes = {};
};

std::array<uint32_t, gpsdk::topology_probe::kVisibleComputeShires> sortedNeighbors(
  const GenericLauncher::InferredTopology& topology, uint32_t centerShire) {
  std::array<uint32_t, gpsdk::topology_probe::kVisibleComputeShires> ordered = {};
  for (uint32_t idx = 0U; idx < ordered.size(); ++idx) {
    ordered[idx] = idx;
  }

  const auto& centerRow = topology.averageCycles[centerShire];
  std::sort(ordered.begin(), ordered.end(), [&](uint32_t lhs, uint32_t rhs) {
    if (lhs == centerShire) {
      return true;
    }
    if (rhs == centerShire) {
      return false;
    }
    if (centerRow[lhs] != centerRow[rhs]) {
      return centerRow[lhs] < centerRow[rhs];
    }
    return lhs < rhs;
  });
  return ordered;
}

std::vector<uint32_t> activeShiresFromMask(uint64_t mask) {
  std::vector<uint32_t> shires;
  while (mask != 0ULL) {
    const auto shire = static_cast<uint32_t>(__builtin_ctzll(mask));
    shires.push_back(shire);
    mask &= (mask - 1ULL);
  }
  return shires;
}

std::optional<gpsdk::star_scratchpad::ClusterSelection> buildNestedSelection(
  const GenericLauncher::InferredTopology& topology, uint32_t centerShire) {
  if (((topology.activeComputeShireMask >> centerShire) & 0x1ULL) == 0ULL) {
    return std::nullopt;
  }

  const auto ordered = sortedNeighbors(topology, centerShire);
  gpsdk::star_scratchpad::ClusterSelection selection;
  selection.effectiveCenterShire = centerShire;
  selection.computeShireMask = (1ULL << centerShire);
  selection.relayCount = gpsdk::star_scratchpad::kNestedRelayCount;
  selection.auxiliaryCount = gpsdk::star_scratchpad::kNestedLeafCount;

  std::array<uint32_t, kTopologyNeighborCount> relays = {};
  uint32_t relayFound = 0U;
  for (uint32_t candidate : ordered) {
    if ((candidate == centerShire) || (((topology.activeComputeShireMask >> candidate) & 0x1ULL) == 0ULL)) {
      continue;
    }
    relays[relayFound++] = candidate;
    if (relayFound == kTopologyNeighborCount) {
      break;
    }
  }

  if (relayFound != kTopologyNeighborCount) {
    return std::nullopt;
  }

  std::set<uint32_t> usedLeaves;
  for (uint32_t relayIdx = 0U; relayIdx < relays.size(); ++relayIdx) {
    const auto relay = relays[relayIdx];
    selection.relayShires[relayIdx] = static_cast<uint8_t>(relay);

    const auto relayOrdered = sortedNeighbors(topology, relay);
    uint32_t chosenLeafs = 0U;
    for (uint32_t candidate : relayOrdered) {
      if ((candidate == relay) || (candidate == centerShire) ||
          (((topology.activeComputeShireMask >> candidate) & 0x1ULL) == 0ULL)) {
        continue;
      }

      bool isRelay = false;
      for (uint32_t relayCheck : relays) {
        if (candidate == relayCheck) {
          isRelay = true;
          break;
        }
      }
      if (isRelay || usedLeaves.count(candidate) != 0U) {
        continue;
      }

      selection.auxiliaryShires[(relayIdx * kTopologyLeafCountPerRelay) + chosenLeafs] = static_cast<uint8_t>(candidate);
      selection.launchedShireMask |= (1ULL << candidate);
      usedLeaves.insert(candidate);
      ++chosenLeafs;
      if (chosenLeafs == kTopologyLeafCountPerRelay) {
        break;
      }
    }

    if (chosenLeafs != kTopologyLeafCountPerRelay) {
      return std::nullopt;
    }

    selection.launchedShireMask |= (1ULL << relay);
  }

  selection.launchedShireMask |= (1ULL << centerShire);
  return selection;
}

bool loadTopologyCacheFile(const std::filesystem::path& cachePath, GenericLauncher::InferredTopology& topology) {
  std::ifstream input(cachePath);
  if (!input.is_open()) {
    return false;
  }

  uint32_t version = 0U;
  std::string token;
  input >> token >> version;
  if (!input.good() || (token != "version") || (version != kTopologyCacheVersion)) {
    return false;
  }

  std::string maskText;
  input >> token >> maskText;
  if (!input.good() || (token != "active_mask")) {
    return false;
  }
  topology.activeComputeShireMask = std::stoull(maskText, nullptr, 0);

  for (uint32_t row = 0U; row < gpsdk::topology_probe::kVisibleComputeShires; ++row) {
    input >> token;
    if (!input.good() || (token != "row")) {
      return false;
    }
    uint32_t rowIndex = 0U;
    input >> rowIndex;
    if (rowIndex != row) {
      return false;
    }
    for (uint32_t col = 0U; col < gpsdk::topology_probe::kVisibleComputeShires; ++col) {
      uint32_t cycles = 0U;
      input >> cycles;
      topology.averageCycles[row][col] = static_cast<uint16_t>(cycles);
    }
  }

  while (input >> token) {
    if (token != "nested") {
      return false;
    }
    uint32_t center = 0U;
    input >> center;
    if (center >= gpsdk::topology_probe::kVisibleComputeShires) {
      return false;
    }

    auto& selection = topology.nestedSelections[center];
    selection.effectiveCenterShire = center;
    selection.computeShireMask = (1ULL << center);

    std::string launchedMaskText;
    input >> token >> launchedMaskText;
    if (token != "launched_mask") {
      return false;
    }
    selection.launchedShireMask = std::stoull(launchedMaskText, nullptr, 0);

    uint32_t relayCount = 0U;
    input >> token >> relayCount;
    if (token != "relays") {
      return false;
    }
    selection.relayCount = static_cast<uint8_t>(relayCount);
    for (uint32_t idx = 0U; idx < relayCount; ++idx) {
      uint32_t relay = 0U;
      input >> relay;
      selection.relayShires[idx] = static_cast<uint8_t>(relay);
    }

    uint32_t auxiliaryCount = 0U;
    input >> token >> auxiliaryCount;
    if (token != "aux") {
      return false;
    }
    selection.auxiliaryCount = static_cast<uint8_t>(auxiliaryCount);
    for (uint32_t idx = 0U; idx < auxiliaryCount; ++idx) {
      uint32_t auxiliary = 0U;
      input >> auxiliary;
      selection.auxiliaryShires[idx] = static_cast<uint8_t>(auxiliary);
    }

    topology.hasNestedSelection[center] = true;
  }

  topology.cachePath = cachePath;
  return true;
}

void saveTopologyCacheFile(const std::filesystem::path& cachePath, const GenericLauncher::InferredTopology& topology) {
  std::filesystem::create_directories(cachePath.parent_path());
  std::ofstream output(cachePath, std::ios::trunc);
  output << "version " << kTopologyCacheVersion << "\n";
  output << "active_mask " << formatMask(topology.activeComputeShireMask) << "\n";
  for (uint32_t row = 0U; row < gpsdk::topology_probe::kVisibleComputeShires; ++row) {
    output << "row " << row;
    for (uint32_t col = 0U; col < gpsdk::topology_probe::kVisibleComputeShires; ++col) {
      output << " " << topology.averageCycles[row][col];
    }
    output << "\n";
  }
  for (uint32_t center = 0U; center < gpsdk::topology_probe::kVisibleComputeShires; ++center) {
    if (!topology.hasNestedSelection[center]) {
      continue;
    }
    const auto& selection = topology.nestedSelections[center];
    output << "nested " << center << " launched_mask " << formatMask(selection.launchedShireMask) << " relays "
           << static_cast<uint32_t>(selection.relayCount);
    for (uint32_t idx = 0U; idx < selection.relayCount; ++idx) {
      output << " " << static_cast<uint32_t>(selection.relayShires[idx]);
    }
    output << " aux " << static_cast<uint32_t>(selection.auxiliaryCount);
    for (uint32_t idx = 0U; idx < selection.auxiliaryCount; ++idx) {
      output << " " << static_cast<uint32_t>(selection.auxiliaryShires[idx]);
    }
    output << "\n";
  }
}

void setIfExists(std::string& dst, const std::filesystem::path& path) {
  if (std::filesystem::exists(path)) {
    dst = path.string();
  }
}

void exportScratchpadAddressMapFile(const std::filesystem::path& outputPath, uint64_t requestedShireMask,
                                    gpsdk::star_scratchpad::ClusterLayout layout,
                                    const gpsdk::star_scratchpad::ClusterSelection& selection) {
  if (outputPath.empty()) {
    return;
  }

  if (!outputPath.parent_path().empty()) {
    std::filesystem::create_directories(outputPath.parent_path());
  }
  std::ofstream output(outputPath, std::ios::trunc);
  output << "layout ";
  switch (layout) {
  case gpsdk::star_scratchpad::ClusterLayout::Star:
    output << "star\n";
    break;
  case gpsdk::star_scratchpad::ClusterLayout::Block:
    output << "block\n";
    break;
  case gpsdk::star_scratchpad::ClusterLayout::NestedStar:
    output << "nested_star\n";
    break;
  }
  output << "requested_center_mask " << formatMask(requestedShireMask) << "\n";
  output << "compute_shire_mask " << formatMask(selection.computeShireMask) << "\n";
  output << "launched_shire_mask " << formatMask(selection.launchedShireMask) << "\n";
  output << "effective_center_shire " << std::dec << selection.effectiveCenterShire << "\n";
  output << "center_shifted " << (selection.centerShifted ? 1 : 0) << "\n";
  output << "relay_count " << static_cast<uint32_t>(selection.relayCount) << "\n";
  for (uint32_t idx = 0U; idx < selection.relayCount; ++idx) {
    output << "relay " << idx << " shire " << static_cast<uint32_t>(selection.relayShires[idx]) << "\n";
  }
  output << "auxiliary_count " << static_cast<uint32_t>(selection.auxiliaryCount) << "\n";
  output << "pool_bytes_per_shire 0x" << std::hex << gpsdk::star_scratchpad::kPoolBytesPerAuxShire << "\n";
  output << "pool_total_bytes 0x" << std::hex << gpsdk::star_scratchpad::poolCapacity(layout) << "\n";
  output << "success_marker_address 0x" << std::hex
         << gpsdk::star_scratchpad::successMarkerAddress(selection.effectiveCenterShire) << "\n";

  for (uint32_t idx = 0U; idx < selection.auxiliaryCount; ++idx) {
    const auto entry =
      gpsdk::star_scratchpad::poolShardAddressMapEntry(idx, static_cast<uint32_t>(selection.auxiliaryShires[idx]));
    output << "shard " << std::dec << entry.shardIndex << " shire " << entry.shireId << " logical_base 0x" << std::hex
           << entry.logicalBaseOffset << " logical_limit 0x" << entry.logicalLimitOffset << " shire_base_offset 0x"
           << entry.shireBaseOffset << " shire_limit_offset 0x" << entry.shireLimitOffset << " base_address 0x"
           << entry.baseAddress << " limit_address 0x" << entry.limitAddress << "\n";
  }
}

std::filesystem::path resolveKernelPath(const std::filesystem::path& kernelPath, Mode mode) {
  if (mode != Mode::SYSEMU) {
    return kernelPath;
  }

  const auto kernelPathStr = kernelPath.string();
  if (kernelPathStr.size() >= 4 && kernelPathStr.compare(kernelPathStr.size() - 4, 4, "_dbg") == 0) {
    return kernelPath;
  }

  const auto debugKernelPath = std::filesystem::path(kernelPathStr + "_dbg");
  if (std::filesystem::exists(debugKernelPath)) {
    std::cout << "loadKernel() sysemu selected, using debug-linked kernel " << debugKernelPath << "\n";
    return debugKernelPath;
  }

  return kernelPath;
}

const char* clusterOptionName(gpsdk::star_scratchpad::ClusterLayout layout) {
  switch (layout) {
  case gpsdk::star_scratchpad::ClusterLayout::Star:
    return "--scratchpad_star";
  case gpsdk::star_scratchpad::ClusterLayout::Block:
    return "--scratchpad_block";
  case gpsdk::star_scratchpad::ClusterLayout::NestedStar:
    return "--scratchpad_nested_star";
  }

  return "--scratchpad_unknown";
}

gpsdk::star_scratchpad::ClusterLayout getScratchpadClusterLayout(bool scratchpadStarCluster,
                                                                 bool scratchpadBlockCluster,
                                                                 bool scratchpadNestedStarCluster) {
  if (scratchpadNestedStarCluster) {
    return gpsdk::star_scratchpad::ClusterLayout::NestedStar;
  }
  return scratchpadBlockCluster ? gpsdk::star_scratchpad::ClusterLayout::Block
                                : gpsdk::star_scratchpad::ClusterLayout::Star;
}

gpsdk::star_scratchpad::ClusterSelection resolveScratchpadCluster(rt::IRuntime* runtime,
                                                                  const std::vector<rt::DeviceId>& devices,
                                                                  uint32_t deviceIdx,
                                                                  uint64_t requestedCenterMask,
                                                                  gpsdk::star_scratchpad::ClusterLayout layout) {
  const auto activeComputeShireMask = runtime->getDeviceProperties(devices.at(deviceIdx)).computeMinionShireMask_;
  const auto selection = gpsdk::star_scratchpad::selectCluster(requestedCenterMask, activeComputeShireMask, layout);
  if (selection.valid()) {
    return selection;
  }

  std::cout << "Invalid " << clusterOptionName(layout) << " configuration. Requested center mask 0x" << std::hex
            << requestedCenterMask << " is not compatible with active compute shire mask 0x" << activeComputeShireMask
            << std::dec << "." << std::endl;
  exit(1);
}
} // namespace

emu::SysEmuOptions getDefaultOptions(std::string const& simulator_params) {

  constexpr uint64_t kSysEmuMaxCycles = std::numeric_limits<uint64_t>::max();
  constexpr uint64_t kSysEmuMinionShiresMask = 0x1FFFFFFFFu;

  emu::SysEmuOptions sysEmuOptions;
  const auto etSdkHome = std::filesystem::path(getEtSdkHome());

  setIfExists(
    sysEmuOptions.bootromTrampolineToBL2ElfPath,
    etSdkHome / "lib/esperanto-fw/BootromTrampolineToBL2/BootromTrampolineToBL2.elf");
  setIfExists(
    sysEmuOptions.spBL2ElfPath,
    etSdkHome / "lib/esperanto-fw/ServiceProcessorBL2/fast-boot/ServiceProcessorBL2_fast-boot.elf");
  setIfExists(
    sysEmuOptions.machineMinionElfPath,
    etSdkHome / "lib/esperanto-fw/MachineMinion/MachineMinion.elf");
  setIfExists(
    sysEmuOptions.masterMinionElfPath,
    etSdkHome / "lib/esperanto-fw/MasterMinion/MasterMinion.elf");
  setIfExists(
    sysEmuOptions.workerMinionElfPath,
    etSdkHome / "lib/esperanto-fw/WorkerMinion/WorkerMinion.elf");
  setIfExists(sysEmuOptions.executablePath, etSdkHome / "bin/sys_emu");

  sysEmuOptions.runDir = std::filesystem::current_path();
  sysEmuOptions.maxCycles = kSysEmuMaxCycles;
  sysEmuOptions.minionShiresMask = kSysEmuMinionShiresMask;
  sysEmuOptions.puUart0Path = sysEmuOptions.runDir + "/pu_uart0_tx.log";
  sysEmuOptions.puUart1Path = sysEmuOptions.runDir + "/pu_uart1_tx.log";
  sysEmuOptions.spUart0Path = sysEmuOptions.runDir + "/spio_uart0_tx.log";
  sysEmuOptions.spUart1Path = sysEmuOptions.runDir + "/spio_uart1_tx.log";
  sysEmuOptions.startGdb = false;

  // Pass the sysemu parameters from command line
  auto cmd = simulator_params;
  std::istringstream iss{cmd};
  sysEmuOptions.additionalOptions =
    std::vector<std::string>{std::istream_iterator<std::string>{iss}, std::istream_iterator<std::string>{}};

  return sysEmuOptions;
}

std::vector<std::byte> GenericLauncher::readFile(const std::string& path) {
  auto file = std::ifstream(path, std::ios_base::binary);
  if (!file.is_open()) {
    std::cout << __func__ << "kernel file " << path << " not found\n";
    return {};
  }
  auto size = std::filesystem::file_size(path);
  std::vector<std::byte> fileContent(size);
  file.read(reinterpret_cast<char*>(fileContent.data()), size);
  return fileContent;
}

std::string GenericLauncher::getTopologyCachePath(uint32_t deviceIdx) const {
  if (!topologyCachePathOverride_.empty()) {
    return topologyCachePathOverride_.string();
  }

  if ((runtime_ == nullptr) || (deviceIdx >= devices_.size())) {
    return (defaultTopologyCacheDir() / ("device-" + std::to_string(deviceIdx) + ".topology")).string();
  }

  const auto props = runtime_->getDeviceProperties(devices_.at(deviceIdx));
  std::ostringstream oss;
  oss << "device-" << deviceIdx << "-mask-" << std::hex << props.computeMinionShireMask_ << "-spare-"
      << props.spareComputeMinionShireId_ << ".topology";
  return (defaultTopologyCacheDir() / oss.str()).string();
}

gpsdk::star_scratchpad::ClusterSelection GenericLauncher::resolveScratchpadClusterSelection(
  uint64_t requestedShireMask, gpsdk::star_scratchpad::ClusterLayout layout, uint32_t deviceIdx) {
  return resolveScratchpadClusterSelectionImpl(requestedShireMask, layout, deviceIdx);
}

const GenericLauncher::InferredTopology& GenericLauncher::getOrCreateInferredTopology(uint32_t deviceIdx) {
  auto found = inferredTopologies_.find(deviceIdx);
  if (found != inferredTopologies_.end()) {
    return *found->second;
  }

  auto topology = std::make_shared<InferredTopology>();
  const auto cachePath = std::filesystem::path(getTopologyCachePath(deviceIdx));
  if (!rebuildTopologyCache_ && loadTopologyCacheFile(cachePath, *topology)) {
    std::cout << "Loaded per-device topology cache from " << cachePath << ".\n";
    auto [it, _] = inferredTopologies_.emplace(deviceIdx, std::move(topology));
    return *it->second;
  }

  if (topologyProbeKernelPath_.empty()) {
    throw std::runtime_error("Nested scratchpad topology requires --topology_probe_kernel on first use for this device");
  }

  topology->activeComputeShireMask = runtime_->getDeviceProperties(devices_.at(deviceIdx)).computeMinionShireMask_;
  topology->cachePath = cachePath;

  const auto probeKernelId = loadKernel(topologyProbeKernelPath_.string(), deviceIdx);
  auto* const resultsBuffer =
    runtime_->mallocDevice(devices_.at(deviceIdx), sizeof(gpsdk::topology_probe::ShireLatencyResults));
  auto* const hostResults = new gpsdk::topology_probe::ShireLatencyResults{};

  const auto activeShires = activeShiresFromMask(topology->activeComputeShireMask);
  for (const auto centerShire : activeShires) {
    gpsdk::topology_probe::ProbeArguments args;
    args.targetShireMask = topology->activeComputeShireMask;
    args.resultsAddress = reinterpret_cast<uint64_t>(resultsBuffer);

    std::vector<std::byte> wrappedArgs;
    const std::byte* launchArgs = reinterpret_cast<const std::byte*>(&args);
    size_t launchArgSize = sizeof(args);

    if (activeNeighborhood_ >= 0) {
      gpsdk::launch::RuntimeArgsHeader header;
      header.flags = gpsdk::launch::kLaunchFlagSingleNeighborhoodPerShire;
      header.payloadSize = static_cast<uint32_t>(sizeof(args));
      header.activeNeighborhood = static_cast<uint8_t>(activeNeighborhood_);
      header.computeShireMask = (1ULL << centerShire);
      header.effectiveCenterShire = static_cast<uint8_t>(centerShire);
      wrappedArgs.resize(sizeof(header) + sizeof(args));
      memcpy(wrappedArgs.data(), &header, sizeof(header));
      memcpy(wrappedArgs.data() + sizeof(header), &args, sizeof(args));
      launchArgs = wrappedArgs.data();
      launchArgSize = wrappedArgs.size();
    }

    rt::KernelLaunchOptions kOpts;
    kOpts.setShireMask(1ULL << centerShire);
    kOpts.setBarrier(true);
    kOpts.setFlushL3(false);
    runtime_->kernelLaunch(defaultStreams_[deviceIdx], probeKernelId, launchArgs, launchArgSize, kOpts);
    waitKernelCompletion(std::chrono::seconds(30), deviceIdx);
    if (checkKernelExecutionErrors()) {
      delete hostResults;
      runtime_->freeDevice(devices_.at(deviceIdx), resultsBuffer);
      unLoadKernel(probeKernelId);
      throw std::runtime_error("Topology probe kernel failed");
    }

    runtime_->memcpyDeviceToHost(defaultStreams_[deviceIdx], resultsBuffer, reinterpret_cast<std::byte*>(hostResults),
                                 sizeof(*hostResults));
    if (!runtime_->waitForStream(defaultStreams_[deviceIdx], std::chrono::seconds(5))) {
      delete hostResults;
      runtime_->freeDevice(devices_.at(deviceIdx), resultsBuffer);
      unLoadKernel(probeKernelId);
      throw std::runtime_error("Timed out reading topology probe results");
    }

    if ((hostResults->magic != gpsdk::topology_probe::kResultsMagic) || (hostResults->centerShire != centerShire)) {
      delete hostResults;
      runtime_->freeDevice(devices_.at(deviceIdx), resultsBuffer);
      unLoadKernel(probeKernelId);
      throw std::runtime_error("Invalid topology probe results");
    }

    for (uint32_t target = 0U; target < gpsdk::topology_probe::kVisibleComputeShires; ++target) {
      topology->averageCycles[centerShire][target] = static_cast<uint16_t>(
        (hostResults->loadBestCycles[target] + hostResults->storeBestCycles[target]) / 2ULL);
    }
  }

  delete hostResults;
  runtime_->freeDevice(devices_.at(deviceIdx), resultsBuffer);
  unLoadKernel(probeKernelId);

  for (const auto centerShire : activeShires) {
    const auto selection = buildNestedSelection(*topology, centerShire);
    if (!selection.has_value()) {
      continue;
    }
    topology->nestedSelections[centerShire] = *selection;
    topology->hasNestedSelection[centerShire] = true;
  }

  saveTopologyCacheFile(cachePath, *topology);
  std::cout << "Built per-device topology cache at " << cachePath << ".\n";

  auto [it, _] = inferredTopologies_.emplace(deviceIdx, std::move(topology));
  return *it->second;
}

gpsdk::star_scratchpad::ClusterSelection GenericLauncher::resolveScratchpadClusterSelectionImpl(
  uint64_t requestedShireMask, gpsdk::star_scratchpad::ClusterLayout layout, uint32_t deviceIdx) {
  if (layout != gpsdk::star_scratchpad::ClusterLayout::NestedStar || (config_.mode_ != Mode::PCIE)) {
    return gpsdk::star_scratchpad::selectCluster(
      requestedShireMask, runtime_->getDeviceProperties(devices_.at(deviceIdx)).computeMinionShireMask_, layout);
  }

  gpsdk::star_scratchpad::ClusterSelection selection;
  if (__builtin_popcountll(requestedShireMask) != 1) {
    return selection;
  }

  const auto requestedCenter = static_cast<uint32_t>(__builtin_ctzll(requestedShireMask));
  const auto& topology = getOrCreateInferredTopology(deviceIdx);
  if (requestedCenter < topology.hasNestedSelection.size() && topology.hasNestedSelection[requestedCenter]) {
    return topology.nestedSelections[requestedCenter];
  }

  uint32_t bestCenter = gpsdk::star_scratchpad::kInvalidShire;
  uint16_t bestCost = std::numeric_limits<uint16_t>::max();
  for (uint32_t candidate = 0U; candidate < topology.hasNestedSelection.size(); ++candidate) {
    if (!topology.hasNestedSelection[candidate]) {
      continue;
    }
    const auto cost = topology.averageCycles[requestedCenter][candidate];
    if ((bestCenter == gpsdk::star_scratchpad::kInvalidShire) || (cost < bestCost) ||
        ((cost == bestCost) && (candidate < bestCenter))) {
      bestCenter = candidate;
      bestCost = cost;
    }
  }

  if (bestCenter == gpsdk::star_scratchpad::kInvalidShire) {
    return selection;
  }

  selection = topology.nestedSelections[bestCenter];
  selection.centerShifted = (bestCenter != requestedCenter);
  return selection;
}

void GenericLauncher::initialize() {

  auto options = rt::getDefaultOptions();
  std::unique_ptr<dev::IDeviceLayer> deviceLayer;
  
  if (std::filesystem::exists(runtimeSocketName_) && std::filesystem::is_socket(runtimeSocketName_) &&
      (config_.mode_ == Mode::PCIE)) {
    useRuntimeMultiProcess_ = true;
  }

  if (useRuntimeMultiProcess_ && ((config_.mode_ == Mode::SYSEMU) || (config_.mode_ == Mode::FAKE))) {
    std::cout << "Client not supported with this mode \n";
    exit(-1);
  }

  switch (config_.mode_) {
  case Mode::PCIE:
    std::cout << "Running tests with PCIE deviceLayer\n";
    if (!useRuntimeMultiProcess_) {
      deviceLayer = dev::IDeviceLayer::createPcieDeviceLayer();
    }
    break;
  case Mode::SYSEMU: {
    std::cout << "Running tests with SYSEMU deviceLayer\n";
    auto opts = getDefaultOptions(simulator_params_);
    std::vector<decltype(opts)> vopts;
    for (auto i = 0; i < config_.numDevices_; ++i) {
      vopts.emplace_back(opts);
      vopts.back().logFile += std::to_string(i);
    }
    deviceLayer = dev::IDeviceLayer::createSysEmuDeviceLayer(vopts);

    break;
  }
  case Mode::FAKE:
    std::cout << "Running tests with FAKE deviceLayer\n";
    deviceLayer = std::make_unique<dev::DeviceLayerFake>();
    options.checkDeviceApiVersion_ = false;

    break;
  case Mode::LAST:
    std::cout << "Unsupported device \n";
    exit(-1);
    break;
  }

  // Only creates the logger if IRuntime will be created by GenericLauncher
  LoggerLauncher logger;

  if (useRuntimeMultiProcess_) {
    runtimeOwned_ = rt::IRuntime::create(runtimeSocketName_);
  } else {
    runtimeOwned_ = rt::IRuntime::create(std::move(deviceLayer), options);
  }

  // get a raw-pointer
  runtime_ = runtimeOwned_.get();

  devices_ = runtime_->getDevices();

  for (auto i = 0U; i < devices_.size(); ++i) {
    defaultStreams_.emplace_back(runtime_->createStream(devices_[i]));
    traceStreams_.emplace_back(runtime_->createStream(devices_[i]));
    numDev_++;
  }

  // Program callbacks for error management.
  auto streamErrorHandler = [this, rt = getRuntime()]([[maybe_unused]] rt::EventId id, const rt::StreamError& error) {
    std::cout << "streamErrorHandler on deviceId[" << std::to_string((int)error.device_) << "] "
              << "() rt reports an error on a stream command(EventId: " << static_cast<int>(id) << "):\n"
              << error.getString();
    if (error.errorCode_ == rt::DeviceErrorCode::DmaHostAborted) {
      std::cout << std::to_string(error.errorCode_) << " Errors on DmaHost are expected, ignoring\n";
      return;
    }

    std::cout << "An Error has been detected" << std::endl;
    kernelError_++;
  };

  // Program callback when we want kernel aborts (due to a timeout) to dump corefiles
  auto abortedKernelHandler = [this, rt = getRuntime()](rt::EventId id, std::byte const* context, size_t size,
                                                        std::function<void()> freeResources) {
    std::cout << "abortedKernelHandler"
              << " () rt reports that a kernel has been aborted (EventId: " << static_cast<int>(id) << ")\n";
    kernelAbort_++;
    freeResources();
  };

  runtime_->setOnStreamErrorsCallback(streamErrorHandler);
  runtime_->setOnKernelAbortedErrorCallback(abortedKernelHandler);

  createUserTraces();
}

void GenericLauncher::writeSysemuTraceDumpCookie(void) {

  auto traceAddrPtrInfo = std::ofstream(sysemuTraceDumpCookiePath_, std::ios::binary | std::ios::out);

  if (traceAddrPtrInfo.good()) {

    traceAddrPtrInfo.write((char*)&numDev_, sizeof(uint32_t));

    for (uint16_t i = 0; i < traceDeviceBuffer_.size(); i++) {
      traceAddrPtrInfo.write((const char*)&traceDeviceBuffer_.at(i), sizeof(uint64_t));
      traceAddrPtrInfo.write((const char*)&kTraceBufferSize, sizeof(kTraceBufferSize));
    }
  } else {
    std::cout << "WARNING!!!, Could not write " << sysemuTraceDumpCookiePath_ << std::endl;
  }
}

void GenericLauncher::createUserTraces(void) {
  // Alloc space on device for user traces. Note: This buffer will be reused across differnet kernel launches.
  if (kernelTracesEnabled()) {

    for (uint32_t idx = 0; idx < numDev_; idx++) {
      std::byte* addrptr = runtime_->mallocDevice(devices_[idx], kTraceBufferSize);
      traceDeviceBuffer_.emplace_back(addrptr);
    }

    if (config_.mode_ == Mode::SYSEMU) {
      writeSysemuTraceDumpCookie();
    }
  }
}

void GenericLauncher::initialize(rt::IRuntime* runtime) {

  if (runtimeParams_) {
    std::cout << "Error: Some command-line parameters are not allowed when runtime instance is provisioned externally."
              << std::endl;
  }

  runtime_ = runtime;
  devices_ = runtime_->getDevices();

  for (auto i = 0U; i < devices_.size(); ++i) {
    defaultStreams_.emplace_back(runtime_->createStream(devices_[i]));
    traceStreams_.emplace_back(runtime_->createStream(devices_[i]));
    numDev_++;
  }

  createUserTraces();
}

void GenericLauncher::unLoadKernel(rt::KernelId kernelId) {
  runtime_->unloadCode(kernelId);
}

void GenericLauncher::removeSysemuTraceDumpCookie(void) {

  std::error_code ec;

  std::filesystem::remove(sysemuTraceDumpCookiePath_, ec);

  if (ec) { // Error on remove
    std::cout << "WARNING!!!, Could not remove " << sysemuTraceDumpCookiePath_ << ", error was: " << ec.value() << " "
              << ec.message() << std::endl;
  }
}

void GenericLauncher::tearDown() {

  if (kernelTracesEnabled()) {
    for (uint32_t deviceIdx = 0; deviceIdx < numDev_; deviceIdx++) {
      runtime_->freeDevice(devices_[deviceIdx], traceDeviceBuffer_[deviceIdx]);
    }
  }

  auto timeout = std::chrono::seconds(1);
  for (auto s : defaultStreams_) {
    auto success = runtime_->waitForStream(s, timeout);
    if (!success) {
      std::cout << __func__ << "() default stream " << uint32_t(s) << " wait timeout\n";
    }
    runtime_->destroyStream(s);
  }
  for (auto s : traceStreams_) {
    auto success = runtime_->waitForStream(s, timeout);
    if (!success) {
      std::cout << __func__ << "() traces stream " << uint32_t(s) << " wait timeout\n";
    }
    runtime_->destroyStream(s);
  }

  resetRuntime();
  defaultStreams_.clear();
  traceStreams_.clear();
  devices_.clear();
}

rt::KernelId GenericLauncher::loadKernel(const std::string& kernelName, uint32_t deviceIdx) {
  const bool debugLoadPhases = debugLoadPhasesEnabled();
  const bool skipLoadWait = skipLoadWaitEnabled();
  const auto resolvedKernelPath = resolveKernelPath(kernelName, config_.mode_);
  if (scratchpadNestedStarCluster_ && (config_.mode_ == Mode::PCIE) &&
      (topologyProbeKernelPath_.empty() || (resolvedKernelPath != topologyProbeKernelPath_))) {
    (void)getOrCreateInferredTopology(deviceIdx);
  }
  if (debugLoadPhases) {
    std::cout << "loadKernel() resolved kernel path " << resolvedKernelPath << "\n";
  }
  auto kernelContent = readFile(resolvedKernelPath.string());
  if (kernelContent.empty()) {
    exit(-1);
  }
  assert(devices_.size() > deviceIdx);
  auto st = defaultStreams_[deviceIdx];
  std::optional<rt::StreamId> tempLoadStream;
  if (skipLoadWait) {
    tempLoadStream = runtime_->createStream(devices_[deviceIdx]);
    st = *tempLoadStream;
  }
  if (debugLoadPhases) {
    std::cout << "loadKernel() invoking runtime_->loadCode on stream " << int(st) << "\n";
  }
  auto res = runtime_->loadCode(st, kernelContent.data(), kernelContent.size());
  if (debugLoadPhases) {
    std::cout << "loadKernel() loadCode returned kernel " << int(res.kernel_) << " event " << int(res.event_)
              << " addr " << std::hex << res.loadAddress_ << std::dec << "\n";
  }
  if (skipLoadWait) {
    const auto delay = getLoadQuiesceDelay();
    std::cout << "loadKernel() skipping load wait for event " << int(res.event_) << ", quiescing for "
              << delay.count() << " ms\n";
    std::this_thread::sleep_for(delay);
  } else {
    runtime_->waitForEvent(res.event_);
  }
  if (tempLoadStream.has_value()) {
    runtime_->destroyStream(*tempLoadStream);
  }
  std::cout << __func__ << "() kernel " << int(res.kernel_) << " loaded at " << std::hex << res.loadAddress_ << "\n";

  return res.kernel_;
}

// TODO: make it configuraion-aware.
std::tuple<uint64_t, uint64_t> getTraceMinions() {
  // all (shireMask: threadMask)
  return {0x1FFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL};
}

constexpr uint64_t getTraceThreadMask() {
  return 0xFFFFFFFFFFFFFFFFULL;
}

void GenericLauncher::dumpTracesToFile(uint64_t fileIdx, rt::KernelId kernelId, uint32_t deviceIdx) {
  if (!kernelTracesEnabled()) {
    return;
  }
  // geting device traces.
  std::vector<std::byte> deviceTrace(kTraceBufferSize);
  runtime_->memcpyDeviceToHost(traceStreams_[deviceIdx], traceDeviceBuffer_[deviceIdx], deviceTrace.data(),
                               deviceTrace.size());
  auto tracesTimeout = std::chrono::seconds(10);
  auto success = runtime_->waitForStream(traceStreams_[deviceIdx], tracesTimeout);

  if (!success) {
    std::cout << __func__ << "() timeout extracting traces from device\n";
    return;
  }

  // serialize traces to disk
  std::string traceSuffix = "";
  if (int(kernelId) != -1) {
    traceSuffix = "_" + std::to_string(int(kernelId));
  }

  auto tracePath =
    std::filesystem::current_path() / std::filesystem::path("traceKernels_dev" + std::to_string(devIdx_) + "_" +
                                                            std::to_string(fileIdx) + traceSuffix + ".bin");
  auto traceStream = std::ofstream(tracePath, std::ios::binary | std::ios::out);
  traceStream.write((char*)deviceTrace.data(), deviceTrace.size());
}

void GenericLauncher::waitKernelCompletion(std::chrono::seconds timeout, uint32_t deviceIdx) {

  auto success = runtime_->waitForStream(defaultStreams_[deviceIdx], timeout);

  if (success) {
    return;
  }
  kernelTimeout_++;
  // Kernel did not complete on the expected time. let's abort the stream in which
  // the kernel is running.
  std::cout << "[TIMEOUT] " << __func__ << "() Wait for Stream command exceeded " << std::dec << int(timeout.count())
            << " seconds.  Aborting stream\n";
  
  auto event = runtime_->abortStream(defaultStreams_[deviceIdx]);
  auto abortTimeout = std::chrono::seconds(10);
  success = runtime_->waitForEvent(event, abortTimeout);
  if (success) {
    std::cout << "[TIMEOUT] " << __func__ << "() event completed succesfuly: " << (int)event << "\n";
    return;
  }
  std::cout << "[TIMEOUT] " << __func__ << "() timeout expired wating for abortStream event: " << (int)event
            << " to complete\n";
  return;
}

uint64_t GenericLauncher::getLaunchShireMask(uint64_t requestedShireMask, uint32_t deviceIdx) {
  if (!scratchpadStarCluster_ && !scratchpadBlockCluster_ && !scratchpadNestedStarCluster_) {
    return requestedShireMask;
  }

  const auto layout =
    getScratchpadClusterLayout(scratchpadStarCluster_, scratchpadBlockCluster_, scratchpadNestedStarCluster_);
  return resolveScratchpadClusterSelectionImpl(requestedShireMask, layout, deviceIdx).launchedShireMask;
}

void GenericLauncher::doKernelLaunch(rt::KernelId kernelId, std::byte* params, size_t size, std::byte* ptr,
                                     size_t stackSize, uint64_t shireMask, uint32_t deviceIdx) {
  rt::KernelLaunchOptions kOpts;
  std::string coreFileName;
  std::filesystem::path cwd;
  std::vector<std::byte> wrappedParams;
  const bool usesScratchpadCluster = scratchpadStarCluster_ || scratchpadBlockCluster_ || scratchpadNestedStarCluster_;
  const auto launchShireMask = getLaunchShireMask(shireMask, deviceIdx);
  const std::byte* launchParams = params;
  size_t launchParamsSize = size;
  uint64_t computeShireMask = shireMask;
  bool clusterCenterShifted = false;

  if (usesScratchpadCluster) {
    const auto layout =
      getScratchpadClusterLayout(scratchpadStarCluster_, scratchpadBlockCluster_, scratchpadNestedStarCluster_);
    const auto selection = resolveScratchpadClusterSelectionImpl(shireMask, layout, deviceIdx);
    computeShireMask = selection.computeShireMask;
    clusterCenterShifted = selection.centerShifted;
    if (clusterCenterShifted) {
      std::cout << clusterOptionName(layout) << " shifted requested center shire "
                << static_cast<uint32_t>(__builtin_ctzll(shireMask)) << " to active center shire "
                << selection.effectiveCenterShire << ".\n";
    }
    exportScratchpadAddressMapFile(scratchpadAddressMapPath_, shireMask, layout, selection);
  }

  if (enableCoreDump_) {
    coreFileName = "core." + std::to_string(getpid()) + ".etsoc." + std::to_string((int)kernelId) + "." +
                   std::to_string((int)deviceIdx);
    cwd = std::filesystem::current_path() / coreFileName;
  }
  kOpts.setShireMask(launchShireMask);
  kOpts.setBarrier(true);
  kOpts.setFlushL3(false);
  kOpts.setCoreDumpFilePath(cwd.string());
  if (kernelTracesEnabled()) {
    kOpts.setUserTracing(reinterpret_cast<uint64_t>(traceDeviceBuffer_[deviceIdx]), kTraceBufferSize, 0, computeShireMask,
                         getTraceThreadMask(), TRACE_EVENT_ENABLE_ALL, TRACE_FILTER_ENABLE_ALL);
  }
  if ((ptr != nullptr) && (stackSize != 0)) {
    kOpts.setStackConfig(ptr, stackSize);
  }

  if ((activeNeighborhood_ >= 0) || usesScratchpadCluster) {
    gpsdk::launch::RuntimeArgsHeader header;
    header.computeShireMask = computeShireMask;
    if (activeNeighborhood_ >= 0) {
      header.flags |= gpsdk::launch::kLaunchFlagSingleNeighborhoodPerShire;
      header.activeNeighborhood = static_cast<uint8_t>(activeNeighborhood_);
    }
    if (scratchpadStarCluster_) {
      header.flags |= gpsdk::launch::kLaunchFlagScratchpadStarCluster;
    }
    if (scratchpadBlockCluster_) {
      header.flags |= gpsdk::launch::kLaunchFlagScratchpadBlockCluster;
    }
    if (scratchpadNestedStarCluster_) {
      header.flags |= gpsdk::launch::kLaunchFlagScratchpadNestedStarCluster;
    }
    if (erbiumSim_) {
      header.flags |= gpsdk::launch::kLaunchFlagErbiumSim;
    }
    if (usesScratchpadCluster) {
      const auto layout =
        getScratchpadClusterLayout(scratchpadStarCluster_, scratchpadBlockCluster_, scratchpadNestedStarCluster_);
      const auto selection = resolveScratchpadClusterSelectionImpl(shireMask, layout, deviceIdx);
      header.effectiveCenterShire = static_cast<uint8_t>(selection.effectiveCenterShire);
      header.scratchpadRelayCount = selection.relayCount;
      header.scratchpadAuxiliaryCount = selection.auxiliaryCount;
      std::copy(selection.relayShires.begin(), selection.relayShires.end(), header.scratchpadRelayShires);
      std::copy(selection.auxiliaryShires.begin(), selection.auxiliaryShires.end(), header.scratchpadAuxiliaryShires);
    }
    header.payloadSize = static_cast<uint32_t>(size);

    wrappedParams.resize(sizeof(header) + size);
    memcpy(wrappedParams.data(), &header, sizeof(header));
    if (size != 0) {
      memcpy(wrappedParams.data() + sizeof(header), params, size);
    }

    launchParams = wrappedParams.data();
    launchParamsSize = wrappedParams.size();
  }

  runtime_->kernelLaunch(defaultStreams_[deviceIdx], kernelId, launchParams, launchParamsSize, kOpts);
}

void GenericLauncher::resetRuntime() {
  if (runtimeOwned_) {
    runtimeOwned_.reset();
  }
}

// Passes pointer to runtime instance without core dump capabilities
// to abortedKernelHandler callback.
// Runtime is used inside the callback to copy error context
// and dump core from device to host.
rt::IRuntime* GenericLauncher::getRuntime() {

  return runtime_;
}

void GenericLauncher::parse_args(int argc, char** argv, bool strict) {

  static const std::vector<struct option> long_opts_vect{{"enableCoreDump", no_argument, nullptr, 0},
                                                         {"useRuntimeMultiProcess", no_argument, nullptr, 0},
                                                         {"runtimeSocket", required_argument, nullptr, 0},
                                                         {"simulator_params", required_argument, nullptr, 0},
                                                         {"active_neighborhood", required_argument, nullptr, 0},
                                                         {"scratchpad_star", no_argument, nullptr, 0},
                                                         {"scratchpad_block", no_argument, nullptr, 0},
                                                         {"erbium_sim", no_argument, nullptr, 0},
                                                         {"scratchpad_address_map", required_argument, nullptr, 0},
                                                         {"topology_probe_kernel", required_argument, nullptr, 0},
                                                         {"topology_cache", required_argument, nullptr, 0},
                                                         {"rebuild_topology_cache", no_argument, nullptr, 0},
                                                         {"scratchpad_nested_star", no_argument, nullptr, 0},
                                                         {nullptr, 0, nullptr, 0}};

  int ret = 0;
  int index = 0;
  opterr = 0;

  /*
    A program that scans multiple argument vectors, or rescans the same vector more than once,
    and wants to make use of GNU extensions such as '+' and '-' at the start of optstring,
    or changes the value of POSIXLY_CORRECT between scans, must reinitialize getopt() by
    resetting optind to 0, rather than the traditional value of 1. (Resetting to 0 forces
    the invocation of an internal initialization routine that rechecks POSIXLY_CORRECT
    and checks for GNU extensions in optstring.)
  */

  optind = 0;

  while ((ret = getopt_long(argc, argv, "", long_opts_vect.data(), &index)) != -1) {
    if (ret == '?') {
      if (strict) {
        std::cout << "This option parameter is not expected: " << argv[optind - 1] << std::endl;
        exit(1);
      } else {
        continue;
      }
    }

    const char* const name = long_opts_vect.data()[index].name;

    if (!strcmp(name, "simulator_params")) {
      simulator_params_ = optarg;
      runtimeParams_ = true;
    } else if (!strcmp(name, "enableCoreDump")) {
      enableCoreDump_ = true;
      runtimeParams_ = true;
    } else if (!strcmp(name, "useRuntimeMultiProcess")) {
      useRuntimeMultiProcess_ = true;
    } else if (!strcmp(name, "runtimeSocket")) {
      runtimeSocketName_ = optarg;
    } else if (!strcmp(name, "active_neighborhood")) {
      activeNeighborhood_ = std::stoi(optarg, nullptr, 0);
      if (!gpsdk::launch::isValidNeighborhood(static_cast<uint32_t>(activeNeighborhood_))) {
        std::cout << "Invalid --active_neighborhood value " << activeNeighborhood_
                  << ". Expected a value in the range [0, 3]." << std::endl;
        exit(1);
      }
    } else if (!strcmp(name, "scratchpad_star")) {
      scratchpadStarCluster_ = true;
    } else if (!strcmp(name, "scratchpad_block")) {
      scratchpadBlockCluster_ = true;
    } else if (!strcmp(name, "erbium_sim")) {
      erbiumSim_ = true;
    } else if (!strcmp(name, "scratchpad_address_map")) {
      scratchpadAddressMapPath_ = optarg;
    } else if (!strcmp(name, "topology_probe_kernel")) {
      topologyProbeKernelPath_ = optarg;
    } else if (!strcmp(name, "topology_cache")) {
      topologyCachePathOverride_ = optarg;
    } else if (!strcmp(name, "rebuild_topology_cache")) {
      rebuildTopologyCache_ = true;
    } else if (!strcmp(name, "scratchpad_nested_star")) {
      scratchpadNestedStarCluster_ = true;
    }
  }

  const auto scratchpadClusterModes =
    static_cast<uint32_t>(scratchpadStarCluster_) + static_cast<uint32_t>(scratchpadBlockCluster_) +
    static_cast<uint32_t>(scratchpadNestedStarCluster_);
  if (scratchpadClusterModes > 1U) {
    std::cout << "--scratchpad_star, --scratchpad_block, and --scratchpad_nested_star are mutually exclusive."
              << std::endl;
    exit(1);
  }

  if (erbiumSim_ && (scratchpadClusterModes == 0U)) {
    std::cout << "--erbium_sim requires one of --scratchpad_star, --scratchpad_block, or --scratchpad_nested_star."
              << std::endl;
    exit(1);
  }

  /* It needs to do again because on invoke sysemu if is the case, It calls getopts again */
  optind = 0;
}

bool GenericLauncher::checkKernelExecutionErrors() {
  // everithing ok!
  if(!kernelError_ && !kernelAbort_ && !kernelTimeout_) {
    return false;
  }
 
  // if the kernel ended in timeout and we have configured a core-dump retrieval, 
  // The core extraction process is asynchronous. we just allow for some safety margin
  // for the process to complete before calling destructors.
  if(kernelTimeout_ && enableCoreDump_) {
    sleep(3);
  }
  return true;
}

std::tuple<std::byte*, size_t> GenericLauncher::allocDeviceStack(size_t threadStackSize, uint64_t shireMask) {
  constexpr size_t kNumThreadsPerShire = 64;
  const auto launchShireMask = getLaunchShireMask(shireMask, devIdx_);
  size_t totalStackSize = __builtin_popcountll(launchShireMask) * kNumThreadsPerShire * threadStackSize;
  std::byte* ptrStack = runtime_->mallocDevice(devices_[devIdx_], totalStackSize, 4096);
  return make_tuple(ptrStack, totalStackSize);
}

void GenericLauncher::freeDeviceStack(std::byte* ptrStack) {
  runtime_->freeDevice(devices_[devIdx_], ptrStack);
}
