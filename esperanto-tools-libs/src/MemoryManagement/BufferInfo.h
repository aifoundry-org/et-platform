//******************************************************************************
// Copyright (C) 2020, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

/// @file

#ifndef ET_RUNTIME_MEMORY_MANAGEMENT_TENSOR_INFO_H
#define ET_RUNTIME_MEMORY_MANAGEMENT_TENSOR_INFO_H

#include "esperanto/runtime/Common/CommonTypes.h"

#include <cassert>
#include <cstdint>
#include <iostream>

namespace et_runtime {
namespace device {
namespace memory_management {

/// Type that holds the buffer offset, inside the DRAM region we control
using BufferOffsetTy = int64_t;

/// Size of a buffer in bytes
using BufferSizeTy = int64_t;

/// Size of the padding of a buffer in bytes
using PaddingSizeTy = uint32_t;

/// @enum BufferType BufferInfo.h
///
/// @brief Type of the buffer/buffer/memory-region
enum class BufferType : uint8_t {
  None = 0,    ///< Empty type
  Free,        ///< Unallocate memory region
  Code,        ///< Code holding buffer
  Constant,    ///< Constant buffers
  Placeholder, ///< Placeholder buffers
  Logging,     ///< Logging buffer
  MAX,         ///< Max type
};

///
using BufferPermissionsTy = uint8_t;

/// @enum BufferPermissions BufferInfo.h
///
/// @brief Permissions of the buffer memory region
enum class BufferPermissions : BufferPermissionsTy {
  None = 0,         ///< Empty permissions
  Read = 1 << 1,    ///< Read permissions
  Write = 1 << 2,   ///< Write permissions
  Execute = 1 << 3, ///< Execute permissions
};

constexpr BufferPermissionsTy operator|(const BufferPermissions a,
                                        const BufferPermissions b) {
  return static_cast<BufferPermissionsTy>(a) |
         static_cast<BufferPermissionsTy>(b);
}

/// @struct BufferCommonMD BufferInfo.h
///
/// Struct holding the common meta-data information for a specific buffer
/// region
struct BufferCommonMD {
  bool in_use;         ///< True iff the memory region is being used
  BufferType type;     ///< Type of the buffer
  BufferID id;         ///< Global Id of this constant buffer
  BufferOffsetTy base; ///< Base offset of the buffer region, this includes both
                       ///< any alignment padding and the meta-data prefix.
  BufferOffsetTy
      aligned_start; ///< The aligned offset that we return as the "pointer"
                     /// that has been allocated inside this region. Before the
                     /// offset of the pointer we place the tensor meta-data on
                     /// the device.
  BufferSizeTy req_size; ///< Requested allocation size by malloc
  BufferSizeTy size;     ///< Total size of the buffer region.
  BufferOffsetTy
      next; ///< Pointer to the Next buffer allocated in the memory region
  BufferOffsetTy
      prev; ///< Pointer to the Previous buffer allocate in the memory region
  PaddingSizeTy padding; ///< Bytes of padding of the buffer
  BufferPermissionsTy
      permissions; ///< Permissions (read, write, execute) of this memory region
};

/// @struct FreeRegion BufferInfo.h
///
/// @brief Struct representing a free region of memory
struct FreeRegion {
  BufferCommonMD hdr; ///< Common buffer metadata
};

/// @struct CodeBuffer BufferInfo.h
///
/// @brief Struct holding the meta-data of a memory region where we store code
///
struct CodeBuffer {
  BufferCommonMD hdr;     ///< Common buffer metadata
  NetworkID network_id;   ///< Network this code region belongs to
  KernelCodeID kernel_id; ///< KernelId that this code belongs to
};

/// @struct ConstantBuffer BufferInfo.h
///
/// Struct holding the meta-data of a constant buffer
///
struct ConstantBuffer {
  BufferCommonMD hdr;   ///< Common buffer metadata
  NetworkID network_id; ///< Network this constant buffer belongs to
};

/// @struct PlaceholderBuffer BufferInfo.h
///
/// Struct holding the meta-data of placeholder buffer
///
struct PlaceholderBuffer {
  BufferCommonMD hdr;   ///< Common buffer metadata
  NetworkID network_id; ///< Network this placeholder buffer belongs to
  StreamID stream_id;   ///< Stream / Inference this placeholder will be updated
                        ///< as part
};

/// @struct LoggingBuffer BufferInfo.h
///
/// Struct holding the meta-data of a placeholder buffer
///
struct LoggingBuffer {
  BufferCommonMD hdr; ///< Common buffer metadata
  uint32_t minion_id; ///< Minion this buffer belongs to
};

/// @class AstractBufferInfo BufferInfo.h
///
/// @brief Helper abstract base class for the different types of buffers
/// that are instances of the following templated. Use to enable storing the
/// following template in std containers
class AbstractBufferInfo {

public:
  AbstractBufferInfo() = default;
  virtual ~AbstractBufferInfo() = default;

  /// @brief Return the offset of this buffer
  virtual const BufferOffsetTy base() const = 0;

  /// @brief Set the base the offset of this buffer
  virtual void base(BufferOffsetTy) = 0;

  /// @brief Return the md_base, this includes the starting point of the
  /// meta-data that are prefixed befored the buffer data
  virtual const BufferOffsetTy mdBase() const = 0;

  /// @brief Return the aligned start of this buffer region
  virtual const BufferOffsetTy alignedStart() const = 0;

  /// @brief Return  the requested allocation size of this buffer
  virtual const BufferSizeTy reqSize() const = 0;

