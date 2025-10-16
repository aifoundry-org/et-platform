//******************************************************************************
// Copyright (c) 2025 Ainekko, Co.
// SPDX-License-Identifier: Apache-2.0
//------------------------------------------------------------------------------

#pragma once
#include "SysEmuOptions.h"
#include "ISysEmuExport.hpp"

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <stdint.h>


namespace emu {
struct SW_SYSEMU_EXPORT Exception : public std::runtime_error {
  using std::runtime_error::runtime_error;
};
class SW_SYSEMU_EXPORT ISysEmu {
public:
  class IHostListener {
  public:
    virtual void pcieReady() = 0;

    // we provide a simple implementation of read and write functions
    virtual void memoryReadFromHost(uint64_t address, size_t size, std::byte* dst);
    virtual void memoryWriteFromHost(uint64_t address, size_t size, const std::byte* src);
    virtual void onSysemuFatalError(const std::string& error);
    virtual ~IHostListener() = default;
  };

  virtual uint32_t waitForInterrupt(uint32_t bitmask) = 0;
  virtual void mmioRead(uint64_t address, size_t size, std::byte* dst) = 0;
  virtual void mmioWrite(uint64_t address, size_t size, const std::byte* src) = 0;
  virtual void raiseDevicePuPlicPcieMessageInterrupt() = 0;
  virtual void raiseDeviceSpioPlicPcieMessageInterrupt() = 0;
  virtual void stop() = 0;
  virtual void pause() = 0;
  virtual void resume() = 0;
  virtual ~ISysEmu() = default;

  static std::unique_ptr<ISysEmu> create(const SysEmuOptions& options, const std::array<uint64_t, 8>& barAddresses,
                                         IHostListener* hostListener);
};
}  // namespace emu
