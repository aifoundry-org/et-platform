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
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>

namespace emu {
class SysEmuImp : public ISysEmu, public api_communicate {
public:
  // ISysEmu interface
  void setHostListener(IHostListener* hostListener) override;
  void mmioRead(uint64_t address, size_t size, std::byte* dst) override;
  void mmioWrite(uint64_t address, size_t size, const std::byte* src) override;
  void raiseDevicePuPlicPcieMessageInterrupt() override;
  void raiseDeviceSpioPlicPcieMessageInterrupt() override;
  void waitForInterrupt(uint32_t bitmask) override;

  // api_communicate interface
  void process() override;
  bool raise_host_interrupt(uint32_t bitmap) override;
  bool host_memory_read(uint64_t host_addr, uint64_t size, void* data) override;
  bool host_memory_write(uint64_t host_addr, uint64_t size, const void* data) override;

  ~SysEmuImp();

  SysEmuImp(const sys_emu_cmd_options& cmdOptions);

private:
  sys_emu emu;

  std::mutex mutex_;
  bool running_ = true;
  uint32_t pendingInterruptsBitmask_ = 0;
  std::condition_variable condVar_;
  IHostListener* hostListener_ = nullptr;
  std::queue<std::function<void()>> requests_;
};
} // namespace emu