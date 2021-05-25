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
#include "agent.h"
#include "emu_gio.h"
#include "memory/main_memory.h"
#include "sys_emu.h"
#include "system.h"
#include "utils.h"
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

struct Agent : bemu::Agent {
  std::string name() const override {
    return "SysEmuImp";
  }
} g_Agent;

void logSysEmuOptions(std::ostream& out, const sys_emu_cmd_options& options) {
  out << std::hex;
  out << "************************************************************************************\n";
  out << "* SysEmu command options                                                           *\n";
  out << "************************************************************************************\n";
  out << " * Elf files: \n";
  for (auto& f : options.elf_files) {
    out << "\t" << f << "\n";
  }
  out << " * File load files: \n";
  for (auto& f : options.file_load_files) {
    out << "\t Addr: 0x" << f.addr << " File: " << f.file << "\n";
  }
  out << " * MemWrite32: \n";
  for (auto& f : options.mem_write32s) {
    out << "\t Addr: 0x" << f.addr << " Value: 0x" << f.value << "\n";
  }
  out << " * Mem desc file: " << options.mem_desc_file << "\n";
  out << " * Api comm path: " << options.api_comm_path << "\n";
  out << " * Minions en: 0x" << options.minions_en << "\n";
  out << " * Shires en: 0x" << options.shires_en << "\n";
  out << " * Master min: " << options.master_min << "\n";
  out << " * Second thread: " << options.second_thread << "\n";
  out << " * Log en: " << options.log_en << "\n";
  out << " * Log thread: \n\t";
  int num_logged_threads = 0;
  for (int i = 0; i < EMU_NUM_THREADS; ++i) {
    if (options.log_thread[i]) {
      ++num_logged_threads;
      out << " " << i;
    }
  }
  if (num_logged_threads > 0) {
    out << "\n";
  } else {
    out << "None\n";
  }
  out << " * Dump at end: \n";
  for (auto& f : options.dump_at_end) {
    out << "\t Addr: 0x" << f.addr << " Size: 0x" << f.size << " File: " << f.file << "\n";
  }
  out << " * Dump at pc: \n";
  for (auto& [k, f] : options.dump_at_pc) {
    out << "\t PC: 0x" << k << " Addr: 0x" << f.addr << " Size: 0x" << f.size << " File: " << f.file << "\n";
  }
  out << " * Dump mem: " << options.dump_mem << "\n";
  out << " * Reset pc: 0x" << options.reset_pc << "\n";
  out << " * SP reset pc: 0x" << options.sp_reset_pc << "\n";
  out << " * Set xreg: \n";
  for (auto& f : options.set_xreg) {
    out << "\t Thread: " << f.thread << " Xreg: " << f.xreg << " Value: 0x" << f.value << "\n";
  }
  out << " * Coherence check: " << options.coherency_check << "\n";
  out << " * Max cycles: 0x" << options.max_cycles << "\n";
  out << " * Mins dis: " << options.mins_dis << "\n";
  out << " * SP dis: " << options.sp_dis << "\n";
  out << " * Mem reset: " << options.mem_reset << "\n";
  out << " * pu_uart0_rx_file: " << options.pu_uart0_rx_file << "\n";
  out << " * pu_uart1_rx_file: " << options.pu_uart1_rx_file << "\n";
  out << " * spio_uart0_rx_file: " << options.spio_uart0_rx_file << "\n";
  out << " * spio_uart1_rx_file: " << options.spio_uart1_rx_file << "\n";
  out << " * pu_uart0_tx_file: " << options.pu_uart0_tx_file << "\n";
  out << " * pu_uart1_tx_file: " << options.pu_uart1_tx_file << "\n";
  out << " * spio_uart0_tx_file: " << options.spio_uart0_tx_file << "\n";
  out << " * spio_uart1_tx_file: " << options.spio_uart1_tx_file << "\n";
  out << " * Log at pc: 0x" << options.log_at_pc << "\n";
  out << " * Stop log at pc: 0x" << options.stop_log_at_pc << "\n";
  out << " * Display trap info: " << options.display_trap_info << "\n";
  out << " * Gdb: " << options.gdb << "\n";
#ifdef SYSEMU_PROFILING
  out << " * Dump prof file: " << options.dump_prof_file << "\n";
#endif
#ifdef SYSEMU_DEBUG
  out << " * Debug: " << options.debug << "\n";
#endif
  out << "************************************************************************************\n";
}

