/*-------------------------------------------------------------------------
 * Copyright (C) 2022, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/

#include "Server.h"
#include "runtime/DeviceLayerFake.h"
#include "runtime/IProfiler.h"
#include <condition_variable>
#include <csignal>
#include <fstream>
#include <gflags/gflags.h>
#include <hostUtils/logging/Logger.h>
#include <ios>
#include <sw-sysemu/SysEmuOptions.h>
#include <sys/stat.h>
#include <unistd.h>

std::atomic<bool> s_running = true;
std::mutex s_m;
std::condition_variable s_cv;
using namespace std::literals;
namespace {

void signalHandler(int signum) {
  std::cout << "Interrupt signal (" << signum << ") received.\n";
  s_running = false;
  s_cv.notify_all();
}

bool validateDeviceType(const char* flagName, const std::string& value) {
  if (value != "pcie" && value != "sysemu" && value != "fake") {
    printf("Invalid value for --%s: %s\n", flagName, value.c_str());
    return false;
  }
  return true;
}
bool validateVerbosity(const char* flagName, int value) {
  if (value < -1 || value > 2) {
    printf("Invalid value for --%s: %d\n", flagName, value);
    return false;
  }
  return true;
}
bool validateTraceMode(const char* flagName, const std::string& value) {
  if (value != "json" && value != "binary") {
    printf("Invalid value for --%s: %s\n", flagName, value.c_str());
    return false;
  }
  return true;
}
constexpr auto kBootRomTrampolineToBl2Elf = "/BootromTrampolineToBL2.elf";
constexpr auto kBl2Elf = "/ServiceProcessorBL2_fast-boot.elf";
constexpr auto kMasterMinionElf = "/MasterMinion.elf";
constexpr auto kMachineMinionElf = "/MachineMinion.elf";
constexpr auto kWorkerMinionElf = "/WorkerMinion.elf";
constexpr auto kSysemuRunDir = "/run";
constexpr uint64_t kSysEmuMaxCycles = std::numeric_limits<uint64_t>::max();
constexpr uint64_t kSysEmuMinionShiresMask = 0x1FFFFFFFFu;
} // namespace

DEFINE_string(socket_path, "/var/run/et_runtime/pcie.sock",
              "Socket path for the runtime communications. This file will be removed, so if you want several runtime "
              "servers, please specify a different one for each runtime server.");
DEFINE_string(log_folder, "/var/log/et_runtime", "Folder where runtime will store the server logs.");
DEFINE_int32(log_verbosity, -1,
             "Defines level of log verbosity. -1 disables verbose logs, 0 enables low verbosity, 1 enables mid "
             "verbosity and 2 enables high verbosity");
DEFINE_string(device_type, "pcie",
              "Indicates which kind of devices the server will use. This value must be one of these: \n\t'pcie' -> "
              "physical devices\n\t'sysemu' -> emulated devices\n\t'fake' -> dummy devices (most features won't work "
              "properly, this is intended for internal development only')");
DEFINE_validator(device_type, &validateDeviceType);
DEFINE_validator(log_verbosity, &validateVerbosity);
DEFINE_string(tracing_folder, "/var/log/et_runtime", "Folder where will be put the tracing files.");
DEFINE_string(tracing_mode, "json", "Tracing mode can be json or bin");
DEFINE_validator(tracing_mode, &validateTraceMode);
DEFINE_string(tracing_file, "daemon.trace",
              "File which will be stored in tracing_folder containing the traces, it will be appended with '.json' or "
              "'.bin' depending on tracing_mode");
DEFINE_bool(enable_tracing, true, "Enables/disables runtime tracing.");
DEFINE_string(sysemu_data_folder, "/var/lib/et_runtime",
              "In case of running device_type=sysemu this folder must contain required sysemu elfs: "
              "\n\tBootromTrampolineToBL2.elf\n\tServiceProcessorBL2_fast-boot.elf\n\tMasterMinion."
              "elf\n\tMachineMinion.elf\n\tWorkerMinion.elf\n");

namespace gflags {}
namespace google {}

int main(int argc, char* argv[]) {

  using namespace gflags;
  using namespace google;
  SetUsageMessage(std::string{"Usage: "} + argv[0] + " [OPTION] ...");
  ParseCommandLineFlags(&argc, &argv, true);

  // logger creation
  if (auto res = mkdir(FLAGS_log_folder.c_str(), 0770); res != 0 && errno != EEXIST) {
    perror("Couldn't create the log folder");
    exit(1);
  }
  const std::string logfilename = "daemon";
  auto worker = g3::LogWorker::createLogWorker();
  auto handle = worker->addDefaultLogger(logfilename, FLAGS_log_folder);
  g3::only_change_at_initialization::addLogLevel(logging::VLOG_HIGH, FLAGS_log_verbosity >= 2);
  g3::only_change_at_initialization::addLogLevel(logging::VLOG_MID, FLAGS_log_verbosity >= 1);
  g3::only_change_at_initialization::addLogLevel(logging::VLOG_LOW, FLAGS_log_verbosity >= 0);
#ifdef NDEBUG
  g3::log_levels::disable(DEBUG);
#endif
  initializeLogging(worker.get());

  // sysemu setup
  auto sysemurundir = FLAGS_sysemu_data_folder + kSysemuRunDir;
  if (FLAGS_device_type == "sysemu") {
    if (auto res = mkdir(FLAGS_sysemu_data_folder.c_str(), 0770); res == 0 || errno != EEXIST) {
      perror("Sysemu data folder does not exist. Won't be able to run sysemu based execution.");
      return 1;
    }
    if (auto res = mkdir(sysemurundir.c_str(), 0770); res != 0 && errno != EEXIST) {
      perror("Couldn't create sysemu run folder");
      return 1;
    }
  }

  std::unique_ptr<dev::IDeviceLayer> deviceLayer;
  auto pid = getpid();
  if (FLAGS_device_type == "pcie") {
    deviceLayer = dev::IDeviceLayer::createPcieDeviceLayer();
  } else if (FLAGS_device_type == "sysemu") {
    emu::SysEmuOptions opts;
    opts.bootromTrampolineToBL2ElfPath = FLAGS_sysemu_data_folder + kBootRomTrampolineToBl2Elf;
    opts.spBL2ElfPath = FLAGS_sysemu_data_folder + kBl2Elf;
    opts.machineMinionElfPath = FLAGS_sysemu_data_folder + kMachineMinionElf;
    opts.masterMinionElfPath = FLAGS_sysemu_data_folder + kMasterMinionElf;
    opts.workerMinionElfPath = FLAGS_sysemu_data_folder + kWorkerMinionElf;
    opts.runDir = sysemurundir;
    opts.maxCycles = kSysEmuMaxCycles;
    opts.minionShiresMask = kSysEmuMinionShiresMask;
    opts.logFile = FLAGS_log_folder + "/" + std::to_string(pid) + ".sysemu.log";
    opts.puUart0Path = FLAGS_log_folder + "/" + std::to_string(pid) + ".pu_uart0_tx.log";
    opts.puUart1Path = FLAGS_log_folder + "/" + std::to_string(pid) + ".pu_uart1_tx.log";
    opts.spUart0Path = FLAGS_log_folder + "/" + std::to_string(pid) + ".spio_uart0_tx.log";
    opts.spUart1Path = FLAGS_log_folder + "/" + std::to_string(pid) + ".spio_uart1_tx.log";
    // check if the elfs exist first
    for (auto& elf : {opts.bootromTrampolineToBL2ElfPath, opts.spBL2ElfPath, opts.machineMinionElfPath,
                      opts.masterMinionElfPath, opts.workerMinionElfPath}) {
      struct stat buffer;
      if (stat(elf.c_str(), &buffer) != 0) {
        std::cerr << "Can't start sysemu. Missing elf file: " << elf << std::endl;
        return 1;
      }
    }
    opts.startGdb = false;
    deviceLayer = dev::IDeviceLayer::createSysEmuDeviceLayer(opts);
  } else {
    deviceLayer = std::make_unique<dev::DeviceLayerFake>();
  }

  // tracing setup
  std::unique_ptr<std::ofstream> traceFileStream;
  if (FLAGS_enable_tracing) {
    if (auto res = mkdir(FLAGS_tracing_folder.c_str(), 0770); res != 0 && errno != EEXIST) {
      perror("Couldn't create the tracing folder");
      return 1;
    }
    auto extension = (FLAGS_tracing_mode == "json") ? ".json"s : ".bin"s;
    auto tracePath = FLAGS_tracing_folder + "/" + std::to_string(pid) + "." + FLAGS_tracing_file + extension;
    auto mode = std::ios_base::out | std::ios_base::trunc;
    if (FLAGS_tracing_mode != "json") {
      mode |= std::ios_base::binary;
    }
    traceFileStream = std::make_unique<std::ofstream>(tracePath, mode);
    CHECK(traceFileStream->is_open()) << "Couldn't open trace file";
  }

  try {
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    signal(SIGABRT, signalHandler);
    auto opts = rt::Options{true, true};
    if (FLAGS_device_type == "fake") {
      opts.checkDeviceApiVersion_ = false;
    }

    rt::Server s(FLAGS_socket_path, std::move(deviceLayer), opts);
    if (FLAGS_enable_tracing) {
      auto profiler = s.getProfiler();
      auto type = rt::IProfiler::OutputType::Json;
      if (FLAGS_tracing_mode != "json") {
        type = rt::IProfiler::OutputType::Binary;
      }
      profiler->start(*traceFileStream, type);
    }

    std::unique_lock lock(s_m);
    s_cv.wait(lock, [] { return !s_running; });
    std::cout << "End server execution\n.";
    return 0;
  } catch (const rt::Exception& e) {
    std::cerr << "Terminating due to a runtime exception: " << e.what();
    return 1;
  } catch (const dev::Exception& e) {
    std::cerr << "Terminating due to a device layer exception: " << e.what();
    return 1;
  } catch (const std::exception& e) {
    std::cerr << "Terminating due to a C++ exception: " << e.what();
    return 1;
  }
}