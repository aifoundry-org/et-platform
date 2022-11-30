//******************************************************************************
// Copyright (C) 2020, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------
#include "SysEmuImp.h"
#include "emu_gio.h"
#include "memory/main_memory.h"
#include "sys_emu.h"
#include "system.h"
#include "utils.h"
#include "preload.h"
#include <fstream>
#include <future>
#include <mutex>
#include <thread>
using namespace emu;

namespace {

#define BAR0_ADDR 0x7E80000010
#define BAR1_ADDR 0x7E80000014
#define BAR2_ADDR 0x7E80000018
#define BAR3_ADDR 0x7E8000001C
#define PF0_ATU_CAP_IATU_REGION_CTRL_2_OFF_OUTBOUND_x_REGION_EN_FIELD_MASK 0x80000000ul

void logSysEmuOptions(const sys_emu_cmd_options& options) {
  SE_LOG(INFO) << std::hex;
  SE_LOG(INFO) << "************************************************************************************\n";
  SE_LOG(INFO) << "* SysEmu command options                                                           *\n";
  SE_LOG(INFO) << "************************************************************************************\n";
  SE_LOG(INFO) << " * Elf files: \n";
  for (auto& f : options.elf_files) {
    SE_LOG(INFO) << "\t" << f << "\n";
  }
  SE_LOG(INFO) << " * File load files: \n";
  for (auto& f : options.file_load_files) {
    SE_LOG(INFO) << "\t Addr: 0x" << f.addr << " File: " << f.file << "\n";
  }
  SE_LOG(INFO) << " * MemWrite32: \n";
  for (auto& f : options.mem_write32s) {
    SE_LOG(INFO) << "\t Addr: 0x" << f.addr << " Value: 0x" << f.value << "\n";
  }
  SE_LOG(INFO) << " * Mem desc file: " << options.mem_desc_file << "\n";
  SE_LOG(INFO) << " * Api comm path: " << options.api_comm_path << "\n";
  SE_LOG(INFO) << " * Minions en: 0x" << options.minions_en << "\n";
  SE_LOG(INFO) << " * Shires en: 0x" << options.shires_en << "\n";
  SE_LOG(INFO) << " * Master min: " << options.master_min << "\n";
  SE_LOG(INFO) << " * Second thread: " << options.second_thread << "\n";
  SE_LOG(INFO) << " * Log en: " << options.log_en << "\n";
  SE_LOG(INFO) << " * Log thread: \n\t";
  int num_logged_threads = 0;
  for (int i = 0; i < EMU_NUM_THREADS; ++i) {
    if (options.log_thread[i]) {
      ++num_logged_threads;
      SE_LOG(INFO) << " " << i;
    }
  }
  if (num_logged_threads > 0) {
    SE_LOG(INFO) << "\n";
  } else {
    SE_LOG(INFO) << "None\n";
  }
  SE_LOG(INFO) << " * Dump at end: \n";
  for (auto& f : options.dump_at_end) {
    SE_LOG(INFO) << "\t Addr: 0x" << f.addr << " Size: 0x" << f.size << " File: " << f.file << "\n";
  }
  SE_LOG(INFO) << " * Dump at pc: \n";
  for (auto& [k, f] : options.dump_at_pc) {
    SE_LOG(INFO) << "\t PC: 0x" << k << " Addr: 0x" << f.addr << " Size: 0x" << f.size << " File: " << f.file << "\n";
  }
  SE_LOG(INFO) << " * Dump mem: " << options.dump_mem << "\n";
  SE_LOG(INFO) << " * Reset pc: 0x" << options.reset_pc << "\n";
  SE_LOG(INFO) << " * SP reset pc: 0x" << options.sp_reset_pc << "\n";
  SE_LOG(INFO) << " * Set xreg: \n";
  for (auto& f : options.set_xreg) {
    SE_LOG(INFO) << "\t Thread: " << f.thread << " Xreg: " << f.xreg << " Value: 0x" << f.value << "\n";
  }
  SE_LOG(INFO) << " * Coherence check: " << options.coherency_check << "\n";
  SE_LOG(INFO) << " * Max cycles: 0x" << options.max_cycles << "\n";
  SE_LOG(INFO) << " * Mins dis: " << options.mins_dis << "\n";
  SE_LOG(INFO) << " * SP dis: " << options.sp_dis << "\n";
  SE_LOG(INFO) << " * Mem reset: " << options.mem_reset << "\n";
  SE_LOG(INFO) << " * pu_uart0_rx_file: " << options.pu_uart0_rx_file << "\n";
  SE_LOG(INFO) << " * pu_uart1_rx_file: " << options.pu_uart1_rx_file << "\n";
  SE_LOG(INFO) << " * spio_uart0_rx_file: " << options.spio_uart0_rx_file << "\n";
  SE_LOG(INFO) << " * spio_uart1_rx_file: " << options.spio_uart1_rx_file << "\n";
  SE_LOG(INFO) << " * pu_uart0_tx_file: " << options.pu_uart0_tx_file << "\n";
  SE_LOG(INFO) << " * pu_uart1_tx_file: " << options.pu_uart1_tx_file << "\n";
  SE_LOG(INFO) << " * spio_uart0_tx_file: " << options.spio_uart0_tx_file << "\n";
  SE_LOG(INFO) << " * spio_uart1_tx_file: " << options.spio_uart1_tx_file << "\n";
  SE_LOG(INFO) << " * Log at pc: 0x" << options.log_at_pc << "\n";
  SE_LOG(INFO) << " * Stop log at pc: 0x" << options.stop_log_at_pc << "\n";
  SE_LOG(INFO) << " * Display trap info: " << options.display_trap_info << "\n";
  SE_LOG(INFO) << " * Gdb: " << options.gdb << "\n";
#ifdef SYSEMU_PROFILING
  SE_LOG(INFO) << " * Dump prof file: " << options.dump_prof_file << "\n";
#endif
#ifdef SYSEMU_DEBUG
  SE_LOG(INFO) << " * Debug: " << options.debug << "\n";
#endif
  SE_LOG(INFO) << "************************************************************************************\n";
}

void runMain(sys_emu_cmd_options opts, api_communicate* comm, std::exception_ptr* error) {
  try {
    SE_LOG(INFO) << "Starting sysemu thread " << std::this_thread::get_id();
    logSysEmuOptions(opts);
    auto emu = std::make_unique<sys_emu>(opts, comm);
    emu->main_internal();
    SE_LOG(INFO) << "Ending sysemu thread " << std::this_thread::get_id();
  } catch (const std::exception& e) {
    SE_LOG(ERROR) << "Aborting sysemu thread " << std::this_thread::get_id() << ": " << e.what();
    *error = std::current_exception();
  }
}

void iatusPrint(bemu::System* chip) {
  const auto& iatus = chip->memory.pcie0_get_iatus();

  bemu::Noagent agent{chip, "SysEmuImp"};

  for (int i = 0, count = iatus.size(); i < count; ++i) {
    auto& iatu = iatus[i];
    auto base_addr = (uint64_t)iatu.upper_base_addr << 32 | iatu.lwr_base_addr;
    auto limit_addr = (uint64_t)iatu.uppr_limit_addr << 32 | iatu.limit_addr;
    auto target_addr = (uint64_t)iatu.upper_target_addr << 32 | iatu.lwr_target_addr;

    LOG_AGENT(INFO, agent, "iATU[%d].ctrl_1: 0x%x", i, iatu.ctrl_1);
    LOG_AGENT(INFO, agent, "iATU[%d].ctrl_2: 0x%x", i, iatu.ctrl_2);
    LOG_AGENT(INFO, agent, "iATU[%d].base_addr: 0x%" PRIx64, i, base_addr);
    LOG_AGENT(INFO, agent, "iATU[%d].limit_addr : 0x%" PRIx64, i, limit_addr);
    LOG_AGENT(INFO, agent, "iATU[%d].target_addr: 0x%" PRIx64, i, target_addr);
  }
}
bool iatuTranslate(bemu::System* chip, uint64_t pci_addr, uint64_t size, uint64_t& device_addr, uint64_t& access_size) {
  const auto& iatus = chip->memory.pcie0_get_iatus();

  bemu::Noagent agent{chip, "SysEmuImp"};

  for (int i = 0; i < iatus.size(); i++) {
    // Check REGION_EN (bit[31])
    if (((iatus[i].ctrl_2 >> 31) & 1) == 0)
      continue;

    // Check MATCH_MODE (bit[30]) to be Address Match Mode (0)
    if (((iatus[i].ctrl_2 >> 30) & 1) != 0) {
      LOG_AGENT(FTL, agent, "iATU[%d]: Unsupported MATCH_MODE", i);
    }

    uint64_t iatu_base_addr = (uint64_t)iatus[i].upper_base_addr << 32 | (uint64_t)iatus[i].lwr_base_addr;
    uint64_t iatu_limit_addr = (uint64_t)iatus[i].uppr_limit_addr << 32 | (uint64_t)iatus[i].limit_addr;
    uint64_t iatu_target_addr = (uint64_t)iatus[i].upper_target_addr << 32 | (uint64_t)iatus[i].lwr_target_addr;

    // Address within iATU
    if (pci_addr >= iatu_base_addr && pci_addr <= iatu_limit_addr) {
      uint64_t host_access_end = pci_addr + size - 1;
      uint64_t access_end = std::min(host_access_end, iatu_limit_addr) + 1;
      uint64_t offset = pci_addr - iatu_base_addr;

      access_size = access_end - pci_addr;
      device_addr = iatu_target_addr + offset;
      return true;
    }
  }
  return false;
}

/**
 * Warn about runtime options that may be ignored.
 * This parses a vector of user-supplied runtime options and makes sure
 * that none of them are forcibly overwritten by the SW stack.
 * @param opts Vector of `--opt` style runtime options.
 */
void checkExtraOptions(const std::vector<std::string>& opts)
{
  static constexpr std::array<const char*, 17> reserved = {
    "-mins_dis", "-minions_en", "-mem_reset", "-mem_reset32",
    "-shires", "-max_cycles", "-elf", "-elf_load", "-lp",
    "-pu_uart0_rx_file", "-pu_uart1_rx_file",
    "-spio_uart0_rx_file", "-spio_uart1_rx_file",
    "-pu_uart0_tx_file", "-pu_uart1_tx_file",
    "-spio_uart0_tx_file", "-spio_uart1_tx_file",
  };
  for (auto&& opt : opts) {
    for (auto&& res : reserved) {
      if (opt.find(res) != std::string::npos) {
        SE_LOG(ERROR) << "Option '" << res << "' is reserved and may be overwritten";
      }
    }
  }
}

} // namespace

