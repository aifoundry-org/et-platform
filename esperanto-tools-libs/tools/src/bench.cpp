/*-------------------------------------------------------------------------
 * Copyright (C) 2022, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/
#include "Constants.h"
#include "json_conversion.h"
#include "runtime/DeviceLayerFake.h"
#include "runtime/IRuntime.h"
#include "runtime/Types.h"
#include "tools/IBenchmarker.h"
#include <experimental/filesystem>
#include <gflags/gflags.h>
#include <hostUtils/logging/Instance.h>
#include <hostUtils/logging/Logger.h>
#include <iomanip>
#include <iostream>

using namespace rt;

DEFINE_uint64(h2d, 16 << 20, "transfer size from host to device");
DEFINE_uint64(d2h, 16 << 20, "transfer size from device to host");
DEFINE_uint32(dmask, -1, "device mask to enable/disable devices to benchmark");
DEFINE_uint64(th, 2, "number of threads per device");
DEFINE_uint64(wl, 10, "number of workloads to execute per thread");
DEFINE_uint32(numh2d, 1, "number of transfers of h2d/numh2d bytes size");
DEFINE_uint32(numd2h, 1, "number of transfers of d2h/numd2h bytes size");
DEFINE_bool(enableLogging, false, "enable INFO level of logger");
DEFINE_string(tracePath, "", "path for the runtime trace. If empty then it won't be a runtime trace");
DEFINE_bool(h, false, "Show help");
DEFINE_bool(json, false, "Outputs a json summary.");
DECLARE_bool(help);
DECLARE_bool(helpshort);

// DEFINE_bool(dma, false, "enable dma buffers (allows zero-copy)");
// DEFINE_string(kernelPath, "", "path of the kernel to load and execute");
DEFINE_uint32(deviceLayer, 0, "DeviceLayer type: 0 -> fake; 1 -> sysemu based; 2 -> pcie; 3-> socket");
DEFINE_string(socketPath, "/var/run/et_runtime/pcie.sock", "socket path when connecting to a daemon");

static bool ValidatePort(const char* flagname, google::uint32 value) {
  if (value >= 0 && value <= 3) // value is ok
    return true;
  printf("Invalid value for --%s: %d\n", flagname, (int)value);
  return false;
}
DEFINE_validator(deviceLayer, &ValidatePort);

inline auto getDefaultSysemuOptions() {
  constexpr uint64_t kSysEmuMaxCycles = std::numeric_limits<uint64_t>::max();
  constexpr uint64_t kSysEmuMinionShiresMask = 0x1FFFFFFFFu;

  emu::SysEmuOptions sysEmuOptions;
  sysEmuOptions.bootromTrampolineToBL2ElfPath = BOOTROM_TRAMPOLINE_TO_BL2_ELF;
  sysEmuOptions.spBL2ElfPath = BL2_ELF;
  sysEmuOptions.machineMinionElfPath = MACHINE_MINION_ELF;
  sysEmuOptions.masterMinionElfPath = MASTER_MINION_ELF;
  sysEmuOptions.workerMinionElfPath = WORKER_MINION_ELF;
  sysEmuOptions.executablePath = std::string(SYSEMU_INSTALL_DIR) + "sys_emu";
  sysEmuOptions.runDir = std::experimental::filesystem::current_path();
  sysEmuOptions.maxCycles = kSysEmuMaxCycles;
  sysEmuOptions.minionShiresMask = kSysEmuMinionShiresMask;
  sysEmuOptions.puUart0Path = sysEmuOptions.runDir + "/pu_uart0_tx.log";
  sysEmuOptions.puUart1Path = sysEmuOptions.runDir + "/pu_uart1_tx.log";
  sysEmuOptions.spUart0Path = sysEmuOptions.runDir + "/spio_uart0_tx.log";
  sysEmuOptions.spUart1Path = sysEmuOptions.runDir + "/spio_uart1_tx.log";
  sysEmuOptions.startGdb = false;
  return sysEmuOptions;
}

auto createDeviceLayer() {
  std::unique_ptr<dev::IDeviceLayer> result;
  switch (FLAGS_deviceLayer) {
  case 0:
    result.reset(new dev::DeviceLayerFake);
    break;
  case 1:
    result = dev::IDeviceLayer::createSysEmuDeviceLayer(getDefaultSysemuOptions());
    break;
  case 2:
    result = dev::IDeviceLayer::createPcieDeviceLayer();
    break;
  case 3:
    // case 3 won't create a devicelayer, it will connect through socket
  default:;
    // do nothing
  }
  return result;
}
int main(int argc, char* argv[]) {
  logging::Instance logger;
  if (!FLAGS_enableLogging) {
    g3::log_levels::disable(INFO);
  }

  google::SetUsageMessage("Usage: ");
  google::ParseCommandLineNonHelpFlags(&argc, &argv, true);
  if (FLAGS_help || FLAGS_h) {
    FLAGS_help = false;
    FLAGS_helpshort = true;
  }
  google::HandleCommandLineHelpFlags();

  auto deviceLayer = createDeviceLayer();
  decltype(rt::IRuntime::create(nullptr)) runtime;
  if (deviceLayer) {
    runtime = rt::IRuntime::create(deviceLayer.get());
  } else {
    runtime = rt::IRuntime::create(FLAGS_socketPath);
  }
  auto benchmarker = IBenchmarker::create(runtime.get());
  IBenchmarker::Options opts;
  opts.runtimeTracePath = FLAGS_tracePath;
  // opts.kernelPath = FLAGS_kernelPath;
  // opts.useDmaBuffers = FLAGS_dma;
  opts.bytesD2H = FLAGS_d2h;
  opts.bytesH2D = FLAGS_h2d;
  opts.numThreads = FLAGS_th;
  opts.numD2H = FLAGS_numd2h;
  opts.numH2D = FLAGS_numh2d;
  opts.numWorkloadsPerThread = FLAGS_wl;

  IBenchmarker::DeviceMask mask;
  mask.mask_ = FLAGS_dmask;
  auto results = benchmarker->run(opts, mask);
  if (FLAGS_json) {
    using namespace nlohmann;
    json j;
    j["options"] = opts;
    j["execution"] = results;
    std::cout << std::setprecision(2) << std::fixed << j.dump(2) << std::endl;
  } else {
    std::cout << "Summary: " << std::setprecision(2) << std::fixed << "\n * H2D: " << results.bytesSentPerSecond / 1e6
              << "MB/s"
              << "\n * D2H: " << results.bytesReceivedPerSecond / 1e6 << "MB/s"
              << "\n * Workloads/s: " << results.workloadsPerSecond << std::endl;
  }
}
