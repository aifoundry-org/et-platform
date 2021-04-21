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
#include <future>
#include <sw-sysemu/ISysEmu.h>

namespace dev {
class SysEmuHostListener : public emu::ISysEmu::IHostListener {
public:
  // IHostListener interface
  void memoryReadFromHost(uint64_t address, size_t size, std::byte* dst) override;
  void memoryWriteFromHost(uint64_t address, size_t size, const std::byte* src) override;
  void pcieReady() override;

  std::future<void> getPcieReadyFuture();

private:
  std::promise<void> pcieReady_;
};
} // namespace dev