void SysEmuImp::set_system(bemu::System* system) {
  chip_ = system;
  agent_.chip = system;
}

void SysEmuImp::process() {
  std::unique_lock<std::mutex> lock(mutex_);
  if (!requests_.empty()) {
    SE_LOG(INFO) << "Processing request...";
    auto&& req = requests_.front();
    req();
    requests_.pop();
  }
  if (should_pause_) {
    using namespace std::chrono_literals;
    condVar_.wait_for(lock, 100ms);
  }
}

void SysEmuImp::mmioRead(uint64_t address, size_t size, std::byte* dst) {
  resume();
  std::promise<void> p;
  auto request = [=, &p]() {
    SE_LOG(INFO) << "Device memory read at: " << std::hex << address << " size: " << size << " host dst: " << dst;
    auto pci_addr = address;
    uint64_t host_access_offset = 0;
    int64_t readSize = size;
    while (readSize > 0) {

      uint64_t device_addr, access_size;

      if (!iatuTranslate(chip_, pci_addr, readSize, device_addr, access_size)) {
        LOG_AGENT(WARN, agent_, "iATU: Could not find translation for host address: 0x%" PRIx64 ", size: 0x%" PRIx64,
                  pci_addr, readSize);
        iatusPrint(chip_);
        break;
      }

      try {
        chip_->memory.read(agent_, device_addr, access_size, dst + host_access_offset);
      } catch (...) {
        p.set_exception(std::current_exception());
        return;
      }

      pci_addr += access_size;
      host_access_offset += access_size;
      readSize -= access_size;
    }

    if (readSize > 0) {
      p.set_exception(std::make_exception_ptr(emu::Exception(
        "Invalid IATU translation. Size too big to be covered fully by iATUs / translation failure. Address: " +
        std::to_string(address) + " size: " + std::to_string(readSize))));
      return;
    }
    p.set_value();
  };
  std::unique_lock<std::mutex> lock(mutex_);
  requests_.emplace(std::move(request));
  lock.unlock();
  p.get_future().get();
}

