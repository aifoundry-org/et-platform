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

#include "esperanto/runtime/Common/CommonTypes.h"
#include "esperanto/runtime/Common/ErrorTypes.h"
#include "esperanto/runtime/Support/ErrorOr.h"

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
class Module;

namespace device {

namespace memory_management {
class MemoryManagerInternals;
}

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

  /// @brief Reserve a memory region starting at address ptr
  etrtError reserveMemoryCode(uintptr_t ptr, size_t size);

  /// @brief Allocate memory on the device
  etrtError malloc(void **devPtr, size_t size);

  /// @brief Free device memory.
  etrtError free(void *devPtr);

  /// @brief Return true iff this points in a device region
  bool isPtrInDevRegion(const void *ptr);

  /// @brief Run memory-manager sanity check
  bool runSanityCheck() const;

  /// @brief Dump the memory manager state in the runtime log
  void recordState() const;

private:
  friend class ::et_runtime::Module;

  static constexpr int64_t DATA_ALIGNMENT =
      1ULL << 13; ///< 8KB alignment requirement

  /// Code allocation functions are exposed only in the Module class that is a
  /// friend

  /// @brief Allocate buffer in the code region on the device
  etrtError mallocCode(void **devPtr, size_t size);

  /// @brief Deallocate code buffer
  etrtError freeCode(void *devPtr);

  void initMemRegions();
  void uninitMemRegions();

  static constexpr uint64_t CODE_SIZE = 1ULL << 32; // 4GB

  std::unique_ptr<memory_management::MemoryManagerInternals> impl_;

  using buffer_map_t = std::unordered_map<BufferOffsetTy, BufferID>;

  Device &device_;
  buffer_map_t
      data_buffer_map_; ///< Map from a data buffer address to a BufferID
  buffer_map_t
      code_buffer_map_; ///< Map from a code buffer address to a BufferID
};
} // namespace device
} // namespace et_runtime

#endif // ET_RUNTIME_MEMORY_MANAGER_H
