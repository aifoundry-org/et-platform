//******************************************************************************
// Copyright (C) 2020, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#pragma once
#include "api_communicate.h"
#include "sw-sysemu/ISysEmu.h"
#include "sys_emu.h"
#include "system.h"
#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>

namespace emu {
class SysEmuImp : public ISysEmu, public api_communicate {
public:
  // ISysEmu interface
  void mmioRead(uint64_t address, size_t size, std::byte* dst) override;
  void mmioWrite(uint64_t address, size_t size, const std::byte* src) override;
  void raiseDevicePuPlicPcieMessageInterrupt() override;
  void raiseDeviceSpioPlicPcieMessageInterrupt() override;
  uint32_t waitForInterrupt(uint32_t bitmask) override;
  void stop() override;


  // api_communicate interface
  void set_system(bemu::System* system) override;
  void process() override;
  bool raise_host_interrupt(uint32_t bitmap) override;
  bool host_memory_read(uint64_t host_addr, uint64_t size, void* data) override;
  bool host_memory_write(uint64_t host_addr, uint64_t size, const void* data) override;
  void notify_iatu_ctrl_2_reg_write(int pcie_id, uint32_t iatu, uint32_t value) override;

  ~SysEmuImp();

  SysEmuImp(const SysEmuOptions& options, const std::array<uint64_t, 8>& barAddresses, IHostListener* hostListener);

private:
  bemu::System* chip_;
  std::thread sysEmuThread_;

  std::mutex mutex_;
  bool running_ = true;
  uint32_t pendingInterruptsBitmask_ = 0;
  std::condition_variable condVar_;
  IHostListener* hostListener_ = nullptr;
  std::queue<std::function<void()>> requests_;
  std::promise<void> iatusReady_;
};
} // namespace emu