void iatusPrint(bemu::System* chip) {
  const auto& iatus = chip->memory.pcie0_get_iatus();

  for (int i = 0, count = iatus.size(); i < count; ++i) {
    auto& iatu = iatus[i];
    auto base_addr = (uint64_t)iatu.upper_base_addr << 32 | iatu.lwr_base_addr;
    auto limit_addr = (uint64_t)iatu.uppr_limit_addr << 32 | iatu.limit_addr;
    auto target_addr = (uint64_t)iatu.upper_target_addr << 32 | iatu.lwr_target_addr;

    LOG_NOTHREAD(INFO, "iATU[%d].ctrl_1: 0x%x", i, iatu.ctrl_1);
    LOG_NOTHREAD(INFO, "iATU[%d].ctrl_2: 0x%x", i, iatu.ctrl_2);
    LOG_NOTHREAD(INFO, "iATU[%d].base_addr: 0x%" PRIx64, i, base_addr);
    LOG_NOTHREAD(INFO, "iATU[%d].limit_addr : 0x%" PRIx64, i, limit_addr);
    LOG_NOTHREAD(INFO, "iATU[%d].target_addr: 0x%" PRIx64, i, target_addr);
  }
}
bool iatuTranslate(bemu::System* chip, uint64_t pci_addr, uint64_t size, uint64_t& device_addr, uint64_t& access_size) {
  const auto& iatus = chip->memory.pcie0_get_iatus();

  for (int i = 0; i < iatus.size(); i++) {
    // Check REGION_EN (bit[31])
    if (((iatus[i].ctrl_2 >> 31) & 1) == 0)
      continue;

    // Check MATCH_MODE (bit[30]) to be Address Match Mode (0)
    if (((iatus[i].ctrl_2 >> 30) & 1) != 0) {
      LOG_NOTHREAD(FTL, "iATU[%d]: Unsupported MATCH_MODE", i);
    }

    uint64_t iatu_base_addr = (uint64_t)iatus[i].upper_base_addr << 32 | (uint64_t)iatus[i].lwr_base_addr;
    uint64_t iatu_limit_addr = (uint64_t)iatus[i].uppr_limit_addr << 32 | (uint64_t)iatus[i].limit_addr;
    uint64_t iatu_target_addr = (uint64_t)iatus[i].upper_target_addr << 32 | (uint64_t)iatus[i].lwr_target_addr;
    uint64_t iatu_size = iatu_limit_addr - iatu_base_addr + 1;

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
} // namespace

void SysEmuImp::set_system(bemu::System* system) {
  chip_ = system;
  g_Agent.chip = system;
}

void SysEmuImp::process() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!requests_.empty()) {
    SE_LOG(INFO) << "Processing request...";
    auto&& req = requests_.front();
    req();
    requests_.pop();
  }
}

