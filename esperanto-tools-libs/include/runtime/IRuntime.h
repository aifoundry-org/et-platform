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

#include "IProfiler.h"
#include "Types.h"
#include <chrono>
#include <cstddef>
#include <memory>
#include <ostream>
#include <vector>

// forward declarations
namespace dev {
class IDeviceLayer;
}

/// \defgroup runtime_api Runtime API
///
/// The runtime API provides different services to the et-soc devices, allowing the user to manage device memory (see
/// mallocDeviceId and freeDeviceId), load executable elfs into the device (see loadCode and unloadCode) and execute
/// workloads in an asynchronous fashion. These workloads typically consist on copying memory from the host to the
/// device (see memcpyHostToDeviceId), launching a kernel (see kernelLaunch) and getting the results back from the
/// device to the host (see  memcpyDeviceIdToHost) Different workloads can be run asynchronously and potentially in
/// parallel, using streams. Every operation is asynchronous, there are also primitives to allow synchronization between
/// the host and the device (see waitForEventId and streamFlush)
///
/// @{
namespace rt {
/// \brief Facade Runtime interface declaration, all runtime interactions should be made using this interface. There is
/// a static method \ref create to make runtime instances (factory method)
///
class IRuntime {
public:
  /// \brief Returns all devices
  ///
  /// @returns a vector containing all device handlers
  ///
  std::vector<DeviceId> getDevices();

  /// \brief Returns the properties of a given device
  ///
  /// @param[in] device handler indicating the device
  ///
  /// @returns the properties for the requested device
  DeviceProperties getDeviceProperties(DeviceId device) const;

  /// \brief Allocates memory in the device, returns a device memory pointer. One can't use this pointer directly from
  /// the host, this pointer is intended to be used for memory operations between the host and the device.
  ///
  /// @param[in] device handler indicating in which device to allocate the memory
  /// @param[in] size indicates the memory allocation size in bytes
  /// @param[in] alignment indicates the required alignment for memory allocation, defaults to device cache line size
  ///
  /// @returns a device memory pointer
  ///
  std::byte* mallocDevice(DeviceId device, size_t size, uint32_t alignment = kCacheLineSize);

  /// \brief Deallocates previously allocated memory on the given device.
  ///
  /// @param[in] device handler indicating in which device to deallocate the
  /// memory
  /// @param[in] buffer device memory pointer previously allocated with
  /// mallocDevice to be deallocated
  ///
  void freeDevice(DeviceId device, std::byte* buffer);

  /// \brief Creates a new stream and associates it to the given device. A stream is an abstraction of a "pipeline"
  /// where you can push operations (mem copies or kernel launches) and enforce the dependencies between these
  /// operations
  ///
  /// @param[in] device handler indicating in which device to associate the stream
  ///
  /// @returns a stream handler
  ///
  StreamId createStream(DeviceId device);

  /// \brief Destroys a previously created stream
  ///
  /// @param[in] stream handler to the stream to be destroyed
  ///
  ///
  void destroyStream(StreamId stream);

  /// \brief Loads an elf into the device. The caller will provide a byte code containing the elf representation and its
  /// size. Host memory.
  /// @param[in] stream handler indicating the stream used for the kernel loading.
  /// @param[in] elf a pointer to host memory containing the elf bytes
  /// @param[in] elf_size the elf size
  ///
  /// @returns a \ref LoadCodeResult with the EventId to sync with (if needed), the kernelId to utilize in later
  /// kernelLaunch and the kernel load address.
  ///
  /// NOTE: remember to not deallocate the elf memory \param elf until the EventId from \ref LoadCodeResult is completed
  LoadCodeResult loadCode(StreamId stream, const std::byte* elf, size_t elf_size);

  /// \brief Unloads a previously loaded elf code, identified by the kernel handler
  ///
  /// @param[in] kernel a handler to the code that must be unloaded
  ///
  void unloadCode(KernelId kernel);

