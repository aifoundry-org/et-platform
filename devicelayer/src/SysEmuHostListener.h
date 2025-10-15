//******************************************************************************
// Copyright (c) 2025 Ainekko, Co.
// SPDX-License-Identifier: Apache-2.0
//------------------------------------------------------------------------------

#pragma once
#include <future>
#include <optional>
#include <sw-sysemu/ISysEmu.h>

namespace dev {
class SysEmuHostListener : public emu::ISysEmu::IHostListener {
public:
  // IHostListener interface
  void memoryReadFromHost(uint64_t address, size_t size, std::byte* dst) override;
  void memoryWriteFromHost(uint64_t address, size_t size, const std::byte* src) override;
  void pcieReady() override;

  void onSysemuFatalError(const std::string& error) override;

  std::optional<std::string> getSysemuLastError() const;

  std::future<void> getPcieReadyFuture();

private:
  std::optional<std::string> sysemuLastError_;
  std::promise<void> pcieReady_;
};
} // namespace dev