void SysEmuImp::mmioWrite(uint64_t address, size_t size, const std::byte* src) {
  resume();
  std::promise<void> p;
  auto request = [=, &p]() {
    SE_LOG(INFO) << "Device memory write at: " << std::hex << address << " size: " << size << " host src: " << src;
    auto pci_addr = address;
    uint64_t host_access_offset = 0;
    int64_t readSize = size;
    while (readSize > 0) {
      uint64_t device_addr, access_size;
      if (!iatuTranslate(chip_, pci_addr, size, device_addr, access_size)) {
        LOG_AGENT(WARN, agent_, "iATU: Could not find translation for host address: 0x%" PRIx64 ", size: 0x%" PRIx64,
                  pci_addr, size);
        iatusPrint(chip_);
        break;
      }
      try {
        chip_->memory.write(agent_, device_addr, access_size, src + host_access_offset);
      } catch (...) {
        p.set_exception(std::current_exception());
        return;
      }

      pci_addr += access_size;
      host_access_offset += access_size;
      readSize -= access_size;
    }
    if (readSize > 0) {
      p.set_exception(std::make_exception_ptr(emu::Exception(
        "Invalid IATU translation. Size too big to be covered fully by iATUs / translation failure. Address: " +
        std::to_string(address) + " size: " + std::to_string(readSize))));
      return;
    }
    p.set_value();
  };
  std::unique_lock<std::mutex> lock(mutex_);
  requests_.emplace(std::move(request));
  lock.unlock();
  p.get_future().get();
}

