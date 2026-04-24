//******************************************************************************
// Copyright (c) 2026 Ainekko, Co.
// SPDX-License-Identifier: Apache-2.0
//------------------------------------------------------------------------------
//
// erbium_launcher - Host launcher for Erbium compute kernels on ET-SoC-1
//
// Loads a RISC-V ELF binary and launches it on a single shire of the ET-SoC-1,
// simulating the Erbium single-neighborhood topology.
//
//------------------------------------------------------------------------------

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <getopt.h>
#include <iostream>
#include <string>
#include <vector>

#include <device-layer/IDeviceLayer.h>
#include <hostUtils/logging/Logger.h>
#include <runtime/IRuntime.h>
#include <runtime/Types.h>
#include <sw-sysemu/SysEmuOptions.h>

namespace {

constexpr size_t kErbiumMemSize = 16 * 1024 * 1024; // 16 MB

struct FileLoad {
  uint64_t addr;
  std::string path;
};

enum class Device { soc1sim, sys_emu };

struct Options {
  std::string elf_path;
  std::string dump_before;
  std::string dump_after;
  std::vector<FileLoad> file_loads;
  uint32_t shire_id = 0;
  uint64_t timeout_secs = 60;
  Device device = Device::soc1sim;
  std::string et_platform_path;
};

std::string get_et_platform_path() {
  if (const char* v = std::getenv("ET_PLATFORM"))
    return v;
  return ET_PLATFORM_INSTALL_PREFIX;
}

void print_usage(const char* prog) {
  std::cerr
      << "Usage: " << prog << " [options]\n\n"
      << "Erbium-on-SoC1 kernel launcher.\n\n"
      << "Required:\n"
      << "  --elf-load <file>       RISC-V ELF binary to load and execute\n\n"
      << "Optional:\n"
      << "  --device <name>         Device backend: soc1sim (default) or sys_emu\n"
      << "  --shire <id>            Shire to run on (default: 0)\n"
      << "  --file_load <addr>,<file>  Load raw file at offset <addr> in device buffer\n"
      << "  --dump_before <file>    Dump device memory to file before kernel launch\n"
      << "  --dump_after <file>     Dump device memory to file after kernel completes\n"
      << "  --timeout <secs>        Kernel timeout in seconds (default: 60, 0 = wait indefinitely)\n"
      << "  -h, --help              Show this message\n\n"
      << "Environment:\n"
      << "  ET_PLATFORM             Toolchain root for firmware (default: "
      << ET_PLATFORM_INSTALL_PREFIX << ")\n";
}

Options parse_args(int argc, char** argv) {
  static const struct option long_opts[] = {
      {"elf-load", required_argument, nullptr, 'e'},
      {"device", required_argument, nullptr, 'd'},
      {"shire", required_argument, nullptr, 's'},
      {"file_load", required_argument, nullptr, 'f'},
      {"dump_before", required_argument, nullptr, 'b'},
      {"dump_after", required_argument, nullptr, 'a'},
      {"timeout", required_argument, nullptr, 't'},
      {"help", no_argument, nullptr, 'h'},
      {nullptr, 0, nullptr, 0},
  };

  Options opts;
  int c;
  while ((c = getopt_long(argc, argv, "h", long_opts, nullptr)) != -1) {
    switch (c) {
    case 'e':
      opts.elf_path = optarg;
      break;
    case 'd':
      if (std::string(optarg) == "sys_emu")
        opts.device = Device::sys_emu;
      else if (std::string(optarg) == "soc1sim")
        opts.device = Device::soc1sim;
      else {
        std::cerr << "Error: unknown --device '" << optarg
                  << "'; expected soc1sim or sys_emu\n";
        std::exit(1);
      }
      break;
    case 's':
      opts.shire_id = static_cast<uint32_t>(std::stoul(optarg));
      break;
    case 'f': {
      std::string arg = optarg;
      auto comma = arg.find(',');
      if (comma == std::string::npos) {
        std::cerr << "Error: --file_load requires <addr>,<file>\n";
        std::exit(1);
      }
      uint64_t addr = std::stoull(arg.substr(0, comma), nullptr, 0);
      opts.file_loads.push_back({addr, arg.substr(comma + 1)});
      break;
    }
    case 'b':
      opts.dump_before = optarg;
      break;
    case 'a':
      opts.dump_after = optarg;
      break;
    case 't':
      opts.timeout_secs = std::stoull(optarg);
      break;
    case 'h':
      print_usage(argv[0]);
      std::exit(0);
    default:
      print_usage(argv[0]);
      std::exit(1);
    }
  }

  if (opts.elf_path.empty()) {
    std::cerr << "Error: --elf-load is required\n\n";
    print_usage(argv[0]);
    std::exit(1);
  }

  return opts;
}

std::vector<std::byte> read_file(const std::string& path) {
  auto size = std::filesystem::file_size(path);
  std::vector<std::byte> buf(size);
  std::ifstream f(path, std::ios::binary);
  if (!f) {
    std::cerr << "Error: cannot open '" << path << "'\n";
    std::exit(1);
  }
  f.read(reinterpret_cast<char*>(buf.data()), static_cast<std::streamsize>(size));
  return buf;
}

void dump_device_memory(rt::IRuntime& runtime, rt::StreamId stream,
                        std::byte* deviceAddr, size_t size,
                        const std::string& path) {
  std::vector<std::byte> hostBuf(size);
  auto evt = runtime.memcpyDeviceToHost(stream, deviceAddr, hostBuf.data(), size);
  if (!runtime.waitForEvent(evt)) {
    std::cerr << "Error: device-to-host memcpy timed out for dump\n";
    return;
  }
  std::ofstream f(path, std::ios::binary);
  if (!f) {
    std::cerr << "Error: cannot open dump file '" << path << "'\n";
    return;
  }
  f.write(reinterpret_cast<const char*>(hostBuf.data()), static_cast<std::streamsize>(size));
  std::cout << "Dumped " << size << " bytes to " << path << "\n";
}

emu::SysEmuOptions make_sysemu_options() {
  std::string et = get_et_platform_path() + "/";

  emu::SysEmuOptions o;
  o.bootromTrampolineToBL2ElfPath = et + "lib/esperanto-fw/BootromTrampolineToBL2/BootromTrampolineToBL2.elf";
  o.spBL2ElfPath      = et + "lib/esperanto-fw/ServiceProcessorBL2/fast-boot/ServiceProcessorBL2_fast-boot.elf";
  o.machineMinionElfPath = et + "lib/esperanto-fw/MachineMinion/MachineMinion.elf";
  o.masterMinionElfPath  = et + "lib/esperanto-fw/MasterMinion/MasterMinion.elf";
  o.workerMinionElfPath  = et + "lib/esperanto-fw/WorkerMinion/WorkerMinion.elf";
  o.executablePath     = et + "bin/sys_emu";

  for (const auto& p : {o.bootromTrampolineToBL2ElfPath, o.spBL2ElfPath,
                        o.machineMinionElfPath, o.masterMinionElfPath,
                        o.workerMinionElfPath, o.executablePath}) {
    if (!std::filesystem::exists(p)) {
      std::cerr << "Error: sys_emu firmware not found: " << p << "\n"
                << "       Set ET_PLATFORM to the installed ET platform root.\n";
      std::exit(1);
    }
  }

  o.runDir = std::filesystem::current_path().string() + "/";
  o.maxCycles = std::numeric_limits<uint64_t>::max();
  o.minionShiresMask = 0x1FFFFFFFFu;
  o.puUart0Path  = o.runDir + "pu_uart0_tx.log";
  o.puUart1Path  = o.runDir + "pu_uart1_tx.log";
  o.spUart0Path  = o.runDir + "spio_uart0_tx.log";
  o.spUart1Path  = o.runDir + "spio_uart1_tx.log";
  o.startGdb  = false;
  o.memcheck  = false;
  return o;
}

} // namespace

