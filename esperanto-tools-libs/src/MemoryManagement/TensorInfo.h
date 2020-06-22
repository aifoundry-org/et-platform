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

/// Type that holds the tensor offset, inside the DRAM region we control
using TensorOffsetTy = uint64_t;

/// Size of a tensor in bytes
using TensorSizeTy = uint64_t;

/// Size of the padding of a tensor in bytes
using PaddingSizeTy = uint32_t;

/// @enum TensorType TensorInfo.h
///
/// @brief Type of the tensor/buffer/memory-region
enum class TensorType : uint8_t {
  None = 0,    ///< Empty type
  Free,        ///< Unallocate memory region
  Code,        ///< Code holding buffer
  Constant,    ///< Constant tensors
  Placeholder, ///< Placeholder tensors
  Logging,     ///< Logging buffer
  MAX,         ///< Max type
};

///
using TensorPermissionsTy = uint8_t;

/// @enum TensorPermissions TensorInfo.h
///
/// @brief Permissions of the tensor memory region
enum class TensorPermissions : TensorPermissionsTy {
  None = 0,         ///< Empty permissions
  Read = 1 << 1,    ///< Read permissions
  Write = 1 << 2,   ///< Write permissions
  Execute = 1 << 3, ///< Execute permissions
};

constexpr TensorPermissionsTy operator|(const TensorPermissions a,
                                        const TensorPermissions b) {
  return static_cast<TensorPermissionsTy>(a) |
         static_cast<TensorPermissionsTy>(b);
}

/// @struct TensorCommonMD TensorInfo.h
///
/// Struct holding the common meta-data information for a specific tensor
/// region
struct TensorCommonMD {
  bool in_use;         ///< True iff the memory region is being used
  TensorType type;     ///< Type of the tensor
  TensorID id;         ///< Global Id of this constant tensor
  TensorOffsetTy base; ///< Base offset of the tensor
  TensorSizeTy size;   ///< Size of the tesnor
  TensorOffsetTy
      next; ///< Pointer to the Next tensor allocated in the memory region
  TensorOffsetTy
      prev; ///< Pointer to the Previous tensor allocate in the memory region
  PaddingSizeTy padding; ///< Bytes of padding of the tensor
  TensorPermissionsTy
      permissions; ///< Permissions (read, write, execute) of this memory region
};

/// @struct FreeRegion TensorInfo.h
///
/// @brief Struct representing a free region of memory
struct FreeRegion {
  TensorCommonMD hdr; ///< Common tensor metadata
};

/// @struct CodeTensor TensorInfo.h
///
/// @brief Struct holding the meta-data of a memory region where we store code
///
struct CodeBuffer {
  TensorCommonMD hdr;     ///< Common tensor metadata
  NetworkID network_id;   ///< Network this code region belongs to
  KernelCodeID kernel_id; ///< KernelId that this code belongs to
};

/// @struct ConstantTensor TensorInfo.h
///
/// Struct holding the meta-data of a constant tensor
///
struct ConstantTensor {
  TensorCommonMD hdr;   ///< Common tensor metadata
  NetworkID network_id; ///< Network this constant tensor belongs to
};

/// @struct PlaceholderTensor TensorInfo.h
///
/// Struct holding the meta-data of placeholder tensor
///
struct PlaceholderTensor {
  TensorCommonMD hdr;   ///< Common tensor metadata
  NetworkID network_id; ///< Network this placeholder tensor belongs to
  StreamID stream_id;   ///< Stream / Inference this placeholder will be updated
                        ///< as part
};

/// @struct LoggingBuffer TensorInfo.h
///
/// Struct holding the meta-data of a placeholder tensor
///
struct LoggingBuffer {
  TensorCommonMD hdr; ///< Common tensor metadata
  uint32_t minion_id; ///< Minion this buffer belongs to
};

/// @class AstractTensorInfo TensorInfo.h
///
/// @brief Helper abstract base class for the different types of tensors
/// that are instances of the following templated. Use to enable storing the
/// following template in std containers
class AbstractTensorInfo {

public:
  AbstractTensorInfo() = default;
  virtual ~AbstractTensorInfo() = default;

  /// @brief Return the md_base, this includes the starting point of the
  /// meta-data that are prefixed befored the tensor data
  virtual const TensorOffsetTy mdBase() const = 0;

  /// @brief Return the offset of this tensor
  virtual const TensorOffsetTy base() const = 0;