void SysEmuImp::mmioRead(uint64_t address, size_t size, std::byte* dst) {
  std::promise<void> p;
  auto request = [=, &p]() {
    SE_LOG(INFO) << "Device memory read at: " << std::hex << address << " size: " << size << " host dst: " << dst;
    auto pci_addr = address;
    uint64_t host_access_offset = 0;
    int64_t readSize = size;
    while (readSize > 0) {

      uint64_t device_addr, access_size;

      if (!iatuTranslate(chip_, pci_addr, readSize, device_addr, access_size)) {
        LOG_NOTHREAD(WARN, "iATU: Could not find translation for host address: 0x%" PRIx64 ", size: 0x%" PRIx64,
                     pci_addr, readSize);
        iatusPrint(chip_);
        break;
      }

      chip_->memory.read(g_Agent, device_addr, access_size, dst + host_access_offset);

      pci_addr += access_size;
      host_access_offset += access_size;
      readSize -= access_size;
    }

    if (readSize > 0) {
      throw Exception(
        "Invalid IATU translation. Size too big to be covered fully by iATUs / translation failure. Address: " +
        std::to_string(address) + " size: " + std::to_string(readSize));
    }
    p.set_value();
  };
  std::unique_lock<std::mutex> lock(mutex_);
  requests_.emplace(std::move(request));
  lock.unlock();
  p.get_future().get();
}

void SysEmuImp::mmioWrite(uint64_t address, size_t size, const std::byte* src) {
  std::promise<void> p;
  auto request = [=, &p]() {
    SE_LOG(INFO) << "Device memory write at: " << std::hex << address << " size: " << size << " host src: " << src;
    auto pci_addr = address;
    uint64_t host_access_offset = 0;
    int64_t readSize = size;
    while (readSize > 0) {
      uint64_t device_addr, access_size;
      if (!iatuTranslate(chip_, pci_addr, size, device_addr, access_size)) {
        LOG_NOTHREAD(WARN, "iATU: Could not find translation for host address: 0x%" PRIx64 ", size: 0x%" PRIx64,
                     pci_addr, size);
        iatusPrint(chip_);
        break;
      }
      chip_->memory.write(g_Agent, device_addr, access_size, src + host_access_offset);

      pci_addr += access_size;
      host_access_offset += access_size;
      readSize -= access_size;
    }
    if (readSize > 0) {
      throw Exception(
        "Invalid IATU translation. Size too big to be covered fully by iATUs / translation failure. Address: " +
        std::to_string(address) + " size: " + std::to_string(readSize));
    }
    p.set_value();
  };
  std::unique_lock<std::mutex> lock(mutex_);
  requests_.emplace(std::move(request));
  lock.unlock();
  p.get_future().get();
}

void SysEmuImp::raiseDevicePuPlicPcieMessageInterrupt() {
  auto request = [=]() {
    SE_LOG(INFO) << "raiseDevicePuPlicPcieMessageInterrupt";
    LOG_NOTHREAD(INFO, "SysEmuImp: raise_device_interrupt(type = %s)", "PU");
    chip_->memory.pu_trg_pcie_mmm_int_inc(g_Agent);
  };
  std::lock_guard<std::mutex> lock(mutex_);
  requests_.emplace(std::move(request));
}

uint32_t SysEmuImp::waitForInterrupt(uint32_t bitmap) {
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
  LOG_NOTHREAD(INFO, "SysEmuImp: Raise Host (Count: %" PRId64 ") Interrupt Bitmap: (0x%" PRIx32 ")", ++raised_interrupt_count_,  bitmap);
  std::lock_guard<std::mutex> lock(mutex_);
  pendingInterruptsBitmask_ |= bitmap;
  condVar_.notify_all();
  return true;
}

void SysEmuImp::raiseDeviceSpioPlicPcieMessageInterrupt() {
  auto request = [=]() {
    SE_LOG(INFO) << "raiseDeviceSpioPlicPcieMessageInterrupt";
    LOG_NOTHREAD(INFO, "SysEmuImp: raise_device_interrupt(type = %s)", "SP");
    chip_->memory.pu_trg_pcie_ipi_trigger(g_Agent);
  };
  std::lock_guard<std::mutex> lock(mutex_);
  requests_.emplace(std::move(request));
}

bool SysEmuImp::host_memory_read(uint64_t host_addr, uint64_t size, void* data) {
  if (!hostListener_) {
    LOG_NOTHREAD(WARN, "%s: can't read host memory without hostListener", "SysEmuImp");
    return false;
  }
  hostListener_->memoryReadFromHost(host_addr, size, reinterpret_cast<std::byte*>(data));
  return true;
}

