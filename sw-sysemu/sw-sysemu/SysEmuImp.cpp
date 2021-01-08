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
#include "emu.h"
#include "memory/main_memory.h"
#include "utils.h"
#include <mutex>
using namespace emu;

namespace {
auto g_mem = &bemu::memory;

struct Agent : bemu::Agent {
  std::string name() const override {
    return "SysEmuImp";
  }
} g_Agent;

void iatusPrint() {
  const auto& iatus = g_mem->pcie_space.pcie0_dbi_slv.iatus;

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
bool iatuTranslate(uint64_t pci_addr, uint64_t size, uint64_t& device_addr, uint64_t& access_size) {
  const auto& iatus = g_mem->pcie_space.pcie0_dbi_slv.iatus;

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

void SysEmuImp::setHostListener(IHostListener* hostListener) {
  hostListener_ = hostListener;
}

void SysEmuImp::process() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!requests_.empty()) {
    auto&& req = requests_.front();
    req();
    requests_.pop();
  }
}

void SysEmuImp::mmioRead(uint64_t address, size_t size, std::byte* dst) {

  auto request = [=]() {
    SE_LOG(INFO) << "Device memory read at: " << std::hex << address << " size: " << size << " host dst: " << dst;
    auto pci_addr = address;
    uint64_t host_access_offset = 0;
    int64_t readSize = size;
    while (readSize > 0) {

      uint64_t device_addr, access_size;

      if (!iatuTranslate(pci_addr, readSize, device_addr, access_size)) {
        LOG_NOTHREAD(WARN, "iATU: Could not find translation for host address: 0x%" PRIx64 ", size: 0x%" PRIx64,
                     pci_addr, readSize);
        iatusPrint();
        break;
      }

      g_mem->read(g_Agent, device_addr, access_size, dst + host_access_offset);

      pci_addr += access_size;
      host_access_offset += access_size;
      readSize -= access_size;
    }

    if (readSize > 0) {
      throw Exception(
        "Invalid IATU translation. Size too big to be covered fully by iATUs / translation failure. Address: " +
        std::to_string(address) + " size: " + std::to_string(readSize));
    }
  };
  std::lock_guard<std::mutex> lock(mutex_);
  requests_.emplace(std::move(request));
}

void SysEmuImp::mmioWrite(uint64_t address, size_t size, const std::byte* src) {
  auto request = [=]() {
    SE_LOG(INFO) << "Device memory write at: " << std::hex << address << " size: " << size << " host src: " << src;
    auto pci_addr = address;
    uint64_t host_access_offset = 0;
    int64_t readSize = size;
    while (readSize > 0) {
      uint64_t device_addr, access_size;
      if (!iatuTranslate(pci_addr, size, device_addr, access_size)) {
        LOG_NOTHREAD(WARN, "iATU: Could not find translation for host address: 0x%" PRIx64 ", size: 0x%" PRIx64,
                     pci_addr, size);
        iatusPrint();
        break;
      }
      g_mem->write(g_Agent, device_addr, access_size, src + host_access_offset);

      pci_addr += access_size;
      host_access_offset += access_size;
      readSize -= access_size;
    }
    if (readSize > 0) {
      throw Exception(
        "Invalid IATU translation. Size too big to be covered fully by iATUs / translation failure. Address: " +
        std::to_string(address) + " size: " + std::to_string(readSize));
    }
  };
  std::lock_guard<std::mutex> lock(mutex_);
  requests_.emplace(std::move(request));
}

void SysEmuImp::raiseDevicePuPlicPcieMessageInterrupt() {
  auto request = [=]() {
    SE_LOG(INFO) << "raiseDevicePuPlicPcieMessageInterrupt";
    LOG_NOTHREAD(INFO, "SysEmuImp: raise_device_interrupt(type = %s)", "PU");
    uint32_t trigger = 1;
    g_mem->pu_mbox_space.pu_trg_pcie.write(g_Agent, bemu::MMM_INT_INC, sizeof(trigger),
                                           reinterpret_cast<bemu::MemoryRegion::const_pointer>(&trigger));
  };
  std::lock_guard<std::mutex> lock(mutex_);
  requests_.emplace(std::move(request));
}

void SysEmuImp::waitForInterrupt(uint32_t bitmap) {
  std::unique_lock<std::mutex> lock(mutex_);
  if (!(pendingInterruptsBitmask_ & bitmap)) {
    condVar_.wait(lock, [this, bitmap]() { return !running_ || (bitmap & pendingInterruptsBitmask_); });
  }
  if (running_) {
    pendingInterruptsBitmask_ &= ~bitmap;
  }
}
bool SysEmuImp::raise_host_interrupt(uint32_t bitmap) {
  LOG_NOTHREAD(INFO, "SysEmuImp: Raise Host Interrupt (0x%" PRIx32 ")", bitmap);
  std::lock_guard<std::mutex> lock(mutex_);
  pendingInterruptsBitmask_ |= bitmap;
  condVar_.notify_all();
}

void SysEmuImp::raiseDeviceSpioPlicPcieMessageInterrupt() {
  auto request = [=]() {
    SE_LOG(INFO) << "raiseDeviceSpioPlicPcieMessageInterrupt";
    LOG_NOTHREAD(INFO, "SysEmuImp: raise_device_interrupt(type = %s)", "SP");
    uint32_t trigger = 1;
    g_mem->pu_mbox_space.pu_trg_pcie.write(g_Agent, bemu::IPI_TRIGGER, sizeof(trigger),
                                           reinterpret_cast<bemu::MemoryRegion::const_pointer>(&trigger));
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

SysEmuImp::~SysEmuImp() {
  std::unique_lock<std::mutex> lock(mutex_);
  running_ = false;
  hostListener_ = nullptr;
  while (!requests_.empty()) {
    requests_.pop();
  }
  pendingInterruptsBitmask_ = 0;
  lock.unlock();
  condVar_.notify_all();
}

SysEmuImp::SysEmuImp(const sys_emu_cmd_options& cmdOptions) {
  emu.main_internal(cmdOptions);
}