int main(int argc, char** argv) {
  auto opts = parse_args(argc, argv);

  uint64_t shire_mask = uint64_t{1} << opts.shire_id;

  const char* dev_name = (opts.device == Device::sys_emu) ? "sys_emu" : "soc1sim";
  std::cout << "erbium_launcher: elf=" << opts.elf_path
            << " device=" << dev_name
            << " shire=" << opts.shire_id << "\n";

  // Initialize logger (must exist before runtime creation)
  logging::LoggerDefault logger;

  // Create device layer and runtime
  std::unique_ptr<dev::IDeviceLayer> deviceLayer;
  if (opts.device == Device::sys_emu) {
    deviceLayer = dev::IDeviceLayer::createSysEmuDeviceLayer(make_sysemu_options());
  } else {
    deviceLayer = dev::IDeviceLayer::createPcieDeviceLayer();
  }
  auto runtime = rt::IRuntime::create(std::move(deviceLayer));

  // Register error callbacks
  runtime->setOnStreamErrorsCallback(
      [](rt::EventId id, const rt::StreamError& err) {
        std::cerr << "Stream error (event " << static_cast<int>(id)
                  << "): code " << static_cast<int>(err.errorCode_) << "\n";
      });
  runtime->setOnKernelAbortedErrorCallback(
      [](rt::EventId id, std::byte*, size_t, std::function<void()> freeRes) {
        std::cerr << "Kernel aborted (event " << static_cast<int>(id) << ")\n";
        freeRes();
      });

  auto devices = runtime->getDevices();
  if (devices.empty()) {
    std::cerr << "Error: no devices found\n";
    return 1;
  }

  auto device = devices[0];
  auto stream = runtime->createStream(device);

  // Load ELF; runtime allocates the device buffer internally (freed by unloadCode).
  // The kernel's linker script reserves the full Erbium memory layout, so the
  // allocated buffer covers heap0 / heap1 and the offsets used by --file_load.
  auto elf = read_file(opts.elf_path);
  auto loadResult = runtime->loadCode(stream, elf.data(), elf.size());
  if (!runtime->waitForEvent(loadResult.event_)) {
    std::cerr << "Error: ELF load timed out\n";
    return 1;
  }

  auto* deviceBuf = loadResult.loadAddress_;

  std::cout << "Kernel loaded at 0x" << std::hex
            << reinterpret_cast<uintptr_t>(deviceBuf)
            << std::dec << "\n";

  // Load raw files into the device buffer at offsets relative to the kernel base.
  for (const auto& fl : opts.file_loads) {
    if (fl.addr >= kErbiumMemSize) {
      std::cerr << "Error: --file_load offset 0x" << std::hex << fl.addr
                << " exceeds buffer size\n";
      return 1;
    }
    auto data = read_file(fl.path);
    if (fl.addr + data.size() > kErbiumMemSize) {
      std::cerr << "Error: --file_load " << fl.path << " (size " << data.size()
                << ") overflows buffer at offset 0x" << std::hex << fl.addr << "\n";
      return 1;
    }
    auto evt = runtime->memcpyHostToDevice(stream, data.data(), deviceBuf + fl.addr, data.size());
    if (!runtime->waitForEvent(evt)) {
      std::cerr << "Error: memcpy timed out for --file_load " << fl.path << "\n";
      return 1;
    }
    std::cout << "Loaded " << std::dec << data.size() << " bytes from " << fl.path
              << " at offset 0x" << std::hex << fl.addr << std::dec << "\n";
  }

  // Pre-launch memory dump
  if (!opts.dump_before.empty()) {
    dump_device_memory(*runtime, stream, deviceBuf, kErbiumMemSize, opts.dump_before);
  }

  // Launch kernel on the selected shire.
  rt::KernelLaunchOptions kOpts;
  kOpts.setShireMask(shire_mask);
  kOpts.setBarrier(true);
  kOpts.setFlushL3(true);

  runtime->kernelLaunch(stream, loadResult.kernel_, nullptr, 0, kOpts);

  std::cout << "Kernel launched, waiting for completion...\n";

  auto timeout = opts.timeout_secs == 0
      ? std::chrono::hours(24)
      : std::chrono::seconds(opts.timeout_secs);

  if (!runtime->waitForStream(stream, timeout)) {
    std::cerr << "Error: kernel execution timed out\n";
    runtime->abortStream(stream);
    return 1;
  }

  // Check for errors
  auto errors = runtime->retrieveStreamErrors(stream);
  if (!errors.empty()) {
    std::cerr << "Kernel finished with " << errors.size() << " error(s)\n";
    for (const auto& err : errors) {
      std::cerr << "  error code: "
                << static_cast<int>(err.errorCode_) << "\n";
    }
  }

  // Post-execution memory dump
  if (!opts.dump_after.empty()) {
    dump_device_memory(*runtime, stream, deviceBuf, kErbiumMemSize, opts.dump_after);
  }

  if (!errors.empty()) {
    runtime->unloadCode(loadResult.kernel_);
    runtime->destroyStream(stream);
    return 1;
  }

  std::cout << "Kernel completed successfully\n";

  runtime->unloadCode(loadResult.kernel_);
  runtime->destroyStream(stream);
  return 0;
}
