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

#include "Types.h"
#include <chrono>
#include <cstddef>
#include <ostream>
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
  virtual KernelId loadCode(DeviceId device, const std::byte* elf, size_t elf_size) = 0;
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
  virtual std::byte* mallocDevice(DeviceId device, size_t size, uint32_t alignment = kCacheLineSize) = 0;

  /// \brief Deallocates previously allocated memory on the given device.
  ///
  /// @param[in] device handler indicating in which device to deallocate the
  /// memory
  /// @param[in] buffer device memory pointer previously allocated with
  /// mallocDevice to be deallocated
  ///
  virtual void freeDevice(DeviceId device, std::byte* buffer) = 0;

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
  /// these parameters into RA register, the kernel code should cast this register to the expected types.
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
  /// @param[in] barrier this parameter indicates if the kernel execution should be postponed till all previous works
  /// issued into this stream finish (a barrier). Usually the kernel launch must be postponed till some previous
  /// memory operations end, hence the default value is true.
  /// @param[in] flushL3 this parameter indicates if the L3 should be flushed before the kernel execution starts.
  ///
  /// @returns EventId is a handler of an event which can be waited for
  /// (waitForEventId) to syncrhonize when the kernel ends the execution
  ///
  virtual EventId kernelLaunch(StreamId stream, KernelId kernel, const std::byte* kernel_args, size_t kernel_args_size,
                               uint64_t shire_mask, bool barrier = true, bool flushL3 = false) = 0;

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
  virtual EventId memcpyHostToDevice(StreamId stream, const std::byte* h_src, std::byte* d_dst, size_t size,
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
  virtual EventId memcpyDeviceToHost(StreamId stream, const std::byte* d_src, std::byte* h_dst, size_t size,
                                     bool barrier = true) = 0;

  /// \brief This will block the caller thread until the given event is
  /// dispatched or the timeout is reached. This primitive allows to synchronize with the device
  /// execution.
  ///
  /// @param[in] event is the event to wait for, result of a memcpy operation or a
  /// kernel launch.
  /// @param[in] timeout is the number of seconds to wait till aborting the wait.
  ///
  /// @returns false if the timeout is reached, true otherwise.
  ///
  virtual bool waitForEvent(EventId event, std::chrono::seconds timeout = std::chrono::hours(24)) = 0;

  /// \brief This will block the caller thread until all commands issued to the
  /// given stream finish or if the timeout is reached. This primitive allows to synchronize with the device
  /// execution.
  ///
  /// @param[in] stream this is the stream to synchronize with.
  /// @param[in] timeout is the number of seconds to wait till aborting the wait.
  ///
  /// @returns false if the timeout is reached, true otherwise.
  ///
  virtual bool waitForStream(StreamId stream, std::chrono::seconds timeout = std::chrono::hours(24)) = 0;

  /// \brief This will return a list of errors and their execution context (if any)
  ///
  /// @param[in] stream this is the stream to synchronize with.
  /// @param[in] timeout is the number of seconds to wait till aborting the wait.
  ///
  /// @returns StreamStatus contains the state of the associated stream.
  ///
  virtual std::vector<StreamError> retrieveStreamErrors(StreamId stream) = 0;

  /// \brief This callback (when set) will be automatically called when a new StreamError occurs, making the polling
  /// through retrieveStreamErrors unnecessary.
  ///
  /// @param[in] callback see \ref StreamErrorCallbac. This is the callback which will be called when a StreamError
  /// occurs.
  ///
  virtual void setOnStreamErrorsCallback(StreamErrorCallback callback) = 0;

  /// \brief Virtual Destructor to enable polymorphic release of the runtime
  /// instances
  virtual ~IRuntime() = default;

  /// \brief Returns a pointer to the profiler interface; don't delete/free this pointer since this is owned by the
  /// runtime itself.
  ///
  /// @returns IProfiler an interface to the profiler. See \ref IProfiler
  ///
  virtual IProfiler* getProfiler() = 0;

  /// \brief Allocates a DmaBuffer which can be used to perform "zero-copy" DMA transfers. Ideally all memcpy operations
  /// should be used using DmaBuffers; if not, runtime will do it under the hood.
  ///
  /// @param[in] device indicates the device where the DmaBuffer will be assoaciated to.
  /// @param[in] size is the desired size in bytes of the DmaBuffer allocation.
  /// @param[in] writeable indicates if the DmaBuffer should be writeable by the host or not (for read-only buffers pass
  /// "false").
  ///
  /// @returns DmaBuffer which can be used to avoid unnecessary staging memory copies; enabling "zero-copy".
  ///
  virtual DmaBuffer allocateDmaBuffer(DeviceId device, size_t size, bool writeable) = 0;

  /// \brief Setup the device for getting master minion and compute minion traces. Tracing is done using internal
  /// buffers which, if overflow, they will overwrite the start of the buffer. In the future there will be a mechanism
  /// to achieve larger traces. There are some parameters to be discussed yet.
  ///
  /// @param[in] stream handler indicating in which stream to queue the setup operation
  /// @param[in] shireMask indicates which shires will be traced
  /// @param[in] threadMask indicates which threads, for each shire, will be traced
  /// @param[in] eventMask indicates which events to be traced. NOTE: there are some lacking information on what are the
  /// events we can trace
  /// @param[in] filterMask bit mask representing a list of filters for a given event to trace. NOTE: we don't know
  /// which are these filters yet
  /// @param[in] barrier this parameter indicates if the setup operation should be postponed till all previous works
  /// issued into this stream finish (a barrier).
  ///
  /// @returns EventId is a handler of an event which can be waited for (waitForEventId) to synchronize when the setup
  /// ends
  ///
  virtual EventId setupDeviceTracing(StreamId stream, uint32_t shireMask, uint32_t threadMask, uint32_t eventMask,
                                     uint32_t filterMask, bool barrier = true) = 0;

  /// \brief Instructs the device to start tracing, and let the trace results on the output binary streams provided, one
  /// or two binary output streams can be provided.
  ///
  /// @param[in] stream handler indicating in which stream to queue the start tracing operation.
  /// @param[out] mmOutput binary stream where master minion device traces will be recorded. Can be null if don't want
  /// master minion traces.
  /// @param[out] cmOutput binary stream where computer minion device traces will be recorded. Can be null if don't want
  /// computer minion traces.
  /// @param[in] barrier this parameter indicates if the start tracing operation should be postponed till all previous
  /// works issued into this stream finish (a barrier).
  ///
  /// @returns EventId is a handler of an event which can be waited for (waitForEventId) to synchronize.
  ///
  virtual EventId startDeviceTracing(StreamId stream, std::ostream* mmOutput, std::ostream* cmOutput,
                                     bool barrier = true) = 0;

  /// \brief Instructs the device to stop tracing.
  ///
  /// @param[in] stream handler indicating in which stream to queue the start tracing operation.
  /// @param[in] barrier this parameter indicates if the stop tracing operation should be postponed till all previous
  /// works issued into this stream finish (a barrier).
  ///
  /// @returns EventId is a handler of an event which can be waited for (waitForEventId) to synchronize.
  ///
  virtual EventId stopDeviceTracing(StreamId stream, bool barrier = true) = 0;

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