  /// @brief Return the size of this buffer
  virtual const BufferSizeTy size() const = 0;

  /// @brief Set the size of this buffer
  virtual void size(BufferSizeTy size) = 0;

  /// @brief Return the region end offset
  virtual const BufferOffsetTy endOffset() const = 0;

  /// @brief Return the buffer id
  virtual const BufferID id() const = 0;

  /// @brief get next allocated buffer
  virtual BufferOffsetTy next() const = 0;

  /// @brief Set next allocated buffer
  virtual void next(BufferOffsetTy) = 0;

  /// @brief Get previous allocated buffer
  virtual BufferOffsetTy prev() const = 0;

  /// @brief Set previous allocated buffer
  virtual void prev(BufferOffsetTy) = 0;

  /// @brief Compare 2 buffers, return true if this buffer is "left" of the
  /// other tesnor
  ///
  /// @param[in] other Other buffer to compare with
  bool operator<(const AbstractBufferInfo &other) const {
    // The end element location
    auto this_end = base() + size() - 1;
    // check for an integer overflow
    assert(this_end >= base());
    return this_end < other.base();
  }
};

/// @brief Return the next available BufferID in the system
///
/// Note that this is a static function and not a member of
/// the BufferInfo template because the BufferID needs to be
/// unique across all types of buffers
BufferID nextBufferID();

/// @class BufferInfo BufferInfo.h
///
/// @brief Template that holds different types of buffers
template <class TENSOR_TYPE>
class BufferInfo final : public AbstractBufferInfo {
public:
  using buffer_type_t = TENSOR_TYPE;

  /// Constructor
  ///
  /// @param[in] base   Base offset in the region where the buffer is stored
  /// @param[in] aligned_ptr Aligned offset in the region that is the actual
  /// start of the data
  /// @param[in] req_size Requested allocation size by malloc
  /// @param[in] size   Size in bytes of the buffer.
  BufferInfo(BufferOffsetTy base, BufferOffsetTy aligned_start,
             BufferSizeTy req_size, BufferSizeTy size)
      : buffer_info_() {
    assert(size >= 0);
    buffer_info_.hdr.in_use = true;
    buffer_info_.hdr.type = type();
    buffer_info_.hdr.id = nextBufferID();
    buffer_info_.hdr.base = base;
    buffer_info_.hdr.aligned_start = aligned_start;
    buffer_info_.hdr.req_size = req_size;
    buffer_info_.hdr.size = size;
    buffer_info_.hdr.permissions = permissions();
  };

  ///
  /// @param[in] base   Base offset in the region where the buffer is stored
  /// @param[in] size   Size in bytes of the buffer.
  BufferInfo(BufferOffsetTy base, BufferSizeTy size)
      : BufferInfo(base, base, size, size){};

  /// @brief Return the size in bytes of the buffer metadata
  /// The size is increased to be cache-line mulitple to avoid miss-aligning
  /// the buffer being allocated.
  static constexpr BufferSizeTy mdSize() {
    // The free list has no meta-data on the device
    if (type() == BufferType::Free) {
      return 0;
    }
    auto type_size = sizeof(buffer_type_t);
    auto aligned_md_size = (((type_size + 63U) / 64U) * 64U);
    assert(aligned_md_size >= type_size);
    return aligned_md_size;
  }

  /// @brief Return the md_base, this includes the starting point of the
  /// meta-data that are prefixed befored the buffer data
  const BufferOffsetTy mdBase() const override {
    auto md_base = alignedStart() - mdSize();
    // check for underflow
    assert(md_base <= alignedStart());
    return md_base;
  }

  /// @brief Return the offset of this buffer
  const BufferOffsetTy base() const override { return buffer_info_.hdr.base; }

  /// @brief Set the base the offset of this buffer
  void base(BufferOffsetTy base) override { buffer_info_.hdr.base = base; }

  const BufferOffsetTy alignedStart() const override {
    return buffer_info_.hdr.aligned_start;
  }
  /// @brief Return  the requested allocation size of this buffer
  const BufferSizeTy reqSize() const override {
    return buffer_info_.hdr.req_size;
  }

  /// @brief Return the size of this buffer
  const BufferSizeTy size() const override { return buffer_info_.hdr.size; }

  /// @brief Set the size of this buffer
  void size(BufferSizeTy size) override { buffer_info_.hdr.size = size; }

  /// @brief Return the region end offset
  const BufferOffsetTy endOffset() const { return base() + size(); }

  /// @brief Return the type of the buffer
  static const BufferType type();

  /// @brief ID of this buffer/buffer
  const BufferID id() const override { return buffer_info_.hdr.id; }

  /// @brief Set next allocated buffer offset
  BufferOffsetTy next() const override { return buffer_info_.hdr.next; }

  /// @brief Set next allocated buffer offset
  void next(BufferOffsetTy v) override { buffer_info_.hdr.next = v; }

  /// @brief Get prev allocated buffer offset
  BufferOffsetTy prev() const override { return buffer_info_.hdr.prev; }

  /// @brief Set previous allocated buffer offset
  void prev(BufferOffsetTy v) override { buffer_info_.hdr.prev = v; }

  /// @brief Return the permission bit-mask of this buffer
  const BufferPermissionsTy permissions() const;

private:
  TENSOR_TYPE buffer_info_;
};

std::ostream &operator<<(std::ostream &os, const AbstractBufferInfo &t);

}; // namespace memory_management
}; // namespace device

}; // namespace et_runtime

#endif //
