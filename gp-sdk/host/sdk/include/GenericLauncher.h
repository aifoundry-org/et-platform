//******************************************************************************
// Copyright (c) 2025 Ainekko, Co.
// SPDX-License-Identifier: Apache-2.0
//------------------------------------------------------------------------------
#ifndef GENERIC_LAUNCHER_H
#define GENERIC_LAUNCHER_H
#include <device-layer/IDeviceLayer.h>
#include <runtime/IRuntime.h>

#include <hostUtils/logging/Logger.h>

#include <string>
#include <tuple>
#include <unistd.h>
#include <vector>

#include "gpsdk_launch_runtime.h"

#if __has_include("filesystem")
#include <filesystem>
#elif __has_include("experimental/filesystem")
#include <experimental/filesystem>
namespace std {
namespace filesystem = std::experimental::filesystem;
}
#endif
namespace fs = std::filesystem;

/**
 * \enum Mode
 * \brief list of execution mode types.
 **/
enum class Mode {
  PCIE,   /*!< Launch kernel in the PCIE connected accelerator. */
  SYSEMU, /*!< Launch kernel in the SYSEMU simulator. */
  FAKE,   /*!< Launch kernel in a FAKE device (dummy loopback) that does nothing. Used for testing purposes. */
  LAST
};

static inline std::string to_string(Mode m) {
  const std::vector<std::string> modes = {"silicon", "sysemu", "fake"};
  return m >= Mode::LAST ? "unknown" : modes[int(m)];
}

static inline Mode modeFromString(std::string m) {
  std::map<std::string, Mode> modeFromStr = {{"sysemu", Mode::SYSEMU}, {"fake", Mode::FAKE}, {"silicon", Mode::PCIE}};
  return modeFromStr.count(m) ? modeFromStr[m] : Mode::LAST;
}

/**
 * Structure to configure the type and number of devices on which the kernel will be launched.
 * \brief Configures type and number of devices.
 */
struct Config {
  void dump() const {
    std::cout << "Selected options:\n";
    std::cout << "  Device type: " << to_string(mode_) << "\n";
    std::cout << "  Device count: " << numDevices_ << "\n";
  }

  Mode mode_{Mode::SYSEMU};
  size_t numDevices_{1};
};

/**
 * Creates a LoggerLauncher object, (It needs to be created before creating the runtime instance).
 * instance. \brief LoggerLauncher class to enable the logger.
 */
class LoggerLauncher {
public:
  LoggerLauncher() {
  }
  // just to enable glog-logger on runtime.
  logging::LoggerDefault loggerDefault_;
};

/**
 * Creates a GenericLauncher object, provides methods to load and execute kernels in ETSoC-1 devices.
 * Kernels are compiled RISC-V ETSoC-1 compatible binary files in elf format
 * \brief GenericLauncher class to load and run kernels in a device.
 */
class GenericLauncher {
public:
  GenericLauncher() = delete;
  /**
   * Creates a new GenericLauncher object with a config.
   * \brief Main constructor.
   * \param config specifies device type and device count configuration used in the kernel execution
   * \param argc number of cmd-line arguments to parse
   * \param argv pointer to arguments
   * \param strictArs (defaults to true) if true, unknown parameters error the app, so it
   * assumes we only receive the parameters of interest.  if false, the check is relaxed (This would allow reparsing
   * previously parsed command-lines for example).
   */
  GenericLauncher(const Config& config, int argc, char** argv, bool strictArgs = true)
    : config_(config) {

    parse_args(argc, argv, strictArgs);
  };

  ~GenericLauncher() {
    if (config_.mode_ == Mode::SYSEMU) {
      removeSysemuTraceDumpCookie();
    }
  }

  /**
   * Initializes the PCIE, SYSEMU or FAKE device interface where the kernel will run.
   * \brief Initializes the execution device.
   */
  void initialize(); // setup

  /**
   * Initializes device interface by using an externally provided runtime instance.
   * \brief Initializes the execution device over a given IRuntime instance.
   * \param runtime pointer to Iruntime instance.
   */
  void initialize(rt::IRuntime* runtime); // setup

  /**
   * Destroys the launcher.
   * \brief Destroys the launcher.
   */
  void tearDown();

  /**
   * Dumps the selected kernel event trace into a formatted text file.
   * \brief Dumps a kernel trace
   * \param fileIdx numeric index appended to the trace file name
   * \param KernelId id of the kernel to dump
   * \param deviceIdx device target to work with
   */
  void dumpTracesToFile(uint64_t fileIdx = 0, rt::KernelId kernelId = (rt::KernelId)(-1), uint32_t deviceIdx = 0);

  /**
   * Starts the execution of the loaded kernel on the device. Host and device code execute asynchronously until
   * waitKernelCompletion() is called.
   * \brief Launches the kernel on the device. \param kernelId id of the kernel to
   * launch.
   * \param kernelId id of the kernel to launch.
   * \param params launch parameters.
   * \param stackPtr point to stack base address has to be aligned to 4096.
   * \param stackSize total size of stack used by all hearts. The base stack size has to be aligned to 4096.
   * \param deviceIdx device target to work with
   * \param shireMask mask with the shires that will execute the kernel.
   */
  template <typename TParams>
  void kernelLaunch(rt::KernelId kernelId, TParams* params, std::byte* stackPtr = nullptr, size_t stackSize = 0,
                    uint32_t deviceIdx = 0, uint64_t shireMask = 0xffffffff) {
    doKernelLaunch(kernelId, (std::byte*)params, sizeof(TParams), stackPtr, stackSize, shireMask, deviceIdx);
  }

