//******************************************************************************
// Copyright (c) 2025 Ainekko, Co.
// SPDX-License-Identifier: Apache-2.0
//------------------------------------------------------------------------------

#pragma once
#include "api_communicate.h"
#include "sw-sysemu/ISysEmu.h"
#include "sys_emu.h"
#include "system.h"
#include "agent.h"
#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>

namespace emu {
class SysEmuImp : public ISysEmu, public api_communicate {
public:

  SysEmuImp(const SysEmuImp&) = delete;
  SysEmuImp& operator=(const SysEmuImp&) = delete;
  SysEmuImp(SysEmuImp&&) = delete;
  SysEmuImp& operator=(SysEmuImp&&) = delete;

  // ISysEmu interface
  void mmioRead(uint64_t address, size_t size, std::byte* dst) override;
  void mmioWrite(uint64_t address, size_t size, const std::byte* src) override;
  void raiseDevicePuPlicPcieMessageInterrupt() override;
  void raiseDeviceSpioPlicPcieMessageInterrupt() override;
  uint32_t waitForInterrupt(uint32_t bitmask) override;
  void stop() override;
  void pause() override;
  void resume() override;


  // api_communicate interface
  void set_system(bemu::System* system) override;
  void process() override;
  bool raise_host_interrupt(uint32_t bitmap) override;
  bool host_memory_read(uint64_t host_addr, uint64_t size, void* data) override;
  bool host_memory_write(uint64_t host_addr, uint64_t size, const void* data) override;
  void notify_iatu_ctrl_2_reg_write(int pcie_id, uint32_t iatu, uint32_t value) override;
  void notify_fatal_error(const std::string& error) override;

  ~SysEmuImp();

  SysEmuImp(const SysEmuOptions& options, const std::array<uint64_t, 8>& barAddresses, IHostListener* hostListener);

private:
  bemu::System* chip_ = nullptr;
  std::thread sysEmuThread_;
  std::exception_ptr sysEmuError_ = nullptr;
  bemu::Noagent agent_{chip_, "SysEmuImp"};

  std::mutex mutex_;
  bool running_ = true;
  bool should_pause_ = false;
  uint32_t pendingInterruptsBitmask_ = 0;
  uint64_t raised_interrupt_count_ = 0;
  std::condition_variable condVar_;
  IHostListener* hostListener_ = nullptr;
  std::queue<std::function<void()>> requests_;
  std::promise<void> iatusReady_;
};
} // namespace emu
