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
#include "esperanto/runtime/Core/Memory.h"
#include "esperanto/runtime/Support/ErrorOr.h"

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <unordered_map>

struct etrtPointerAttributes;

class TestMemoryManager;

namespace et_runtime {

class Device;
class AbstractMemoryPtr;
class DeviceBuffer;
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

  MemoryManager(Device *dev);
  virtual ~MemoryManager();

  /// For now have separate init/deinit functions we need to delegate to the
  /// device the responsibility to initialize the different components in the
  /// right order
  bool init();
  bool deInit();

  uintptr_t ramBase() const;

  /// @brief Allocate memory on the Device.
  ///
  /// Take a byte count and return a @ref DeviceBuffer to
  /// that number of (contiguous, long-word aligned) bytes of shared global
  /// memory on the calling thread's currently attached Device. The allocated
  /// Device memory region is associated with the calling thread and will be
  /// automatically freed when the calling thread exits. This will return a
  /// failure indication if it is not possible to meet the given request.
  ///
  /// The memory on the device gets deallocated automatically when the lifetime
  /// of the @ref DeviceBuffer object ends.
  ///
  /// @param[in]  size  The number of bytes of memory that should be allocated
  /// on the Device.
  /// @param[in] info Meta-data related to this buffer.
  /// @return  ErrorOr ( etrtErrorInvalidValue, etrtErrorMemoryAllocation ) or a
  /// valid DeviceBuffer object
  ErrorOr<DeviceBuffer> malloc(size_t size, const BufferDebugInfo &info);

  /// @brief Allocate a Constant type of buffer , this forces the allocation to
  /// happen starting the free-list search from the "left" side of the
  /// data-region.
  ///
  /// @param[in]  size  The number of bytes of memory that should be allocated
  /// on the Device.
  /// @param[in] info Meta-data related to this buffer.
  /// @return  ErrorOr ( etrtErrorInvalidValue, etrtErrorMemoryAllocation ) or a
  /// valid DeviceBuffer object
  ErrorOr<DeviceBuffer> mallocConstant(size_t size,
                                       const BufferDebugInfo &info);

  /// @brief Allocate a Placeholder type of buffer , this forces the allocation
  /// to happen starting the free-list search from the "right" side of the
  /// data-region.
  ///
  /// @param[in]  size  The number of bytes of memory that should be allocated
  /// on the Device.
  /// @param[in] info Meta-data related to this buffer.
  /// @return  ErrorOr ( etrtErrorInvalidValue, etrtErrorMemoryAllocation ) or a
  /// valid DeviceBuffer object
  ErrorOr<DeviceBuffer> mallocPlaceholder(size_t size,
                                          const BufferDebugInfo &info);
  // TODO: REMOVE
  /// @brief Reserve a memory region starting at address ptr
  etrtError reserveMemoryCode(uintptr_t ptr, size_t size);

  // TODO: REMOVE
  /// @brief Allocate memory on the device
  etrtError malloc(void **devPtr, size_t size);

  // TODO: REMOVE
  /// @brief Free device memory.
  etrtError free(void *devPtr);

  // TODO: REMOVE
  /// @brief Return true iff this points in a device region
  bool isPtrInDevRegion(const void *ptr);

  /// @brief Run memory-manager sanity check
  bool runSanityCheck() const;

  /// @brief Dump the memory manager state in the runtime log
  void recordState() const;

protected:
  friend class ::et_runtime::Module;
  friend class ::et_runtime::DeviceBuffer;
  friend class ::TestMemoryManager;

  static constexpr int64_t DATA_ALIGNMENT =
      1ULL << 13; ///< 8KB alignment requirement

  /// Code allocation functions are exposed only in the Module class that is a
  /// friend

  // REMOVE revisit
  /// @brief Allocate buffer in the code region on the device
  etrtError mallocCode(void **devPtr, size_t size);

  /// @brief Allocate a Code type of buffer , this forces the allocation to
  /// happen starting the free-list search from the "right" side of the
  /// data-region. This function is not public and should be alloced to be
  /// callled only from the code management modules. The user shold not be
  /// allows to allocate memory in the code-region on the device.
  ///
  /// @param[in]  size  The number of bytes of memory that should be allocated
  /// on the Device.
  /// @param[in] info Meta-data related to this buffer.
  /// @return  ErrorOr ( etrtErrorInvalidValue, etrtErrorMemoryAllocation ) or a
  /// valid DeviceBuffer object
  ErrorOr<DeviceBuffer> mallocCode(size_t size, const BufferDebugInfo &info);

  // Revisit
  /// @brief Deallocate code buffer
  etrtError freeCode(void *devPtr);

  /// @brief Deallocate code buffer
  /// This function is private and should be be called directly by the user
  /// but only DeviceBuffer object when ther reference count reaches zero
  virtual etrtError freeCode(BufferID bid);

  /// @brief Deallocate a data buffer
  /// This function is private and should be be called directly by the user
  /// but only DeviceBuffer object when ther reference count reaches zero
  virtual etrtError freeData(BufferID bid);

  void initMemRegions();
  void uninitMemRegions();

  static constexpr uint64_t CODE_SIZE = 1ULL << 32; // 4GB

  std::unique_ptr<memory_management::MemoryManagerInternals> impl_;

  using buffer_map_t = std::unordered_map<BufferOffsetTy, BufferID>;

  Device *device_;
  buffer_map_t
      data_buffer_map_; ///< Map from a data buffer address to a BufferID
  buffer_map_t
      code_buffer_map_; ///< Map from a code buffer address to a BufferID

  Deallocator
      code_deallocator_; ///< Callable object to allow deallocating code buffers
  Deallocator
      data_deallocator_; ///< Callable object to allow deallocating data buffers
};
} // namespace device
} // namespace et_runtime

#endif // ET_RUNTIME_MEMORY_MANAGER_H
