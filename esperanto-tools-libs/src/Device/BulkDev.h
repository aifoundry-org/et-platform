//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#ifndef ET_RUNTIME_DEVICE_BULKDEV_H
#define ET_RUNTIME_DEVICE_BULKDEV_H

#include "CharDevice.h"

#include "Support/TimeHelpers.h"

#include <chrono>
#include <experimental/filesystem>

namespace et_runtime {
namespace device {

///
/// @brief Helper class that encapsulates the interactions with a character
/// device that implements the Bulk data transfers whether they are MMIO or
/// DMA.
class BulkDev final : public CharacterDevice {
public:
  static constexpr uintptr_t DMA_BASE_ADDR = 0x8200000000;

  /// @brief Construct a Bulk device passing the path to it
  BulkDev(const std::experimental::filesystem::path &char_dev);
  /// @bried Copying the device has been disabled
  BulkDev(const BulkDev &rhs) = delete;
  /// @brief Move constructor for the device
  BulkDev(BulkDev &&other);

  /// @brief Do a MMIO write
  bool write_mmio(uintptr_t addr, const void *data, ssize_t size);

  /// @brief Do an MMIO read
  ssize_t read_mmio(uintptr_t addr, void *data, ssize_t size);

  /// @brief Do an DMA write
  bool write_dma(uintptr_t addr, const void *data, ssize_t size);

  /// @brief Do an DMA read
  ssize_t read_dma(uintptr_t addr, void *data, ssize_t size);

  /// @brief Return the SOC absolute base address of the backing storage of the
  /// device
  // FIXME remove this hard-coded value
  uintptr_t baseAddr() const { return 0x8100000000; }

  /// @brief Return the SOC DRAM size
  uintptr_t size() const { return size_; }

private:
  uintptr_t base_addr_ =
      0; ///< Base address of the device to be used to translate
         ///< absolute adresses FIXME: We should pull this information
         ///< form an IOCTL to the character device along with the size

  uintptr_t size_; ///< Size of the backing device DRAM we can access

  /// @brief Query the device for the base address
  uintptr_t queryBaseAddr();

  /// @brief Querh the device for the size;
  uintptr_t querySize();

  /// @brief Switch device to use MMIO
  bool enable_mmio();

  /// @brief Switch device to use DMA
  bool enable_dma();
};
} // namespace device

} // namespace et_runtime

#endif // ET_RUNTIME_DEVICE_BULKDEV_H