  /// \brief Queues a execution work into a stream. The work is identified by the kernel handler, which has been
  /// previously loaded into the device which is associated to the stream. The parameters of the kernel are given by
  /// kernel_args and kernel_args_size; these parameters will be copied from the host memory to the device memory by
  /// the runtime. The firmware will load these parameters into RA register, the kernel code should cast this register
  /// to the expected types. The kernel execution is always asynchronous.
  ///
  /// @param[in] stream handler indicating in which stream the kernel will be executed. The kernel code have to be
  /// registered into the device associated to the stream previously.
  /// @param[in] kernel handler which indicate what code to execute in the device.
  /// @param[in] kernel_args buffer containing all the parameters to be copied to the device memory prior to the code
  /// execution
  /// @param[in] kernel_args_size size of the kernel_args buffer
  /// @param[in] kernelLaunchOptions contains all configurable kernel parameters
  EventId kernelLaunch(StreamId stream, KernelId kernel, const std::byte* kernel_args, size_t kernel_args_size,
                       const KernelLaunchOptions& kernelLaunchOptions = KernelLaunchOptions());

  /// \deprecated See kernelLaunch using KernelLaunchOptions
  ///
  /// @param[in] stream handler indicating in which stream the kernel will be executed. The kernel code have to be
  /// registered into the device associated to the stream previously.
  /// @param[in] kernel handler which indicate what code to execute in the device.
  /// @param[in] kernel_args buffer containing all the parameters to be copied to the device memory prior to the code
  /// execution
  /// @param[in] kernel_args_size size of the kernel_args buffer
  /// @param[in] shire_mask indicates in what shires the kernel will be executed
  /// @param[in] barrier this parameter indicates if the kernel execution should be postponed till all previous works
  /// issued into this stream finish (a barrier). Usually the kernel launch must be postponed till some previous
  /// memory operations end, hence the default value is true.
  /// @param[in] flushL3 this parameter indicates if the L3 should be flushed before the kernel execution starts.
  /// @param[in] userTraceConfig this parameter can be null or point to a device buffer (previously allocated with
  /// \ref mallocDevice). If the pointer is not null, then the firmware will utilize this buffer to fill-up user trace
  /// data. This buffer must be of size 4KB * num_harts (4KB*2080). We will provide later on an API to allocate the
  /// buffer with the size required.
  /// @param[in] coreDumpFilePath this string, if not empty, will contain a valid file path to store a coredump in case
  /// the kernel execution produces an error. File must be accessible by the user running the application, file will be
  /// created or overwritten if existing.
  ///
  /// @returns EventId is a handler of an event which can be waited for (waitForEventId) to synchronize when the kernel
  /// ends the execution.
  ///
  EventId kernelLaunch(StreamId stream, KernelId kernel, const std::byte* kernel_args, size_t kernel_args_size,
                       uint64_t shire_mask, bool barrier = true, bool flushL3 = false,
                       std::optional<UserTrace> userTraceConfig = std::nullopt,
                       const std::string& coreDumpFilePath = "");

  /// \brief Queues a memcpy operation from host memory to device memory. The device memory must be previously
  /// allocated by a mallocDevice.
  ///
  /// @param[in] stream handler indicating in which stream to queue the memcpy operation
  /// @param[in] h_src host memory buffer to copy from
  /// @param[in] d_dst device memory buffer to copy to
  /// @param[in] size indicates the size of the memcpy
  /// @param[in] barrier this parameter indicates if the memcpy operation should be postponed till all previous works
  /// issued into this stream finish (a barrier). Usually the memcpies from host to device can run in parallel, hence
  /// the default value is false All memcpy operations are always asynchronous.
  /// @param[in] cmaCopyFunction this parameter allows to customize the function used to copy from user-space virtual
  /// memory to the CMA buffer. Intended for internal usage, regular API user shouldn't modify the default value.
  ///
  /// @returns EventId is a handler of an event which can be waited for (waitForEventId) to synchronize when the memcpy
  /// ends.
  ///
  /// NOTE: the host memory pointer must be kept alive until the operation has completely ended in device.
  ///
  EventId memcpyHostToDevice(StreamId stream, const std::byte* h_src, std::byte* d_dst, size_t size,
                             bool barrier = false, const CmaCopyFunction& cmaCopyFunction = defaultCmaCopyFunction);

