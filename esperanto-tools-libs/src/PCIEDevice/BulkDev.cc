//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "BulkDev.h"

#include "esperanto/runtime/Support/Logging.h"

#include <cassert>
#include <thread>
#include <unistd.h>

namespace et_runtime {
namespace device {

BulkDev::BulkDev(const std::experimental::filesystem::path &char_dev)
    : CharacterDevice(char_dev) {
  // FIXME enable using the value of the IOCTL
  auto input_base_addr = queryBaseAddr();
  RTDEBUG << "Device base address : 0x" << std::hex << input_base_addr << "\n";
#if ENABLE_DEVICE_FW
  size_ = HOST_MANAGED_DRAM_END - HOST_MANAGED_DRAM_START;
#endif
  RTDEBUG << "Device DRAM size : 0x" << std::hex << size_ << "\n";
}

BulkDev::BulkDev(BulkDev &&other) : CharacterDevice(std::move(other)) {}

bool BulkDev::enable_mmio() {
  auto success = ioctl_set(SET_BULK_CFG, static_cast<uint32_t>(BULK_CFG_MMIO));
  if (!success) {
    RTERROR << "Failed to enable MMIO \n";
    std::terminate();
  }
  RTDEBUG << "Bulk MMIO \n";
  return true;
}

bool BulkDev::enable_dma() {
  auto success = ioctl_set(SET_BULK_CFG, static_cast<uint32_t>(BULK_CFG_DMA));
  if (!success) {
    RTERROR << "Failed to enable DMA \n";
    std::terminate();
  }
  RTDEBUG << "Bulk DMA \n";
  return true;
}

uintptr_t BulkDev::queryBaseAddr() {

  uintptr_t arg;
  auto [valid, res] = ioctl(GET_DRAM_BASE, &arg);
  if (!valid) {
    RTERROR << "Failed to get DRAM Base address \n";
    std::terminate();
  }
  return res;
}

uintptr_t BulkDev::querySize() {
  uintptr_t arg;
  auto [valid, res] = ioctl(GET_DRAM_SIZE, &arg);
  if (!valid) {
    RTERROR << "Failed to get DRAM size \n";
    std::terminate();
  }
  return res;
}

bool BulkDev::write_mmio(uintptr_t addr, const void *data, ssize_t size) {
  auto success = enable_mmio();
  assert(success);
  return write(addr, data, size);
}

ssize_t BulkDev::read_mmio(uintptr_t addr, void *data, ssize_t size) {
  auto success = enable_mmio();
  assert(success);
  return read(addr, data, size);
}

bool BulkDev::write_dma(uintptr_t addr, const void *data, ssize_t size) {
  auto success = enable_dma();
  assert(success);
  return write(addr, data, size);
}

ssize_t BulkDev::read_dma(uintptr_t addr, void *data, ssize_t size) {
  auto success = enable_dma();
  assert(success);
  return read(addr, data, size);
}

} // namespace device
} // namespace et_runtime
