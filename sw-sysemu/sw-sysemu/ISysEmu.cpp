//******************************************************************************
// Copyright (c) 2025 Ainekko, Co.
// SPDX-License-Identifier: Apache-2.0
//------------------------------------------------------------------------------

#include "sw-sysemu/ISysEmu.h"
#include "SysEmuImp.h"
#include "utils.h"

using namespace emu;

void ISysEmu::IHostListener::memoryReadFromHost(uint64_t address, size_t size, std::byte* dst) {
  auto src = reinterpret_cast<std::byte*>(address);
  std::copy(src, src + size, dst);
}

void ISysEmu::IHostListener::memoryWriteFromHost(uint64_t address, size_t size, const std::byte* src) {
  auto dst = reinterpret_cast<std::byte*>(address);
  std::copy(src, src + size, dst);
}
void ISysEmu::IHostListener::onSysemuFatalError(const std::string& error) {
  SE_LOG(ERROR) << "Got a FATAL error from sysemu: " << error;
}

std::unique_ptr<ISysEmu> ISysEmu::create(const SysEmuOptions& options, const std::array<uint64_t, 8>& barAddresses,
                                         IHostListener* hostListener) {
  return std::make_unique<SysEmuImp>(options, barAddresses, hostListener);
}
