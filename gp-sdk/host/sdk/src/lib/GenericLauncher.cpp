//******************************************************************************
// Copyright (c) 2025 Ainekko, Co.
// SPDX-License-Identifier: Apache-2.0
//------------------------------------------------------------------------------

#include <cassert>
#include <esperanto/et-trace/encoder.h>
#include <fstream>
#include <iostream>
#include <iterator>
#include <cstring>
#include <runtime/DeviceLayerFake.h>
#include <runtime/Types.h>
#include <sw-sysemu/SysEmuOptions.h>

#include <chrono>
#include <getopt.h>
#include <stdio.h>
#include <thread>
#include <tuple>
#include <vector>

#include "GenericLauncher.h"

// Trace Buffer realted constants.
constexpr size_t kTraceBytesPerHart = 4096;
constexpr size_t kNumHarts = 2048;
constexpr size_t kTraceBufferSize = kTraceBytesPerHart * kNumHarts;

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

void setIfExists(std::string& dst, const std::filesystem::path& path) {
  if (std::filesystem::exists(path)) {
    dst = path.string();
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

constexpr uint32_t kScratchpadStarRows = 4U;
constexpr uint32_t kScratchpadStarCols = 8U;

uint32_t getScratchpadCenterShire(uint64_t centerShireMask, const char* optionName) {
  if (__builtin_popcountll(centerShireMask) != 1) {
    std::cout << "Invalid " << optionName << " configuration. --shire_mask must select exactly one center shire."
              << std::endl;
    exit(1);
  }

  const auto centerShire = static_cast<uint32_t>(__builtin_ctzll(centerShireMask));
  if (centerShire >= (kScratchpadStarRows * kScratchpadStarCols)) {
    std::cout << "Invalid " << optionName << " center shire " << centerShire
              << ". Expected a compute shire in the range [0, 31]." << std::endl;
    exit(1);
  }

  // Assume the 32 compute shires are numbered row-major on a 4x8 mesh.
  const auto row = centerShire / kScratchpadStarCols;
  const auto col = centerShire % kScratchpadStarCols;
  if ((row == 0U) || (row == (kScratchpadStarRows - 1U)) || (col == 0U) || (col == (kScratchpadStarCols - 1U))) {
    std::cout << "Invalid " << optionName << " center shire " << centerShire
              << ". The center must not be on the edge of the 4x8 compute-shire mesh." << std::endl;
    exit(1);
  }

  return centerShire;
}

uint64_t expandScratchpadStarCluster(uint64_t centerShireMask) {
  const auto centerShire = getScratchpadCenterShire(centerShireMask, "--scratchpad_star");

  return centerShireMask | (1ULL << (centerShire - 1U)) | (1ULL << (centerShire + 1U)) |
         (1ULL << (centerShire - kScratchpadStarCols)) | (1ULL << (centerShire + kScratchpadStarCols));
}

uint64_t expandScratchpadBlockCluster(uint64_t centerShireMask) {
  const auto centerShire = getScratchpadCenterShire(centerShireMask, "--scratchpad_block");

  return centerShireMask | (1ULL << (centerShire - kScratchpadStarCols - 1U)) |
         (1ULL << (centerShire - kScratchpadStarCols)) | (1ULL << (centerShire - kScratchpadStarCols + 1U)) |
         (1ULL << (centerShire - 1U)) | (1ULL << (centerShire + 1U)) |
         (1ULL << (centerShire + kScratchpadStarCols - 1U)) | (1ULL << (centerShire + kScratchpadStarCols)) |
         (1ULL << (centerShire + kScratchpadStarCols + 1U));
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

uint64_t GenericLauncher::getLaunchShireMask(uint64_t requestedShireMask) const {
  if (scratchpadBlockCluster_) {
    return expandScratchpadBlockCluster(requestedShireMask);
  }
  return scratchpadStarCluster_ ? expandScratchpadStarCluster(requestedShireMask) : requestedShireMask;
}

void GenericLauncher::doKernelLaunch(rt::KernelId kernelId, std::byte* params, size_t size, std::byte* ptr,
                                     size_t stackSize, uint64_t shireMask, uint32_t deviceIdx) {
  rt::KernelLaunchOptions kOpts;
  std::string coreFileName;
  std::filesystem::path cwd;
  std::vector<std::byte> wrappedParams;
  const auto launchShireMask = getLaunchShireMask(shireMask);
  const std::byte* launchParams = params;
  size_t launchParamsSize = size;

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
    kOpts.setUserTracing(reinterpret_cast<uint64_t>(traceDeviceBuffer_[deviceIdx]), kTraceBufferSize, 0, shireMask,
                         getTraceThreadMask(), TRACE_EVENT_ENABLE_ALL, TRACE_FILTER_ENABLE_ALL);
  }
  if ((ptr != nullptr) && (stackSize != 0)) {
    kOpts.setStackConfig(ptr, stackSize);
  }

  if ((activeNeighborhood_ >= 0) || scratchpadStarCluster_ || scratchpadBlockCluster_) {
    gpsdk::launch::RuntimeArgsHeader header;
    header.computeShireMask = shireMask;
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
    }
  }

  if (scratchpadStarCluster_ && scratchpadBlockCluster_) {
    std::cout << "--scratchpad_star and --scratchpad_block are mutually exclusive." << std::endl;
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
  const auto launchShireMask = getLaunchShireMask(shireMask);
  size_t totalStackSize = __builtin_popcountll(launchShireMask) * kNumThreadsPerShire * threadStackSize;
  std::byte* ptrStack = runtime_->mallocDevice(devices_[devIdx_], totalStackSize, 4096);
  return make_tuple(ptrStack, totalStackSize);
}

void GenericLauncher::freeDeviceStack(std::byte* ptrStack) {
  runtime_->freeDevice(devices_[devIdx_], ptrStack);
}