bool SysEmuImp::host_memory_write(uint64_t host_addr, uint64_t size, const void* data) {
  if (!hostListener_) {
    LOG_NOTHREAD(WARN, "%s: can't write host memory without hostListener", "SysEmuImp");
    return false;
  }
  hostListener_->memoryWriteFromHost(host_addr, size, reinterpret_cast<const std::byte*>(data));
  return true;
}

void SysEmuImp::notify_iatu_ctrl_2_reg_write(int pcie_id, uint32_t iatu, uint32_t value) {
  LOG_NOTHREAD(DEBUG, "SysEmuImp::notify_iatu_ctrl_2_reg_write: %d, 0x%x, 0x%x", pcie_id, iatu, value);
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
    chip_->set_emu_done(true);
    p.set_value();
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
}

SysEmuImp::SysEmuImp(const SysEmuOptions& options, const std::array<uint64_t, 8>& barAddresses,
                     IHostListener* hostListener)
  : hostListener_(hostListener) {
  const std::vector<std::string> preloadElfs = {options.bootromTrampolineToBL2ElfPath, options.spBL2ElfPath,
                                                options.masterMinionElfPath, options.machineMinionElfPath,
                                                options.workerMinionElfPath};

  sys_emu_cmd_options opts;

  if (!options.additionalOptions.empty()) {
    std::vector<const char*> extraOptions;
    extraOptions.emplace_back("dummyExecName");
    for (auto&& opt : options.additionalOptions) {
      extraOptions.emplace_back(opt.c_str());
    }
    auto parsed = sys_emu::parse_command_line_arguments(extraOptions.size(), const_cast<char**>(extraOptions.data()));
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
  opts.gdb = options.startGdb;

  opts.pu_uart0_tx_file = options.puUart0Path.empty() ? options.runDir + "/" + "pu_uart0_tx.log" : options.puUart0Path;
  opts.pu_uart1_tx_file = options.puUart1Path.empty() ? options.runDir + "/" + "pu_uart1_tx.log" : options.puUart1Path;

  /* Initialize SP UART0 port based on FIFO retargeting selection from caller*/
  if(!options.spUart0FifoOutPath.empty())
  {
    opts.spio_uart0_tx_file = options.spUart0FifoOutPath;
  }
  else
  {
    opts.spio_uart0_tx_file =
      options.spUart0Path.empty() ? options.runDir + "/" + "spio_uart0_tx.log" : options.spUart0Path;
  }

  if(!options.spUart0FifoInPath.empty())
  {
    opts.spio_uart0_rx_file = options.spUart0FifoInPath;
  }

  SE_LOG(INFO) << "SP TX FIFO path " << opts.spio_uart0_tx_file;
  SE_LOG(INFO) << "SP RX FIFO path " << opts.spio_uart0_rx_file;

  opts.spio_uart1_tx_file =
    options.spUart1Path.empty() ? options.runDir + "/" + "spio_uart1_tx.log" : options.spUart1Path;

  opts.elf_files = preloadElfs;
  opts.mem_check = options.memcheck;

  sysEmuThread_ = std::thread([opts, this, logfile = options.logFile]() {
    SE_LOG(INFO) << "Starting sysemu thread " << std::this_thread::get_id();
    std::ofstream log{logfile};
    bemu::log.setOutputStream(&log);
    logSysEmuOptions(log, opts);
    sys_emu emu(opts, this);
    emu.main_internal();
    SE_LOG(INFO) << "Ending sysemu thread " << std::this_thread::get_id();
  });

  // Wait until all the iATUs configured by BL2 have been enabled
  auto future = iatusReady_.get_future();
  future.get();

  SE_LOG(INFO) << "Calling pcieReady";
  hostListener_->pcieReady();
}

void SysEmuImp::stop() {
  running_ = false;
  // Wake host interrupt waiters
  condVar_.notify_all();
}
