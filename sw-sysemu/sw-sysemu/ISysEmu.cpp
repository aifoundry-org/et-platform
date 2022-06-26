//******************************************************************************
// Copyright (C) 2021, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
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