  /// \brief Queues a memcpy operation from device memory to host memory. The device memory must be a valid region
  /// previously allocated by a mallocDevice; the host memory must be a previously allocated memory in the host by the
  /// conventional means (for example the heap)
  ///
  /// @param[in] stream handler indicating in which stream to queue the memcpy operation
  /// @param[in] d_src device memory buffer to copy from
  /// @param[in] h_dst host memory buffer to copy to
  /// @param[in] size indicates the size of the memcpy
  /// @param[in] barrier this parameter indicates if the memcpy operation should be postponed till all previous works
  /// issued into this stream finish (a barrier). Usually the memcpies from device to host must wait till a previous
  /// kernel execution finishes, hence the default value is true All memcpy operations are always asynchronous.
  /// @param[in] cmaCopyFunction this parameter allows to customize the function used to copy from user-space virtual
  /// memory to the CMA buffer. Intended for internal usage, regular API user shouldn't modify the default value.
  ///
  /// @returns EventId is a handler of an event which can be waited for (waitForEventId) to synchronize when the memcpy
  /// ends.
  ///
  /// NOTE: the host memory pointer must be kept alive until the operation has completely ended in device.
  ///
  EventId memcpyDeviceToHost(StreamId stream, const std::byte* d_src, std::byte* h_dst, size_t size,
                             bool barrier = true, const CmaCopyFunction& cmaCopyFunction = defaultCmaCopyFunction);

  /// \brief Queues many memcpy operations from host memory to device memory. These operations are defined in struct
  /// \ref MemcpyList. The device memory must be a valid region previously allocated by a mallocDevice; the host memory
  /// must be a previously allocated memory in the host by the conventional means (for example the heap).
  ///
  /// @param[in] stream handler indicating in which stream to queue the memcpy
  /// operation
  /// @param[in] memcpyList contains all the operations required. See \ref MemcpyList
  /// @param[in] barrier this parameter indicates if the memcpy operation should be postponed till all previous works
  /// issued into this stream finish (a barrier). Usually the memcpies from device to host must wait till a previous
  /// kernel execution finishes, hence the default value is false. All memcpy operations are always asynchronous.
  /// @param[in] cmaCopyFunction this parameter allows to customize the function used to copy from user-space virtual
  /// memory to the CMA buffer. Intended for internal usage, regular API user shouldn't modify the default value.
  ///
  /// @returns EventId is a handler of an event which can be waited for (waitForEventId) to synchronize when the memcpy
  /// ends.
  ///
  EventId memcpyHostToDevice(StreamId stream, MemcpyList memcpyList, bool barrier = false,
                             const CmaCopyFunction& cmaCopyFunction = defaultCmaCopyFunction);

  /// \brief Queues many memcpy operations from device memory to host memory. These operations are defined in struct
  /// \ref MemcpyList. The device memory must be a valid region previously allocated by a mallocDevice; the host memory
  /// must be a previously allocated memory in the host by the conventional means (for example the heap).
  ///
  /// @param[in] stream handler indicating in which stream to queue the memcpy
  /// operation
  /// @param[in] memcpyList contains all the operations required. See \ref MemcpyList
  /// @param[in] barrier this parameter indicates if the memcpy operation should be postponed till all previous works
  /// issued into this stream finish (a barrier). Usually the memcpies from device to host must wait till a previous
  /// kernel execution finishes, hence the default value is true All memcpy operations are always asynchronous.
  /// @param[in] cmaCopyFunction this parameter allows to customize the function used to copy from user-space virtual
  /// memory to the CMA buffer. Intended for internal usage, regular API user shouldn't modify the default value.
  ///
  /// @returns EventId is a handler of an event which can be waited for (waitForEventId) to synchronize when the memcpy
  /// ends.
  ///
  EventId memcpyDeviceToHost(StreamId stream, MemcpyList memcpyList, bool barrier = true,
                             const CmaCopyFunction& cmaCopyFunction = defaultCmaCopyFunction);

