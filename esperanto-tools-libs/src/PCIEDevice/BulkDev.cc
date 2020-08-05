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

#include "Tracing/Tracing.h"
#include <cassert>
#include <et_ioctl.h>
#include <thread>
#include <unistd.h>

namespace et_runtime {
namespace device {

BulkDev::BulkDev(const std::experimental::filesystem::path &char_dev)
    : CharacterDevice(char_dev) {
  // FIXME enable using the value of the IOCTL
  auto input_base_addr = queryBaseAddr();
  size_ = HOST_MANAGED_DRAM_END - HOST_MANAGED_DRAM_START;
  TRACE_PCIeDevice_bulk_device_register(input_base_addr, size_);
}

BulkDev::BulkDev(BulkDev &&other) : CharacterDevice(std::move(other)) {}

bool BulkDev::enable_mmio() {
  auto success = ioctl_set(SET_BULK_CFG, static_cast<uint32_t>(BULK_CFG_MMIO));
  if (!success) {
    RTERROR << "Failed to enable MMIO \n";
    std::terminate();
  }
  TRACE_PCIeDevice_enable_mmio();
  return true;
}

bool BulkDev::enable_dma() {
  auto success = ioctl_set(SET_BULK_CFG, static_cast<uint32_t>(BULK_CFG_DMA));
  if (!success) {
    RTERROR << "Failed to enable DMA \n";
    std::terminate();
  }
  TRACE_PCIeDevice_enable_dma();
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
  TRACE_PCIeDevice_mmio_write_start(addr, (uint64_t)data, size);
  auto res = write(addr, data, size);
  TRACE_PCIeDevice_mmio_write_end();
  return res;
}

ssize_t BulkDev::read_mmio(uintptr_t addr, void *data, ssize_t size) {
  auto success = enable_mmio();
  assert(success);
  TRACE_PCIeDevice_mmio_read_start(addr, (uint64_t)data, size);
  auto res = read(addr, data, size);
  TRACE_PCIeDevice_mmio_read_end();
  return res;
}

bool BulkDev::write_dma(uintptr_t addr, const void *data, ssize_t size) {
  auto success = enable_dma();
  assert(success);
  TRACE_PCIeDevice_dma_write_start(addr, (uint64_t)data, size);
  auto res = write(addr, data, size);
  TRACE_PCIeDevice_dma_write_end();
  return res;
}

ssize_t BulkDev::read_dma(uintptr_t addr, void *data, ssize_t size) {
  auto success = enable_dma();
  assert(success);
  TRACE_PCIeDevice_dma_read_start(addr, (uint64_t)data, size);
  auto res = read(addr, data, size);
  TRACE_PCIeDevice_dma_read_end();
  return res;
}

} // namespace device
} // namespace et_runtime
