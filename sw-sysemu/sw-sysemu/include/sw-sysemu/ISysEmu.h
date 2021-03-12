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
#include "SysEmuOptions.h"
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <stdint.h>

namespace emu {
struct Exception : public std::runtime_error {
  using std::runtime_error::runtime_error;
};
class ISysEmu {
public:
  class IHostListener {
  public:
    virtual void pcieReady() = 0;

    // we provide a simple implementation of read and write functions
    virtual void memoryReadFromHost(uint64_t address, size_t size, std::byte* dst);
    virtual void memoryWriteFromHost(uint64_t address, size_t size, const std::byte* src);
    virtual ~IHostListener() = default;
  };

  virtual uint32_t waitForInterrupt(uint32_t bitmask) = 0;
  virtual void mmioRead(uint64_t address, size_t size, std::byte* dst) = 0;
  virtual void mmioWrite(uint64_t address, size_t size, const std::byte* src) = 0;
  virtual void raiseDevicePuPlicPcieMessageInterrupt() = 0;
  virtual void raiseDeviceSpioPlicPcieMessageInterrupt() = 0;
  virtual void stop() = 0;
  virtual ~ISysEmu() = default;

  static std::unique_ptr<ISysEmu> create(const SysEmuOptions& options, const std::array<uint64_t, 8>& barAddresses,
                                         IHostListener* hostListener);
};
}  // namespace emu