void SysEmuImp::raiseDevicePuPlicPcieMessageInterrupt() {
  resume();
  auto request = [=]() {
    SE_LOG(INFO) << "raiseDevicePuPlicPcieMessageInterrupt";
    LOG_AGENT(INFO, agent_, "raise_device_interrupt(type = %s)", "PU");
    chip_->memory.pu_trg_pcie_mmm_int_inc(agent_);
  };
  std::lock_guard<std::mutex> lock(mutex_);
  requests_.emplace(std::move(request));
}

uint32_t SysEmuImp::waitForInterrupt(uint32_t bitmap) {
  resume();
  std::unique_lock<std::mutex> lock(mutex_);
  if (!(pendingInterruptsBitmask_ & bitmap)) {
    condVar_.wait(lock, [this, bitmap]() { return !running_ || (bitmap & pendingInterruptsBitmask_); });
  }
  if (running_) {
    bitmap &= pendingInterruptsBitmask_;
    pendingInterruptsBitmask_ &= ~bitmap;
  }
  return bitmap;
}

bool SysEmuImp::raise_host_interrupt(uint32_t bitmap) {
  LOG_AGENT(INFO, agent_, "Raise Host (Count: %" PRId64 ") Interrupt Bitmap: (0x%" PRIx32 ")",
            ++raised_interrupt_count_, bitmap);
  std::lock_guard<std::mutex> lock(mutex_);
  pendingInterruptsBitmask_ |= bitmap;
  condVar_.notify_all();
  return true;
}

void SysEmuImp::raiseDeviceSpioPlicPcieMessageInterrupt() {
  resume();
  auto request = [=]() {
    SE_LOG(INFO) << "raiseDeviceSpioPlicPcieMessageInterrupt";
    LOG_AGENT(INFO, agent_, "raise_device_interrupt(type = %s)", "SP");
    chip_->memory.pu_trg_pcie_ipi_trigger(agent_);
  };
  std::lock_guard<std::mutex> lock(mutex_);
  requests_.emplace(std::move(request));
}

bool SysEmuImp::host_memory_read(uint64_t host_addr, uint64_t size, void* data) {
  if (!hostListener_) {
    LOG_AGENT(WARN, agent_, "can't read host memory without hostListener (addr=0x%" PRIx64 ")", host_addr);
    return false;
  }
  hostListener_->memoryReadFromHost(host_addr, size, reinterpret_cast<std::byte*>(data));
  return true;
}

bool SysEmuImp::host_memory_write(uint64_t host_addr, uint64_t size, const void* data) {
  if (!hostListener_) {
    LOG_AGENT(WARN, agent_, "can't write host memory without hostListener (addr=0x%" PRIx64 ")", host_addr);
    return false;
  }
  hostListener_->memoryWriteFromHost(host_addr, size, reinterpret_cast<const std::byte*>(data));
  return true;
}

