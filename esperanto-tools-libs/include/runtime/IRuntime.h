/*-------------------------------------------------------------------------
 * Copyright (C) 2020, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/

#pragma once
#include <cstddef>
#include <exception>
#include <memory>
#include <stdexcept>
#include <vector>

// forward declarations
namespace dev {
class IDeviceLayer;
}

/// \defgroup runtime_api Runtime API
///
/// The runtime API provides different services to the et-soc devices, allowing
/// the user to manage device memory (see mallocDeviceId and freeDeviceId), load
/// executable elfs into the device (see loadCode and unloadCode) and execute
/// workloads in an asynchronous fashion. These workloads typically consist on
/// copying memory from the host to the device (see memcpyHostToDeviceId),
/// launching a kernel (see kernelLaunch) and getting the results back from the
/// device to the host (see  memcpyDeviceIdToHost) Different workloads can be run
/// asynchronously and potentially in parallel, using streams. Every operation
/// is asynchronous, there are also primitives to allow synchronization between
/// the host and the device (see waitForEventId and streamFlush)
///
///
/// @{
namespace rt {

/// \brief Forward declaration of \ref IProfiler
class IProfiler;

/// \brief Event Handler
enum class EventId : uint16_t {};

/// \brief Stream Handler
enum class StreamId : int {};

/// \brief Device Handler
enum class DeviceId : int {};

/// \brief KernelId Handler
enum class KernelId : int {};

/// \brief RuntimePtr is an alias for a pointer to a Runtime instantation
using RuntimePtr = std::unique_ptr<class IRuntime>;

/// \brief The error handling in the runtime is trough exceptions. This is the
/// only exception kind that the runtime can throw
class Exception : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

/// \brief Constants
namespace {
constexpr int kCacheLineSize = 64; // TODO This should not be here, it should be
                                   // in a header with project-wide constants
}

/// \brief Facade Runtime interface declaration, all runtime interactions should
/// be made using this interface. There is a static method \ref create to make
/// runtime instances (factory method)
///
class IRuntime {
public:
  /// \brief Returns all devices
  ///
  /// @returns a vector containing all device handlers
  ///
  virtual std::vector<DeviceId> getDevices() = 0;

  /// \brief Loads an elf into the device. The caller will provide a byte code
  /// containing the elf representation and its size. Host memory.
  ///
  /// @param[in] device handler indicating which device to upload the elf
  /// @param[in] elf a pointer to host memory containing the elf bytes
  /// @param[in] elf_size the elf size
  ///
  /// @returns a kernel handler which will identify the uploaded code
  ///
  virtual KernelId loadCode(DeviceId device, const void* elf, size_t elf_size) = 0;
  /// \brief Unloads a previously loaded elf code, identified by the kernel
  /// handler
  ///
  /// @param[in] kernel a handler to the code that must be unloaded
  ///
  virtual void unloadCode(KernelId kernel) = 0;

  /// \brief Allocates memory in the device, returns a device memory pointer.
  /// One can't use this pointer directly from the host, this pointer is
  /// intended to be used for memory operations between the host and the device.
  ///
  /// @param[in] device handler indicating in which device to allocate the
  /// memory
  /// @param[in] size indicates the memory allocation size in bytes
  /// @param[in] alignment indicates the required alignment for memory
  /// allocation, defaults to device cache line size
  ///
  /// @returns a device memory pointer
  ///
  virtual void* mallocDevice(DeviceId device, size_t size, int alignment = kCacheLineSize) = 0;

  /// \brief Deallocates previously allocated memory on the given device.
  ///
  /// @param[in] device handler indicating in which device to deallocate the
  /// memory
  /// @param[in] buffer device memory pointer previously allocated with
  /// mallocDevice to be deallocated
  ///
  virtual void freeDevice(DeviceId device, void* buffer) = 0;

  /// \brief Creates a new stream and associates it to the given device.
  /// A stream is an abstraction of a "pipeline" where you can push operations
  /// (mem copies or kernel launches) and enforce the dependencies between these
  /// operations
  ///
  /// @param[in] device handler indicating in which device to associate the
  /// stream
  ///
  /// @returns a stream handler
  ///
  virtual StreamId createStream(DeviceId device) = 0;

  /// \brief Destroys a previously created stream
  ///
  /// @param[in] stream handler to the stream to be destroyed
  ///
  ///
  virtual void destroyStream(StreamId stream) = 0;

  /// \brief Queues a execution work into a stream. The work is identified by
  /// the kernel handler, which has been previously loaded into the device which
  /// is associated to the stream. The parameters of the kernel are given by
  /// kernel_args and kernel_args_size; these parameters will be copied from the
  /// host memory to the device memory by the runtime. The firmware will load
  /// these parameters into ??? and ??? registers, the kernel code should cast
  /// these registers to the expected types.
  /// The kernel execution is always asynchronous.
  ///
  /// @param[in] stream handler indicating in which stream the kernel will be
  /// executed. The kernel code have to be registered into the device associated
  /// to the stream previously.
  /// @param[in] kernel handler which indicate what code to execute in the
  /// device.
  /// @param[in] kernel_args buffer containing all the parameters to be copied
  /// to the device memory prior to the code execution
  /// @param[in] kernel_args_size size of the kernel_args buffer
  /// @param[in] shire_mask indicates in what shires the kernel will be executed
  /// @param[in] barrier this parameter indicates if the kernel execution should
  /// be postponed till all previous works issued into this stream finish (a
  /// barrier). Usually the kernel launch must be postponed till some previous
  /// memory operations end, hence the default value is true
  ///
  /// @returns EventId is a handler of an event which can be waited for
  /// (waitForEventId) to syncrhonize when the kernel ends the execution
  ///
  virtual EventId kernelLaunch(StreamId stream, KernelId kernel, const void* kernel_args, size_t kernel_args_size,
                               uint64_t shire_mask, bool barrier = true) = 0;

  /// \brief Queues a memcpy operation from host memory to device memory. The
  /// device memory must be previously allocated by a mallocDevice.
  ///
  /// @param[in] stream handler indicating in which stream to queue the memcpy
  /// operation
  /// @param[in] h_src host memory buffer to copy from
  /// @param[in] d_dst device memory buffer to copy to
  /// @param[in] size indicates the size of the memcpy
  /// @param[in] barrier this parameter indicates if the memcpy operation should
  /// be postponed till all previous works issued into this stream finish (a
  /// barrier). Usually the memcpies from host to device can run in parallel,
  /// hence the default value is false All memcpy operations are always
  /// asynchronous.
  ///
  /// @returns EventId is a handler of an event which can be waited for
  /// (waitForEventId) to synchronize when the memcpy ends
  ///
  virtual EventId memcpyHostToDevice(StreamId stream, const void* h_src, void* d_dst, size_t size,
                                     bool barrier = false) = 0;
  /// \brief Queues a memcpy operation from device memory to host memory. The
  /// device memory must be a valid region previously allocated by a
  /// mallocDevice; the host memory must be a previously allocated memory in the
  /// host by the conventional means (for example the heap)
  ///
  /// @param[in] stream handler indicating in which stream to queue the memcpy
  /// operation
  /// @param[in] d_src device memory buffer to copy from
  /// @param[in] h_dst host memory buffer to copy to
  /// @param[in] size indicates the size of the memcpy
  /// @param[in] barrier this parameter indicates if the memcpy operation should
  /// be postponed till all previous works issued into this stream finish (a
  /// barrier). Usually the memcpies from device to host must wait till a
  /// previous kernel execution finishes, hence the default value is true All
  /// memcpy operations are always asynchronous.
  ///
  /// @returns EventId is a handler of an event which can be waited for
  /// (waitForEventId) to synchronize when the memcpy ends
  ///
  virtual EventId memcpyDeviceToHost(StreamId stream, const void* d_src, void* h_dst, size_t size,
                                     bool barrier = true) = 0;

  /// \brief This will block the caller thread until the given event is
  /// dispatched. This primitive allows to synchronize with the device
  /// execution.
  ///
  /// @param[in] event is the event to wait for, result of a memcpy operation or a
  /// kernel launch.
  ///
  virtual void waitForEvent(EventId event) = 0;

  /// \brief This will block the caller thread until all commands issued to the
  /// given stream finish. This primitive allows to synchronize with the device
  /// execution.
  ///
  /// @param[in] stream this is the stream to synchronize with.
  ///
  virtual void waitForStream(StreamId stream) = 0;

  /// \brief Virtual Destructor to enable polymorphic release of the runtime
  /// instances
  virtual ~IRuntime() = default;

  /// \brief Returns a pointer to the profiler interface; don't delete/free this pointer since this is owned by the
  /// runtime itself.
  ///
  /// @returns IProfiler an interface to the profiler. See \ref IProfiler
  ///
  virtual IProfiler* getProfiler() = 0;

  ///
  /// \brief Factory method to instantiate a IRuntime implementation
  ///
  ///
  /// @returns RuntimePtr an IRuntime instance. See \ref dev::IDeviceLayer
  ///
  static RuntimePtr create(dev::IDeviceLayer* deviceLayer);
};

} // namespace rt
  /// @}
  // End of runtime_api