  /// \brief Queues a device to device memcpy operation. The source device will be the corresponding to streamSrc, the
  /// destination device is provided in deviceDst. The device memory must be a valid region previously allocated by a
  /// mallocDevice, in both devices. The operation will be queued into the source device, hence it can be synced using
  /// streamSrc (or resulting eventId). Take into account that if synchronization is required on the destination device
  /// it needs to be done on higher level; or use instead the other memcpyDeviceToDevice operation.
  ///
  /// @param[in] streamSrc handler indicating in which stream to queue the memcpy operation. This op will be queued in
  /// the source device, hence the stream must correspond to a source device.
  /// @param[in] deviceDst handler indicating the destination device of the memcpy operation.
  /// @param[in] d_src device memory buffer on source device to copy from.
  /// @param[in] d_dst device memory buffer on destination device to copy to.
  /// @param[in] size indicates the size of the memcpy.
  /// @param[in] barrier this parameter indicates if the memcpy operation should be postponed till all previous works
  /// issued into this stream finish (a barrier). All memcpy operations are always asynchronous.
  /// @returns EventId is a handler of an event which can be waited for (waitForEventId) to synchronize when the memcpy
  /// ends.
  ///
  EventId memcpyDeviceToDevice(StreamId streamSrc, DeviceId deviceDst, const std::byte* d_src, std::byte* d_dst,
                               size_t size, bool barrier = true);

  /// \brief Queues a device to device memcpy operation. The source device is provided in deviceSrc, the
  /// destination device will be the corresponding to streamDst. The device memory must be a valid region previously
  /// allocated by a mallocDevice, in both devices. The operation will be queued into the destination device, hence it
  /// can be synced using streamDst (or resulting eventId). Take into account that if synchronization is required on the
  /// source device it needs to be done on higher level; or use instead the other memcpyDeviceToDevice operation.
  ///
  /// @param[in] deviceSrc handler indicating the source device of the memcpy operation.
  /// @param[in] streamDst handler indicating in which stream to queue the memcpy operation. This op will be queued in
  /// the destination device, hence the stream must correspond to a destination device.
  /// @param[in] d_src device memory buffer on source device to copy from.
  /// @param[in] d_dst device memory buffer on destination device to copy to.
  /// @param[in] size indicates the size of the memcpy.
  /// @param[in] barrier this parameter indicates if the memcpy operation should be postponed till all previous works
  /// issued into this stream finish (a barrier). All memcpy operations are always asynchronous.
  /// @returns EventId is a handler of an event which can be waited for (waitForEventId) to synchronize when the memcpy
  /// ends.
  ///
  EventId memcpyDeviceToDevice(DeviceId deviceSrc, StreamId streamDst, const std::byte* d_src, std::byte* d_dst,
                               size_t size, bool barrier = true);

  /// \brief This will block the caller thread until the given event is dispatched or the timeout is reached. This
  /// primitive allows to synchronize with the device execution.
  ///
  /// @param[in] event is the event to wait for, result of a memcpy operation or a
  /// kernel launch.
  /// @param[in] timeout is the number of seconds to wait till aborting the wait. If timeout is 0 seconds, then it
  /// won't block in any case.
  ///
  /// @returns false if the timeout is reached, true otherwise.
  ///
  bool waitForEvent(EventId event, std::chrono::seconds timeout = std::chrono::hours(24));

  /// \brief This will block the caller thread until all commands issued to the given stream finish or if the timeout
  /// is reached. This primitive allows to synchronize with the device execution.
  ///
  /// @param[in] stream this is the stream to synchronize with.
  /// @param[in] timeout is the number of seconds to wait till aborting the wait. If timeout is 0 seconds, then it
  /// won't block in any case.
  ///
  /// @returns false if the timeout is reached, true otherwise.
  ///
  bool waitForStream(StreamId stream, std::chrono::seconds timeout = std::chrono::hours(24));

  /// \brief This will return a list of errors and their execution context (if any)
  ///
  /// @param[in] stream this is the stream to synchronize with.
  ///
  /// @returns StreamStatus contains the state of the associated stream.
  ///
  std::vector<StreamError> retrieveStreamErrors(StreamId stream);

