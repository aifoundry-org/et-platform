//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#ifndef ET_RUNTIME_MEMORY_MANAGER_H
#define ET_RUNTIME_MEMORY_MANAGER_H

#include "Common/ErrorTypes.h"
#include "Support/ErrorOr.h"

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <unordered_map>

struct etrtPointerAttributes;

namespace et_runtime {

class Device;
class AbstractMemoryPtr;
class DeviceMemoryPtr;
class HostMemoryPtr;

namespace device {

struct LinearMemoryAllocator;

/// @brief MemoryManager, responsible for tracking device memory use.
class MemoryManager {
public:
  static constexpr size_t kHostMemRegionSize = (1 << 20) * 256;

  MemoryManager(Device &dev);
  ~MemoryManager();

  /// For now have separate init/deinit functions we need to delegate to the
  /// device the responsibility to initialize the different components in the
  /// right order
  bool init();
  bool deInit();

  uintptr_t ramBase() const;

  /// FIXME SW-1292
  ///
  /// @brief  Allocate memory on the Host.
  ///
  /// Take a byte count and return an error or a @ref HostMemoryPtr to that
  /// number of directly DMA-able bytes of memory on the Host. This will return
  /// a failure indication if it is not possible to meet the given request.
  /// @ref HostMemoryPtr provides unique_ptr semantics and memory gets
  /// automatically deallocated then lifetime of the pointer ends.
  ///
  /// @param[in]  size  The number of bytes of memory that should be allocated
  /// on the Host.
  /// @return  Error or @HostMemoryPtr to the allocated memory location on the
  /// host
  ///
  ErrorOr<HostMemoryPtr> mallocHost();

  /// FIXME SW-1292
  ///
  /// @brief Allocate memory on the Device.
  ///
  /// Take a byte count and return a @ref DeviceMemoryPtr to
  /// that number of (contiguous, long-word aligned) bytes of shared global
  /// memory on the calling thread's currently attached Device. The allocated
  /// Device memory region is associated with the calling thread and will be
  /// automatically freed when the calling thread exits. This will return a
  /// failure indication if it is not possible to meet the given request.
  ///
  /// The memory on the device gets deallocated automatically when the lifetime
  /// of the @ref DeviceMemoryPtr object ends.
  ///
  /// @param[in]  size  The number of bytes of memory that should be allocated
  /// on the Device.
  /// @return  ErrorOr ( etrtErrorInvalidValue, etrtErrorMemoryAllocation ) or a
  /// valid pointer
  ErrorOr<DeviceMemoryPtr> mallocDevice(size_t size);

  /// @brief Allocate pinned memory on the host
  etrtError mallocHost(void **ptr, size_t size);
  /// @brief Deallocate host memory
  etrtError freeHost(void *ptr);
  /// @brief Reserve a memory region starting at address ptr
  etrtError reserveMemory(void *ptr, size_t size);
  /// @brief Allocate memory on the device
  etrtError malloc(void **devPtr, size_t size);
  /// @brief Free device memory.
  etrtError free(void *devPtr);
  /// @brief For a given pointer get its attributes
  etrtError pointerGetAttributes(struct etrtPointerAttributes *attributes,
                                 const void *ptr);

  /// @brief Return true iff this is a host pointer
  bool isPtrAllocatedHost(const void *ptr);
  /// @brief Return true iff this is a device pointer
  bool isPtrAllocatedDev(const void *ptr);
  /// @brief Return true iff this points in a device region
  bool isPtrInDevRegion(const void *ptr);

private:
  void initMemRegions();
  void uninitMemRegions();

  std::unordered_map<uint8_t *, std::unique_ptr<uint8_t>> host_mem_region_;
  std::unique_ptr<LinearMemoryAllocator> dev_mem_region_;
  std::unique_ptr<LinearMemoryAllocator> kernels_dev_mem_region_;
  Device &device_;
};
} // namespace device
} // namespace et_runtime

#endif // ET_RUNTIME_MEMORY_MANAGER_H