void SysEmuImp::notify_iatu_ctrl_2_reg_write(int pcie_id, uint32_t iatu, uint32_t value) {
  LOG_AGENT(DEBUG, agent_, "notify_iatu_ctrl_2_reg_write: %d, 0x%x, 0x%x", pcie_id, iatu, value);
  // We only care about PCIE0
  // This expects the latest iATU to be configured by BL2 to be iATU 3:
  //   https://gitlab.esperanto.ai/software/device-bootloaders/-/blob/master/shared/src/pcie_init.c#L644
  if ((pcie_id == 0) && (iatu == 3) && (value & PF0_ATU_CAP_IATU_REGION_CTRL_2_OFF_OUTBOUND_x_REGION_EN_FIELD_MASK)) {
    iatusReady_.set_value();
  }
}

SysEmuImp::~SysEmuImp() {
  std::promise<void> p;
  auto request = [=, &p]() {
    try {
      chip_->set_emu_done(true);
      p.set_value();
    } catch (...) {
      p.set_exception(std::current_exception());
    }
  };
  std::unique_lock<std::mutex> lock(mutex_);
  requests_.emplace(std::move(request));
  stop();
  lock.unlock();
  // Wait until set_emu_done is called
  p.get_future().get();
  // Empty request queue
  lock.lock();
  while (!requests_.empty()) {
    requests_.pop();
  }
  lock.unlock();

  SE_LOG(INFO) << "Waiting for sysemu thread to finish.";
  sysEmuThread_.join();
  SE_LOG(INFO) << "Sysemu thread finished.";
  if (sysEmuError_) {
    std::rethrow_exception(sysEmuError_);
  }
}

