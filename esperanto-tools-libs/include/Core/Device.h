//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#ifndef ET_RUNTIME_DEVICE_H
#define ET_RUNTIME_DEVICE_H

#include "Support/ErrorOr.h"

#include <memory>

namespace et_runtime {

class Stream;
class AbstractMemoryPtr;
class HostMemoryPtr;
class DeviceMemoryPtr;

class Device {
public:
  Device() = default;

  ///
  /// @brief  Detach the currently connected Device from the caller's process.
  ///
  /// Release the calling process's attached Device, or do nothing if a Device
  /// is currently not attached. This results in the calling process's context
  /// being freed, all of the connected Device's resources associated with this
  /// process being released, and the given Device being made able to be
  /// connected to once again.
  ///
  /// @return  etrtSuccess
  ////
  etrtError resetDevice(int deviceID);

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

  ///
  /// @brief  Copy the contents of one region of allocated memory to another.
  ///
  /// Initiate the copying of the given number of bytes from the given source
  /// location to the given destination location.  The source and destination
  /// regions can be other either the Host or an attached Device, but both
  /// regions must be currently allocated by the calling process using this API.
  /// This call returns when the transfer has completed (or a failure occurs).
  ///
  /// @param[in] dst  A pointer to the location where the memory is to be
  /// copied.
  /// @param[in] src  A pointer to the location from which the memory is to be
  /// copied.
  /// @param[in] count  The number of bytes that should be copied.
  /// @param[in] kind  Enum indicating the direction of the transfer (i.e.,
  /// H->H, H->D, D->H, or D->D).
  /// @return  etrtSuccess, etrtErrorInvalidValue,
  /// etrtErrorInvalidMemoryDirection
  etrtError memcpy(AbstractMemoryPtr *dst, const AbstractMemoryPtr *src,
                   size_t count);

  ///
  /// @brief  Sets the bytes in allocated memory region to a given value.
  ///
  /// Writes a given number of bytes in an allocated memory region to a given
  /// byte value. This function executes asynchronously, unless the target
  /// memory address refers to a pinned Host memory region.
  ///
  /// @param[in] ptr  Pointer to location of currently allocated memory region
  /// that is to be written.
  /// @param[in] value  Constant byte value to write into the given memory
  /// region.
  /// @param[in] count  Number of bytes to be written with the given value.
  /// @return  etrtSuccess, etrtErrorInvalidValue
  ////
  etrtError memset(AbstractMemoryPtr &ptr, uint8_t value, size_t count);

  ///
  /// @brief  Create a new Stream.
  ///
  /// Return the handle for a newly created Stream for the caller's process.
  ///
  /// @return  ErrorOr (etrtError: etrtErrorInvalidValue) or pointer to @ref
  /// Stream object
  ////
  ErrorOr<std::unique_ptr<Stream>> streamCreate();

  ///
  /// @brief  Create a new Stream.
  ///
  /// Return the handle for a newly created Stream for the caller's process.
  ///
  /// The `flags` argument allows the caller to have control over the behavior
  /// of the newly created Stream. If a 0 (or `cudaStreamDefault`) value is
  /// given, then the Stream is created using the default options. If the
  /// `cudaStreamNonBlocking` value is given, then the newly created Stream may
  /// run concurrently with the Default Stream (i.e., Stream 0), and it performs
  /// no implicit synchronization with the Default Stream.
  ///
  /// @param[in] flags  A bitmap composed of flags that select options for the
  /// new Stream.
  /// @return  ErrorOr (etrtError: etrtErrorInvalidValue) or pointer to @ref
  /// Stream
  ////
  ErrorOr<std::unique_ptr<Stream>> streamCreateWithFlags(unsigned int flags);
};
} // namespace et_runtime

#endif // ET_RUNTIME_DEVICE_H