  /**
   * Starts the execution of the loaded kernel on the device. Host and device code execute asynchronously until
   * waitKernelCompletion() is called.
   * \brief Launches the kernel on the device without providing parameters.
   * \param kernelId id of the kernel to launch.
   * \param deviceIdx device target to work with
   * \param shireMask mask with the shires that will execute the kernel.*
   */
  void kernelLaunch(rt::KernelId kernelId, uint32_t deviceIdx = 0, uint64_t shireMask = 0xffffffff) {
    doKernelLaunch(kernelId, nullptr, 0, nullptr, 0, shireMask, deviceIdx);
  }

  /**
   * Blocks host side progress until the kernel on the device completes its execution.
   * If the device kernel does not complete in the allocated \p timeout seconds, it will be aborted raising an error.
   * \brief Waits for device kernel completion.
   * \param timeout max number of seconds to wait before timing out
   * \param deviceIdx device target to work with
   */
  void waitKernelCompletion(std::chrono::seconds timeout, uint32_t deviceIdx = 0);

  /**
   * Loads an ETSoC-1 RISC-V binary in ELF format located at \p kernelName filepath.
   * \brief Loads a kernel on the device.
   * \param kernelName path to the kernel file (elf format) to load
   * \param deviceIdx device target to work with
   * \return returns a kernelId
   */
  rt::KernelId loadKernel(const std::string& kernelName, uint32_t deviceIdx = 0);

  /**
   * Unloads the kernel specified by \p kernelId from the device.
   * \brief Unloads a kernel from the device.
   * \param kernelId id of the kernel to unload
   */
  void unLoadKernel(rt::KernelId kernelId);

  /**
   * Get the number of PCIE devices detected
   * \brief It has real value after initialize call
   * \return numDev_
   */
  uint32_t getNumDevices(void) {
    return numDev_;
  }

  /**
   * Helper function to check wether some abnormal situation happened.
   * Note, it may introduce a small delay in case coredump retrieval is expected to happen asynchronously.
   */ 
  bool checkKernelExecutionErrors();

  /**
   * Reserve all necessary stack space at device.
   * \brief reserve stack space and caculates the total stack size reserved.
   * \param threadStacksize stack size per hart.
   * \param shireMask mask with the shires that will execute the kernel.
   * \return A tuple with stackPtr pointer to base address reserved and totalSize with total stack size reserved.
   */
  std::tuple<std::byte*, size_t> allocDeviceStack(size_t threadStackSize, uint64_t shireMask);

  /**
   * Free memory reserved for stack.
   * \brief free stack space reserved.
   * \param ptrStack pointer to stack base address
   */
  void freeDeviceStack(std::byte* ptrStack);

  std::atomic<uint64_t> kernelError_ = 0; // Number of kernels that reported an error
  std::atomic<uint64_t> kernelAbort_ = 0; // Number of kernels aborted
  std::atomic<uint64_t> kernelTimeout_ = 0; // Number of kernels that ended in timeout

  
  constexpr static const char* help_msg =
    "  '', --enableCoreDump          enable core dump generation on device failures\n"
    "  '', --useRuntimeMultiProcess  Server/Client mode will be use by app running it as a client.\n"
    "  '', --runtimeSocket           Socket filename to be use.\n"
    "  '', --simulator_params        Hyperparameters to pass to simulator, overrides default values\n"
    "  '', --active_neighborhood     Restrict execution to one neighborhood per shire (0-3).\n"
    "  '', --scratchpad_star         Expand a single center shire into a center+N/S/E/W scratchpad cluster.\n";

private:
  std::vector<std::byte> readFile(const std::string& path);
  uint64_t getLaunchShireMask(uint64_t requestedShireMask) const;
  void doKernelLaunch(rt::KernelId, std::byte* params, size_t size, std::byte* stackPtr, size_t stackSize,
                      uint64_t shireMask, uint32_t deviceIdx);
  void resetRuntime();
  void createUserTraces(void);
  void writeSysemuTraceDumpCookie(void);
  void removeSysemuTraceDumpCookie(void);
  rt::IRuntime* getRuntime();
  bool enableCoreDump_ = false;
  void parse_args(int argc, char* argv[], bool strict);

  // parameters expected
  fs::path gp_sdk_device_installdir_;
  std::string simulator_params_;
  bool useRuntimeMultiProcess_ = false;
  int activeNeighborhood_ = -1;
  bool scratchpadStarCluster_ = false;
  std::string sysemuTraceDumpCookiePath_ =
    std::filesystem::path(std::filesystem::temp_directory_path().string() + "/" + "sysemuTraceDumpCookie." +
                          std::to_string(getuid()) + "." + std::to_string(getpid()) + ".bin");

protected:
  const Config& config_;
  rt::IRuntime* runtime_;
  rt::RuntimePtr runtimeOwned_ = nullptr;
  rt::RuntimePtr runtimeBase_;
  std::vector<rt::DeviceId> devices_;
  std::vector<rt::StreamId> defaultStreams_;
  std::vector<rt::StreamId> traceStreams_;
  std::vector<std::byte*> traceDeviceBuffer_;
  std::string runtimeSocketName_ = "/var/run/et_runtime/pcie.sock";
  uint32_t numDev_ = 0;
  size_t devIdx_ = 0;
  bool runtimeParams_ = false;
};

#endif // GENERIC_LAUNCHER_H