  /// @brief Return the size of this tensor
  virtual const TensorSizeTy size() const = 0;

  /// @brief Return the total size of this tensor, including padding and
  /// meta-data
  virtual const TensorSizeTy totalSize() const = 0;

  /// @brief Return the region end offset
  virtual const TensorOffsetTy endOffset() const = 0;

  /// @brief Return the tensor id
  virtual const TensorID id() const = 0;

  /// @brief get next allocated buffer
  virtual TensorOffsetTy next() const = 0;

  /// @brief Set next allocated buffer
  virtual void next(TensorOffsetTy) = 0;

  /// @brief Get previous allocated buffer
  virtual TensorOffsetTy prev() const = 0;

  /// @brief Set previous allocated buffer
  virtual void prev(TensorOffsetTy) = 0;

  /// @brief Compare 2 tensors, return true if this tensor is "left" of the
  /// other tesnor
  ///
  /// @param[in] other Other tensor to compare with
  bool operator<(const AbstractTensorInfo &other) const {
    // The end element location
    auto this_end = base() + size() - 1;
    // check for an integer overflow
    assert(this_end >= base());
    return this_end < other.base();
  }
};

/// @brief Return the next available TensorID in the system
///
/// Note that this is a static function and not a member of
/// the TensorInfo template because the TensorID needs to be
/// unique across all types of tensors
TensorID nextTensorID();

/// @class TensorInfo TensorInfo.h
///
/// @brief Template that holds different types of tensors
template <class TENSOR_TYPE>
class TensorInfo final : public AbstractTensorInfo {
public:
  using tensor_type_t = TENSOR_TYPE;

  /// Constructor
  ///
  /// @param[in] base   Offset in the region where the tensor is stored
  /// @param[in] size   Size in bytes of the tensor.
  TensorInfo(TensorOffsetTy base, TensorSizeTy size) : tensor_info_() {
    assert(size > 0);
    tensor_info_.hdr.in_use = true;
    tensor_info_.hdr.type = type();
    tensor_info_.hdr.id = nextTensorID();
    tensor_info_.hdr.base = base;
    tensor_info_.hdr.size = size;
    tensor_info_.hdr.permissions = permissions();
  };

  /// @brief Return the size in bytes of the tensor metadata
  static constexpr TensorSizeTy mdSize() {
    // The free list has no meta-data on the device
    if (type() == TensorType::Free) {
      return 0;
    }
    return sizeof(tensor_type_t);
  }

  /// @brief Return the md_base, this includes the starting point of the
  /// meta-data that are prefixed befored the tensor data
  const TensorOffsetTy mdBase() const override {
    auto md_base = base() - mdSize();
    // check for underflow
    assert(md_base <= base());
    return md_base;
  }

  /// @brief Return the offset of this tensor
  const TensorOffsetTy base() const override { return tensor_info_.hdr.base; }

  /// @brief Return the size of this tensor
  const TensorSizeTy size() const override { return tensor_info_.hdr.size; }

  /// @brief Return the region end offset
  const TensorOffsetTy endOffset() const { return mdBase() + totalSize(); }

  /// @brief Return the total size of this tensor, including padding and
  /// meta-data
  const TensorSizeTy totalSize() const override { return size() + mdSize(); }

  /// @brief Return the type of the tensor
  static const TensorType type();

  /// @brief ID of this buffer/tensor
  const TensorID id() const override { return tensor_info_.hdr.id; }

  /// @brief Set next allocated buffer offset
  TensorOffsetTy next() const override { return tensor_info_.hdr.next; }

  /// @brief Set next allocated buffer offset
  void next(TensorOffsetTy v) override { tensor_info_.hdr.next = v; }

  /// @brief Get prev allocated buffer offset
  TensorOffsetTy prev() const override { return tensor_info_.hdr.prev; }

  /// @brief Set previous allocated buffer offset
  void prev(TensorOffsetTy v) override { tensor_info_.hdr.prev = v; }

  /// @brief Return the permission bit-mask of this tensor
  const TensorPermissionsTy permissions() const;

  /// @brief Return reference to the tensor info. Used to udpate/set the
  /// tensor-type specific information of the tensor.
  tensor_type_t &tensor_info() { return tensor_info_; }

private:
  TENSOR_TYPE tensor_info_;
};

std::ostream &operator<<(std::ostream &os, const AbstractTensorInfo &t);

}; // namespace memory_management
}; // namespace device

}; // namespace et_runtime

#endif //