  /// \brief This callback (when set) will be automatically called when a kernel abort happens. This is implemented
  /// as a workaround (SW-13045).
  ///
  /// @param[in] callback see \ref rt::KernelAbortedCallback. This is the callback which will be called when a kernel
  /// abort happens
  ///
  void setOnKernelAbortedErrorCallback(const KernelAbortedCallback& callback);

  /// \brief This callback (when set) will be automatically called when a new StreamError occurs, making the polling
  /// through retrieveStreamErrors unnecessary.
  ///
  /// @param[in] callback see \ref rt::StreamErrorCallback. This is the callback which will be called when a StreamError
  /// occurs.
  ///
  void setOnStreamErrorsCallback(StreamErrorCallback callback);

  /// \brief Sets a profiler to be used by runtime.
  ///
  /// @param[in] profiler see \ref rt::profiling::IProfilerRecorder. This is the profiler that the runtime will use to
  /// store and process the traces.
  ///
  void setProfiler(std::unique_ptr<profiling::IProfilerRecorder>&& profiler) {
    profiler_ = std::move(profiler);
    onProfilerChanged();
  }

  /// \brief Returns a pointer to the profiler interface; don't delete/free this pointer since this is owned by the
  /// runtime itself.
  ///
  /// @returns IProfilerRecorder an interface to the profiler. See \ref rt::profiling::IProfilerRecorder
  ///
  profiling::IProfilerRecorder* getProfiler() const {
    return profiler_.get();
  }

  /// \brief Instructs the device to abort given command.
  /// NOTE: as per current firmware implementation, aborting a command will have undesirable side effects on the rest
  /// of the submitted commands to the same virtual queue. So, after aborting a command it could potentially affect the
  /// rest of the executions
  /// BUG: individual memcpyHostToDevice and memcpyDeviceToHost commands can not be aborted individually right now. The
  /// could be aborted through \ref abortStream.
  ///
  /// @param[in] commandId indicates the command's eventId to abort
  /// @param[in] timeout indicates the maximum time trying to QUEUE an abort command into the device. If the timeout is
  /// reached and the runtime was not able to push the command into the device, an Exception will be thrown
  ///
  /// @returns EventId is a handler of the abort command itself which can be waited for (waitForEventId) to
  /// synchronize.
  ///
  EventId abortCommand(EventId commandId, std::chrono::milliseconds timeout = std::chrono::milliseconds(5000));

  /// \brief Instructs the device to abort all outstanding commands on a given stream. This will affect to all commands
  /// previously issued to given stream, later commands will execute normally. NOTE: as per current firmware
  /// implementation, aborting a command will have undesirable side effects on the rest of the submitted commands to
  /// the same virtual queue. So, after aborting a command it could potentially affect the rest of the executions.
  ///
  /// @param[in] streamId indicates the stream to abort commands
  ///
  /// @returns EventId is a handler of the abortStream which can be waited for (waitForEventId) to synchronize.
  EventId abortStream(StreamId streamId);

  /// \brief Returns the DmaInfo of given device (see \ref DmaInfo). This DmaInfo contains max size for each operation
  /// in a memcpy list command and max number of operations allowed in a single memcpy list command.
  ///
  /// @param[in] deviceId indicates the device to get the DMA configuration from.
  ///
  /// @returns DmaInfo contains max number of operations allowed in a single memcpy list command and max size for each
  /// operation.
  DmaInfo getDmaInfo(DeviceId deviceId) const;

  /// \brief Returns if memcpy P2P is enabled between both devices.
  ///
  /// @param[in] one the first device to check for p2p capabilities.
  /// @param[in] other the other device to check for p2p capabilities.
  ///
  /// @returns true if both devices can communicate directly through p2p, false otherwise.
  ///
  bool isP2PEnabled(DeviceId one, DeviceId other) const;

  /// \brief Virtual Destructor to enable polymorphic release of the runtime instances
  virtual ~IRuntime();
  ///

  /// \brief Constructor to initialize with a dummy profiler
  IRuntime();
  ///

