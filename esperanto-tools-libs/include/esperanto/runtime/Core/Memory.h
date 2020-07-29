//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#ifndef ET_RUNTIME_MEMORY_H
#define ET_RUNTIME_MEMORY_H

#include "esperanto/runtime/Common/CommonTypes.h"
#include "esperanto/runtime/Common/ErrorTypes.h"

#include <atomic>
#include <functional>

class TestDeviceBuffer;

namespace et_runtime {

class Module;
class Device;

/// @enum Enumeration of the different types of buffers we can have in the
/// system
enum class BufferType { Unkown = 0, Host, Device };

/// @class RefCount Memory.h
///
/// Class implementing basic reference counting
class RefCounter {
public:
  using value_type = int64_t;

  RefCounter() : val_(0) {}

  /// @brief Return the value of the counter
  value_type val() { return val_.load(); }

  /// @brief Increment the counter by one
  value_type inc() {
    val_++;
    return val_;
  }

  /// @brief Decrement the counter by one
  value_type dec() {
    val_--;
    return val_;
  }

private:
  std::atomic_int64_t val_; ///< Atomic counter
};

/// @class HostBuffer Memory.h
///
/// Class holding a pointer to Host allocated memory.
///
class HostBuffer {

public:
  HostBuffer(void *ptr) : ptr_(ptr) {}

  /// @brief return the type of this buffer that this Host
  BufferType type() const { return BufferType::Host; }

  /// @brief return the stored pointer
  void *ptr() const { return ptr_; }

private:
  void *ptr_ = nullptr;
};

/// @brief Callable object that will be used to deallocate the
/// DeviceBuffer from the appropriate memory region
using Deallocator = std::function<etrtError(BufferID)>;

/// @class DeviceMemoryPtr Memory.h
///
/// Class that implements a device buffer smart pointer that can
/// serve as a fat pointer well. This class is going to be the
/// return type of any memory allocations happening on the device.
/// It is going to implement "smart-pointer" semantics to ensure
/// that we do not have any memory leaks on the device. Also it
/// will hold any necessary meta-data for the buffer to enable
/// "fat-pointer" checks: e.g. when doing a memcpy inside this
/// buffer we should be able to retrieve the buffer's size and be
/// able to perform any necessary bounds checks
class DeviceBuffer {
public:
  /// @brief Default constructor, not device buffer assigned
  DeviceBuffer();

  /// @brief Constructor
  ///
  /// @param[in] id: ID of the allocated buffer
  /// @param[in] offset: Offset of the device where the data starts
  /// @param[in] mem_mannager: Pointer to the memory allocator where the buffer
  /// is part of
  DeviceBuffer(BufferID id, BufferOffsetTy offset, Deallocator *deallocator);

  /// @brief Destructor, Release the device memory
  ~DeviceBuffer();

  /// @brief Copy constructor
  ///
  /// Point to the same device buffer as other
  DeviceBuffer(const DeviceBuffer &other);

  /// @brief Move constructor from other to this object
  DeviceBuffer(DeviceBuffer &&other);

  /// @brief Assignment operator
  DeviceBuffer &operator=(DeviceBuffer &other);

  /// @brief Move operator
  DeviceBuffer &operator=(DeviceBuffer &&other);

  /// @brief Return true iff no other object points to the same BufferID
  bool unique() const;

  /// @brief Return the number of objects that hold the same BufferID
  decltype(auto) ref_cntr() {
    RefCounter::value_type val = 0;
    if (ref_cntr_) {
      val = ref_cntr_->val();
    }
    return val;
  }

  /// @brief Return true if the DeviceBuffer holds a BufferID
  explicit operator bool() const;

  /// @brief Return type of this buffer that is Device
  BufferType type() const { return BufferType::Device; }

  /// @brief Return the ID of this DeviceBuffer
  const BufferID id() const { return buffer_id_; }

  /// @brief Return a "pointer" on the device. Currently this returns
  /// an absolute address on the device that is computed based on the base
  /// DRAM address as initialized in the MemoryManager. This is not a public
  /// interface and should be called only by the Device class.
  /// FIXME the following interface should be revisited
  void *ptr() const { return reinterpret_cast<void *>(offset_); }

  /// @brief Compare to DeviceBuffer, used
  friend bool operator<(const DeviceBuffer &a, const DeviceBuffer &b);

  /// @brief Equality comparison
  friend bool operator==(const DeviceBuffer &a, const DeviceBuffer &b);

private:
  friend class ::TestDeviceBuffer;
  friend class ::et_runtime::Module;
  friend class ::et_runtime::Device;

  BufferID buffer_id_;    ///< ID of the buffer
  BufferOffsetTy offset_; ///< Offset of the buffer inside the Device DRAM
  RefCounter *ref_cntr_;  ///< Reference counter
  Deallocator
      *deallocator_; ///< Callable object that will deallocate the buffer

  /// @brief Return the offset of this DeviceBuffer on the device
  const BufferOffsetTy offset() const { return offset_; }

  /// @brief Copy the other DeviceBuffer contents to this one
  void copyBuffer(const DeviceBuffer &other);

  /// @brief Zero out all fields of this class
  void zeroFields();

  /// @brief Release the buffer, if the ref-counter is zero delete the object
  bool ReleaseBuffer();
};

bool operator<(const et_runtime::DeviceBuffer &a,
               const et_runtime::DeviceBuffer &b);

bool operator==(const et_runtime::DeviceBuffer &a,
                const et_runtime::DeviceBuffer &b);

}; // namespace et_runtime

namespace std {
template <> struct hash<et_runtime::DeviceBuffer> {
  std::size_t operator()(et_runtime::DeviceBuffer const &s) const noexcept {
    return s.id();
  }
};
} // namespace std

#endif // ET_RUNTIME_MEMORY_H