SysEmuImp::SysEmuImp(const SysEmuOptions& options, const std::array<uint64_t, 8>& barAddresses,
                     IHostListener* hostListener)
  : hostListener_(hostListener) {
  const std::vector<std::string> preloadElfs = {options.bootromTrampolineToBL2ElfPath, options.spBL2ElfPath,
                                                options.masterMinionElfPath, options.machineMinionElfPath,
                                                options.workerMinionElfPath};

  sys_emu_cmd_options opts;

  if (!options.additionalOptions.empty()) {
    checkExtraOptions(options.additionalOptions);
    std::vector<const char*> extraOptions;
    extraOptions.emplace_back("dummyExecName");
    for (auto& opt : options.additionalOptions) {
      extraOptions.emplace_back(opt.c_str());
    }
    auto parsed = sys_emu::parse_command_line_arguments(extraOptions.size(), const_cast<char**>(extraOptions.data()));
    optind = 0; // need to reset global variable optind because above function uses getopt_long. See
                // https://linux.die.net/man/3/getopt_long
    if (!std::get<0>(parsed)) {
      throw Exception("Error parsing SysEmu arguments");
    }
    opts = std::get<1>(parsed);
  }
  if (opts.mem_write32s.empty()) {
    opts.mem_write32s.emplace_back(
      sys_emu_cmd_options::mem_write32{BAR0_ADDR, static_cast<uint32_t>(barAddresses[0] & 0xFFFFFFFFu)});
    opts.mem_write32s.emplace_back(
      sys_emu_cmd_options::mem_write32{BAR1_ADDR, static_cast<uint32_t>(barAddresses[0] >> 32)});
    opts.mem_write32s.emplace_back(
      sys_emu_cmd_options::mem_write32{BAR2_ADDR, static_cast<uint32_t>(barAddresses[2] & 0xFFFFFFFFu)});
    opts.mem_write32s.emplace_back(
      sys_emu_cmd_options::mem_write32{BAR3_ADDR, static_cast<uint32_t>(barAddresses[2] >> 32)});
  }

  opts.mins_dis = true;
  opts.minions_en = 0xFFFFFFFF;
  opts.mem_reset = options.mem_reset32;
  opts.shires_en = options.minionShiresMask | (1ull << 34); // always enable Service Processor
  opts.max_cycles = options.maxCycles;
  opts.gdb |= options.startGdb;

  /* Route PU UART0 output to log file */
  opts.pu_uart0_tx_file = options.puUart0Path.empty() ? options.runDir + "/" + "pu_uart0_tx.log" : options.puUart0Path;
  SE_LOG(ERROR) << "PU UART0 TX logfile path " << opts.pu_uart0_tx_file;

  /* Route PU UART 1 TX for FIFO if a valid FIFO path was provided by caller, else route to log file */
  if (!options.puUart1FifoOutPath.empty()) {
    opts.pu_uart1_tx_file = options.puUart1FifoOutPath;
  } else {
    opts.pu_uart1_tx_file =
      options.puUart1Path.empty() ? options.runDir + "/" + "pu_uart1_tx.log" : options.puUart1Path;
  }
  SE_LOG(ERROR) << "PU UART1 TX FIFO path " << opts.pu_uart1_tx_file;

  /* Route PU UART 1 RX for FIFO if a valid FIFO path was provided by caller */
  if (!options.puUart1FifoInPath.empty()) {
    opts.pu_uart1_rx_file = options.puUart1FifoInPath;
    SE_LOG(ERROR) << "PU UART1 RX FIFO path " << opts.pu_uart1_tx_file;
  }

  /* Route SP UART 0 TX for FIFO if a valid FIFO path was provided by caller, else route to log file */
  if (!options.spUart0FifoOutPath.empty()) {
    opts.spio_uart0_tx_file = options.spUart0FifoOutPath;
    SE_LOG(INFO) << "SP TX FIFO path " << opts.spio_uart0_tx_file;
  } else {
    opts.spio_uart0_tx_file =
      options.spUart0Path.empty() ? options.runDir + "/" + "spio_uart0_tx.log" : options.spUart0Path;
  }

  /* Route SP UART 0 RX for FIFO if a valid FIFO path was provided by caller */
  if (!options.spUart0FifoInPath.empty()) {
    opts.spio_uart0_rx_file = options.spUart0FifoInPath;
    SE_LOG(INFO) << "SP RX FIFO path " << opts.spio_uart0_rx_file;
  }

  /* Route SP UART 1 TX for FIFO if a valid FIFO path was provided by caller, else route to log file */
  if (!options.spUart1FifoOutPath.empty()) {
    opts.spio_uart1_tx_file = options.spUart1FifoOutPath;
    SE_LOG(INFO) << "SP TX FIFO path " << opts.spio_uart1_tx_file;
  } else {
    opts.spio_uart1_tx_file =
      options.spUart1Path.empty() ? options.runDir + "/" + "spio_uart1_tx.log" : options.spUart1Path;
  }

  /* Route SP UART 0 TX for FIFO if a valid FIFO path was provided by caller */
  if (!options.spUart1FifoInPath.empty()) {
    opts.spio_uart1_rx_file = options.spUart1FifoInPath;
    SE_LOG(INFO) << "SP RX FIFO path " << opts.spio_uart1_rx_file;
  }

  std::copy_if(preloadElfs.begin(), preloadElfs.end(),
               std::back_inserter(opts.elf_files), [](const std::string& path) { return !path.empty(); });
  opts.mem_check |= options.memcheck;
  opts.l2_scp_check |= options.l2ScpCheck;
  opts.l1_scp_check |=  options.l1ScpCheck;
  opts.flb_check |= options.flbCheck;
  opts.tstore_check |= options.tstoreCheck;
  opts.log_path = options.logFile;

  sysEmuThread_ = std::thread(runMain, opts, this, &sysEmuError_); // FIXME Passing `this` like this is dangerous..

  // Wait until all the iATUs configured by BL2 have been enabled
  auto future = iatusReady_.get_future();
  future.get();

  SE_LOG(INFO) << "Calling pcieReady";
  hostListener_->pcieReady();
}

void SysEmuImp::notify_fatal_error(const std::string& error) {
  hostListener_->onSysemuFatalError(error);
}

void SysEmuImp::stop() {
  running_ = false;
  // Wake host interrupt waiters
  condVar_.notify_all();
}

void SysEmuImp::pause() {
  if (!should_pause_) {
    SE_LOG(INFO) << "Pause sysemu thread";
    should_pause_ = true;
  }
}

void SysEmuImp::resume() {
  if (should_pause_) {
    SE_LOG(INFO) << "Resume sysemu thread";
    should_pause_ = false;
    condVar_.notify_one();
  }
}