  ///
  /// \brief Factory method to instantiate a standalone IRuntime implementation
  ///
  /// @param[in] deviceLayer is the deviceLayer implementation that runtime will use. See \ref dev::IDeviceLayer
  /// @param[in] options can set some runtime parameters. See \ref rt::Options
  ///
  /// @returns RuntimePtr an IRuntime instance.
  ///
  static RuntimePtr create(std::shared_ptr<dev::IDeviceLayer> const& deviceLayer,
                           Options options = getDefaultOptions());

  ///
  /// \brief Factory method to instantiate a client IRuntime implementation
  ///
  /// @param[in] socketPath indicates which socket the Client will connect to
  ///
  /// @returns RuntimePtr an IRuntime instance.
  ///
  static RuntimePtr create(const std::string& socketPath);

private:
  std::unique_ptr<profiling::IProfilerRecorder> profiler_;

  // NVI applied, all public interface is non-virtual; customization is in the private part
  virtual std::vector<DeviceId> doGetDevices() = 0;

  virtual DeviceProperties doGetDeviceProperties(DeviceId device) const = 0;

  virtual LoadCodeResult doLoadCode(StreamId stream, const std::byte* elf, size_t elf_size) = 0;
  virtual void doUnloadCode(KernelId kernel) = 0;

  virtual std::byte* doMallocDevice(DeviceId device, size_t size, uint32_t alignment = kCacheLineSize) = 0;
  virtual void doFreeDevice(DeviceId device, std::byte* buffer) = 0;

  virtual StreamId doCreateStream(DeviceId device) = 0;
  virtual void doDestroyStream(StreamId stream) = 0;

  virtual EventId doKernelLaunch(StreamId stream, KernelId kernel, const std::byte* kernel_args,
                                 size_t kernel_args_size, const KernelLaunchOptionsImp& options) = 0;

  virtual EventId doMemcpyHostToDevice(StreamId stream, const std::byte* src, std::byte* dst, size_t size, bool barrier,
                                       const CmaCopyFunction& cmaCopyFunction) = 0;
  virtual EventId doMemcpyDeviceToHost(StreamId stream, const std::byte* src, std::byte* dst, size_t size, bool barrier,
                                       const CmaCopyFunction& cmaCopyFunction) = 0;
  virtual EventId doMemcpyHostToDevice(StreamId stream, MemcpyList memcpyList, bool barrier,
                                       const CmaCopyFunction& cmaCopyFunction) = 0;
  virtual EventId doMemcpyDeviceToHost(StreamId stream, MemcpyList memcpyList, bool barrier,
                                       const CmaCopyFunction& cmaCopyFunction) = 0;

  virtual EventId doMemcpyDeviceToDevice(StreamId streamSrc, DeviceId deviceDst, const std::byte* d_src,
                                         std::byte* d_dst, size_t size, bool barrier) = 0;
  virtual EventId doMemcpyDeviceToDevice(DeviceId deviceSrc, StreamId streamDst, const std::byte* d_src,
                                         std::byte* d_dst, size_t size, bool barrier) = 0;

  virtual bool doWaitForEvent(EventId event, std::chrono::seconds timeout = std::chrono::hours(24)) = 0;
  virtual bool doWaitForStream(StreamId stream, std::chrono::seconds timeout = std::chrono::hours(24)) = 0;

  virtual EventId doAbortCommand(EventId commandId,
                                 std::chrono::milliseconds timeout = std::chrono::milliseconds(5000)) = 0;

  virtual EventId doAbortStream(StreamId streamId) = 0;

  virtual void doSetOnStreamErrorsCallback(StreamErrorCallback callback) = 0;

  virtual void doSetOnKernelAbortedErrorCallback(const KernelAbortedCallback& callback) = 0;

  virtual std::vector<StreamError> doRetrieveStreamErrors(StreamId stream) = 0;

  virtual DmaInfo doGetDmaInfo(DeviceId deviceId) const = 0;

  virtual void onProfilerChanged(){/* defaults do nothing */};

  virtual bool doIsP2PEnabled(DeviceId, DeviceId) const {
    return false;
  }
};

} // namespace rt
  /// @}
  // End of runtime_api
