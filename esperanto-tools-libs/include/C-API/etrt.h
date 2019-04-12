/**
 * Copyright (C) 2018, Esperanto Technologies Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @file etrt.h
 * @brief ET Runtime Library Interface
 * @version 0.1.0
 *
 * @defgroup ETCRT_API  ET Runtime API
 *
 * Overview
 * ========
 * This API is the primary means by which application code running on a host
 * makes use of the Esperanto SoC's acceleration capabilities. The main features
 * of this API are (host-/device-side) memory management, host-device
 * synchronization, and the launching of *kernels* -- i.e., blocks of code that
 * are distributed among the SoC's Minions to be executed (in parallel).
 *
 * The current computational model provided by this API restricts the use of an
 * individual ET SoC to a single host process at any point in time (with serial
 * reuse permitted).  However, this API assumes that a given host process can
 * have multiple threads of execution, each of which could be accessing the
 * device attached to their parent process on the host.
 *
 * The key abstractions supported by this API are: Kernels, Streams, and Events.
 *
 * Kernels are the units of computation that are accelerated by the ET SoC.
 * Kernels are defined by a block of machine instructions and are executed as
 * many parallel instances which divide up the work needed to complete the top
 * level computation defined by the Kernel. This API provides the client with a
 * means of expressing how to distribute and organize the execution of multiple
 * instances of the Kernel.  The API also provides a means by which the Kernel
 * instances can be differentiated in order to collectively perform the required
 * computation.
 *
 * Kernels are launched within virtual work-queues known as Streams, and their
 * execution is performed asynchronously with respect to the execution of
 * Kernels on other Streams (as well as the continued execution of code on the
 * host).  Operations executed within a given Stream are executed sequentially,
 * and there are no execution constraints or guarantees on the order of
 * execution of operations among different Streams.
 *
 * The host code can synchronize with the execution of operations on devices by
 * waiting for all of the previously issued operations on a given Stream to be
 * completed (successfully or otherwise). In addition, this API provides client
 * code with the ability to do finer-grained synchronization with attached
 * devices through the use of the Event abstraction. Events can be thought of as
 * containers for snapshots of pending work on a device, and they can be used to
 * indicate when the defined work has been completed.
 *
 * In addition to these abstractions, this API supports the copying of regions
 * of memory between all combinations of hosts and devices.  The API abstracts
 * away many of the lower level details of these memory transfer operations, and
 * enables asynchronous operation to allow the overlapping of data movement and
 * computations.
 *
 * **N.B.** Hereafter, an ET SoC (of any variety or version) is referred to as a
 * "Device," the processing node to which the ET SoC is attached is known as the
 * "Host," and the ET Runtime API will be known as the "API."
 *
 * @todo add the ability to JIT source-code files into Kernels from this API
 * @todo finish the definition of Kernel launch semantics (and define the option
 * flags)
 * @todo add links to data structure definitions
 * @todo get auto-linking to typedefs and enums working
 * @todo Document other files
 *
 */

#ifndef ETRT_H
#define ETRT_H

#include "et-misc.h"
#include "etrt-bin.h"

// Registry public interface.
EXAPI void **__etrtRegisterFatBinary(void *fatCubin);
EXAPI void __etrtUnregisterFatBinary(void **fatCubinHandle);
EXAPI void __etrtRegisterFunction(void **fatCubinHandle, const char *hostFun,
                                  char *deviceFun, const char *deviceName,
                                  int thread_limit, uint3 *tid, uint3 *bid,
                                  dim3 *bDim, dim3 *gDim, int *wSize);
EXAPI void __etrtRegisterVar(void **fatCubinHandle, char *hostVar,
                             char *deviceAddress, const char *deviceName,
                             int ext, size_t size, int constant, int global);

/**
 * @addtogroup ETCRT_ERROR_HANDLING ET Runtime API Error Handling
 * @ingroup  ETCRT_API
 * @{
 *
 * All calls to this API return a common enum (i.e., `etrtError`) that indicates
 * whether the call succeeded (via `etrtErrorSuccess`), or failed (in which case
 * the return value is an error code).
 */

/**
 * @brief  Return the description string for an API call return value.
 *
 * Take an API return value and return a pointer to the string that describes
 the result of an API call.
 *
 * @param[in] error  The value returned by an API call.
 * @return  A string that describes the result of an API call.

 */
EXAPI const char *etrtGetErrorString(etrtError_t error);

/**
 * @brief  Returns the last error from an API call.
 *
 * Returns the last ET Runtime error generated by calls to the API, or value of
 * etrtSuccess if no error occurred since the last time this was called.
 *
 * TODO define how this works with respect to other client processes -- probably
 caller-specific
 *
 * @return  etrtSuccess, etrtErrorMissingConfiguration,
 etrtErrorMemoryAllocation, etrtErrorInitializationError,
 * etrtErrorLaunchFailure, etrtErrorLaunchTimeout,
 etrtErrorLaunchOutOfResources, etrtErrorInvalidDeviceFunction,
 * etrtErrorInvalidConfiguration, etrtErrorInvalidDevice, etrtErrorInvalidValue,
 etrtErrorInvalidPitchValue,
 * etrtErrorInvalidSymbol, etrtErrorUnmapBufferObjectFailed,
 etrtErrorInvalidDevicePointer, etrtErrorInvalidTexture,
 * etrtErrorInvalidTextureBinding, etrtErrorInvalidChannelDescriptor,
 etrtErrorInvalidMemcpyDirection,
 * etrtErrorInvalidFilterSetting, etrtErrorInvalidNormSetting, etrtErrorUnknown,
 etrtErrorInvalidResourceHandle,
 * etrtErrorInsufficientDriver, etrtErrorNoDevice, etrtErrorSetOnActiveProcess,
 etrtErrorStartupFailure,
 * etrtErrorInvalidPtx, etrtErrorNoKernelImageForDevice,
 etrtErrorJitCompilerNotFound

 */
EXAPI etrtError_t etrtGetLastError(void);

/**
 * @}
 */

/**
 * @addtogroup ETCRT_DEVICE_MGMT ET Runtime API Device Management
 * @ingroup  ETCRT_API
 * @{
 *
 * This API provides the calling processes with a means to identify and query
 * the Devices available to them on their Host.
 *
 * Available Device instances are identified with ordinal values (represented as
 * small integers).  These values are used to attach a caller to a specific
 * Device.  Once attached, subsequent API calls are implicitly directed to the
 * attached Device.  Callers can detach from the attached Device and reattach to
 * a Device, in a serial fashion.
 *
 * Currently, at most one Host process can be attached to a Device at any time.
 * A Host process can be simultaneously attached to multiple Devices.
 */

/**
 * @brief  Return the number of Devices found on this Host.
 *
 * Returns a count of the number of Devices currently attached and accessible on
 * the Host.
 *
 * @param[out] count  A pointer to the location where the count of number of
 * Devices is to be written.
 * @return  etrtSuccess
 */
EXAPI etrtError_t etrtGetDeviceCount(int *count);

/**
 * @brief  Return information about a given Device
 *
 * Takes the ordinal representing one of the Host's currently attached and
 * accessible Devices and returns a structure with information about the
 * associated Device in the location given by the pointer argument.
 *
 * TODO describe (or reference the data structure) the information provided for
 * each Device
 *
 * @param[out] prop  A pointer to a data structure that is to be filled in with
 * Device information upon return.
 * @param[in]  device  An ordinal that refers to the (local) Device that is to
 * be queried.
 * @return  etrtSuccess, etrtErrorInvalidDevice
 */
EXAPI etrtError_t etrtGetDeviceProperties(struct etrtDeviceProp *prop,
                                          int device);

/**
 * @brief  Returns the number of the Device currently in use by the calling
 * process.
 *
 * Returns the ordinal representing the Device to which the caller's thread is
 * currently attached. A NIL value is returned if the caller's thread is
 * currently not attached to a Device.
 *
 * @param[out] device  A pointer to the location into which the Device number is
 * to be written.
 * @return  etrtSuccess, etrtErrorInvalidValue
 */
EXAPI etrtError_t etrtGetDevice(int *device);

/**
 * @brief  Attach a Device to the calling process.
 *
 * Select the Device to be used by subsequent calls to the API by the caller's
 * process. For this to succeed, A valid Device number must be given, and the
 * indicated Device must not be attached to another process.  If the indicated
 * Device is currently attached to the caller's process, then this has no effect
 * (and returns ????).
 *
 * @param[in] device  The ordinal that represents the Device to be used by the
 * caller's process.
 * @return  etrtSuccess, etrtErrorInvalidDevice, etrtErrorDeviceAlreadyInUse
 */
EXAPI etrtError_t etrtSetDevice(int device);

/**
 * @brief  Return the version of the ET Device Driver that is being used.
 *
 * Returns a string containing the (SemVer 2.0) version number for the ET Device
 * Driver that is currently being used.
 *
 * @return  etrtSuccess
 */
EXAPI const char *etrtGetDriverVersion();

/**
 * @brief  Detach the currently connected Device from the caller's process.
 *
 * Release the calling process's attached Device, or do nothing if a Device is
 * currently not attached. This results in the calling process's context being
 * freed, all of the connected Device's resources associated with this process
 * being released, and the given Device being made able to be connected to once
 * again.
 *
 * @return  etrtSuccess
 */
EXAPI etrtError_t etrtResetDevice();

/**
 * @}
 */

/**
 * @addtogroup ETCRT_MEMORY_MGMT ET Runtime API Memory Management
 * @ingroup  ETCRT_API
 * @{
 *
 * This API provides calls that can be used to allocate and free memory on both
 * the Host and Device, as well as directing memory transfers to take place
 * among the Host and Devices.
 *
 * TODO  Describe the whole deal with Host versus device addresses -- how
 * they're used and how to translate between them
 *
 *
 * **Memory (De)Allocation**
 *
 * Separate sets of functions are used to allocate and free memory on the Host
 * and Device.  It is up to the user to keep track of which pointers refer to
 * regions of allocated memory on the different physical devices.
 *
 * This API abstracts away all of the details of memory management (e.g.,
 * alignment, pinning, etc.) and provides the caller with a simple malloc/free
 * interface where the caller just deals with pointers and sizes (in number of
 * bytes).
 *
 * The runtime environment automatically deals with reclaiming orphaned Device
 * memory resources when a calling process exits.
 *
 * **Memory Transfers**
 *
 * This API allows a caller to transfer regions of memory from the Host to a
 * Device, from a Device to a Host, and from a Device to (the same or another)
 * Device. All transfers must be to/from regions of memory that are currently
 * allocated by the calling process. In principle, there are both synchronous
 * and asynchronous versions of the memory copy functions. However, for
 * Host-to-Host copies, the (so-named) asynchronous calls execute synchronously.
 *
 */

/**
 * @brief  Allocate memory on the Host.
 *
 * Take a byte count and return a pointer (in the given pointer-pointer) to that
 * number of (contiguous, long-word aligned, pinned) bytes of memory on the
 * Host. This will return a failure indication if it is not possible to meet the
 * given request.
 *
 * @param[out] ptr  A pointer to a pointer where the allocated Host memory
 * region is located.
 * @param[in]  size  The number of bytes of memory that should be allocated on
 * the Host.
 * @return  etrtSuccess, etrtErrorInvalidValue, etrtErrorMemoryAllocation
 */
EXAPI etrtError_t etrtMallocHost(void **ptr, size_t size);

/**
 * @brief  Free allocated Host memory.
 *
 * Take a pointer to a location in Host memory and free it.
 * This call will fail if the given pointer does not correspond to the start of
 * a region of Host memory that was not previously allocated (via
 * etrtMallocHost) by the caller's thread.
 *
 * @param[in] ptr  A pointer to a previously allocated region of Host memory.
 * @return  etrtSuccess, etrtErrorInvalidValue
 */
EXAPI etrtError_t etrtFreeHost(void *ptr);

/**
 * @brief  Allocate memory on the Device.
 *
 * Take a byte count and return a pointer (in the given pointer-pointer) to that
 * number of (contiguous, long-word aligned) bytes of shared global memory on
 * the calling thread's currently attached Device. The allocated Device memory
 * region is associated with the calling thread and will be automatically freed
 * when the calling thread exits. This will return a failure indication if it is
 * not possible to meet the given request.
 *
 * @param[out] devPtr  A pointer to where the pointer to the allocated Device
 * memory region is to be written.
 * @param[in]  size  The number of bytes of memory that should be allocated on
 * the Device.
 * @return  etrtSuccess, etrtErrorInvalidValue, etrtErrorMemoryAllocation
 */
EXAPI etrtError_t etrtMalloc(void **devPtr, size_t size);

/**
 * @brief  Free allocated Device memory.
 *
 * Take a pointer to a location in Device memory and free it.
 * This call will fail if the given pointer does not correspond to the start of
 * a region of Device memory that was not previously allocated (via etrtMalloc)
 * by the caller's thread.
 *
 * @param[in] devPtr  A pointer to a previously allocated region of Device
 * memory.
 * @return  etrtSuccess, etrtErrorInvalidValue
 */
EXAPI etrtError_t etrtFree(void *devPtr);

/**
 * @brief Get information related to a region of memory allocated by this API.
 *
 * Given a pointer to a region of (Host or Device) memory currently allocated by
 * the calling process, return information about that memory region. The
 * returned information includes: where the region resides (i.e., on the Host or
 * on an attached Device), the Device on which the region exists (should it not
 * be on the Host), the Host address that corresponds to a Device's memory
 * location, and the Device address that corresponds to a Host memory location.
 *
 * TODO add a reference to the `etrtPointerAttributes` data structure
 *
 * @param[in] attributes  A pointer to the data structure that is to be filled
 * with memory region information.
 * @param[in] ptr  A pointer to an allocated region of (Host or Device) memory
 * @return  etrtSuccess, etrtErrorInvalidValue
 */
EXAPI etrtError_t etrtPointerGetAttributes(
    struct etrtPointerAttributes *attributes, const void *ptr);

/**
 * @brief  Asynchronously copy the contents of one region of allocated memory to
 * another.
 *
 * Initiate the copying of the given number of bytes from the given source
 * location to the given destination location.  The source and destination
 * regions can be other either the Host or an attached Device, but both regions
 * must be currently allocated by the calling process using this API. This call
 * returns immediately and the transfer can be performed asynchronously with
 * respect to the the execution of operations on other Streams.  The caller can
 * synchronize with the completion of the transfer by using the Stream
 * synchronization functions.
 *
 * @param[in] dst  A pointer to the location where the memory is to be copied.
 * @param[in] src  A pointer to the location from which the memory is to be
 * copied.
 * @param[in] count  The number of bytes that should be copied.
 * @param[in] kind  Enum indicating the direction of the transfer (i.e., H->H,
 * H->D, D->H, or D->D).
 * @param[in] stream  The Stream upon which this operation is to be performed.
 * Passing an empty stream (i.e., a NULL) means that it is to be executed on the
 * default Stream.
 * @return  etrtSuccess, etrtErrorInvalidValue, etrtErrorInvalidMemoryDirection
 */
EXAPI etrtError_t etrtMemcpyAsync(void *dst, const void *src, size_t count,
                                  enum etrtMemcpyKind kind,
                                  etrtStream_t stream);

/**
 * @brief  Copy the contents of one region of allocated memory to another.
 *
 * Initiate the copying of the given number of bytes from the given source
 * location to the given destination location.  The source and destination
 * regions can be other either the Host or an attached Device, but both regions
 * must be currently allocated by the calling process using this API. This call
 * returns when the transfer has completed (or a failure occurs).
 *
 * @param[in] dst  A pointer to the location where the memory is to be copied.
 * @param[in] src  A pointer to the location from which the memory is to be
 * copied.
 * @param[in] count  The number of bytes that should be copied.
 * @param[in] kind  Enum indicating the direction of the transfer (i.e., H->H,
 * H->D, D->H, or D->D).
 * @return  etrtSuccess, etrtErrorInvalidValue, etrtErrorInvalidMemoryDirection
 */
EXAPI etrtError_t etrtMemcpy(void *dst, const void *src, size_t count,
                             enum etrtMemcpyKind kind);

/**
 * @brief  Sets the bytes in allocated memory region to a given value.
 *
 * Writes a given number of bytes in an allocated memory region to a given byte
 * value. This function executes asynchronously, unless the target memory
 * address refers to a pinned Host memory region.
 *
 * @param[in] ptr  Pointer to location of currently allocated memory region that
 * is to be written.
 * @param[in] value  Constant byte value to write into the given memory region.
 * @param[in] count  Number of bytes to be written with the given value.
 * @return  etrtSuccess, etrtErrorInvalidValue
 */
EXAPI etrtError_t etrtMemset(void *ptr, int value, size_t count);

/**
 * @}
 */

/**
 * @addtogroup ETCRT_STREAM_MGMT ET Runtime API Stream Management
 * @ingroup  ETCRT_API
 * @{
 *
 * Streams are essentially independent work queues for executing operations in
 * parallel. Operations can be issued on specific Streams, with the guarantee
 * that they execute sequentially (in the order of their issue) within the
 * Stream.  No guarantees are made on the order of execution of operations among
 * different Streams.
 *
 * If new Streams are not explicitly created, execution is done on the Default
 * Stream (i.e., Stream 0). A degree of parallelism can be achieved by executing
 * sequences of asynchronous operations on a single Stream.  However, additional
 * degrees of (high-level) parallelism (and programming independence) can be
 * achieved by creating additional Streams and executing sequences of operations
 * on each of them in parallel. Streams help simplify the management of
 * independent activities of complex applications -- e.g., separate portions of
 * an application executed by multiple (child) threads of the (paraent) calling
 * process. Synchronization and execution-cycle management can be done at the
 * level of Streams, e.g., waiting for all of the operations on a particular
 * Stream to complete, or creating/deleting an entire Stream. Note that this is
 * in addition to the finer-grained type of synchronization that can be
 * accomplished with the Event mechanism.
 *
 * This API chooses to have the Default Stream follow the per-thread Stream
 * semantics of CUDA (as opposed to CUDA's Legacy Stream semantics).
 */

/**
 * @brief  Create a new Stream.
 *
 * Return the handle for a newly created Stream for the caller's process.
 *
 * The `flags` argument allows the caller to have control over the behavior of
 * the newly created Stream. If a 0 (or `cudaStreamDefault`) value is given,
 * then the Stream is created using the default options. If the
 * `cudaStreamNonBlocking` value is given, then the newly created Stream may run
 * concurrently with the Default Stream (i.e., Stream 0), and it performs no
 * implicit synchronization with the Default Stream.
 *
 * @param[in] pStream  A Pointer to where the new stream handle is to be
 * written.
 * @param[in] flags  A bitmap composed of flags that select options for the new
 * Stream.
 * @return  etrtSuccess, etrtErrorInvalidValue
 */
// @todo FIXME
// EXAPI etrtError_t etrtStreamCreateWithFlags(etrtStream_t *pStream, unsigned
// int flags);

EXAPI etrtError_t etrtStreamCreate(etrtStream_t *pStream);
EXAPI etrtError_t etrtStreamCreateWithFlags(etrtStream_t *pStream,
                                            unsigned int flags);

/**
 * @brief  Delete an existing Stream.
 *
 * Delete the stream associated with the given Stream handle and free any
 * resources associated with the Stream. After calling this function, no new
 * operations can be executed he given Stream and all in-progress and pending
 * operations are completed before the Stream is deallocated and its resources
 * freed. This function returns immediately and the draining and cleanup of the
 * given Stream is done asynchronously with respect to the continued execution
 * of the calling process.
 *
 * @param[in] stream  The handle for one of the calling process' Streams.
 * @return  etrtSuccess, etrtErrorInvalidValue, etrtErrorInvalidResourceHandle
 */
EXAPI etrtError_t etrtStreamDestroy(etrtStream_t stream);

/**
 * @brief  Block until all of a Stream's operations have completed.
 *
 * Take the handle for one of the calling process's Streams and returns only
 * when all of the operations that were issued on it have completed.
 *
 * TODO decide if we want to support the `etDeviceScheduleBlockingSync` option
 * on Devices
 *
 * @param[in] stream  The handle for one of the calling process' Streams.
 * @return  etrtSuccess, etrtErrorInvalidResourceHandle
 */
EXAPI etrtError_t etrtStreamSynchronize(etrtStream_t stream);

/**
 * @brief  Have a Stream wait for an Event before continuing with its execution.
 *
 * Take the handle for a Stream and an Event handle, and issue an operation that
 * will cause the execution of subsequently issued operations on the Stream to
 * block until the given Event is received.
 *
 * @param[in] stream  The handle for one of the calling process' Streams.
 * @param[in] event  The handle for one of the calling process' Events.
 * @return  etrtSuccess, etrtErrorInvalidValue, etrtErrorInvalidResourceHandle
 */
// @todo FIXME
// EXAPI etrtError_t etrtStreamWaitEvent(etrtStream_t stream, etrtEvent_t
// event);
EXAPI etrtError_t etrtStreamWaitEvent(etrtStream_t stream, etrtEvent_t event,
                                      unsigned int flags);

/**
 * @}
 */

/**
 * @addtogroup ETCRT_EVENT_MGMT ET Runtime API Event Management
 * @ingroup  ETCRT_API
 * @{
 *
 * Events are how concurrent operations (within and among Devices) are
 * synchronized. Events are explicitly created and deleted, have work (i.e.,
 * pending operations in a Stream) associated with them, and can be queried to
 * determine if that work has been completed.
 *
 * The workflow for using Events is as follows:
 * - an Event is created
 * - the state of the outstanding operations within a Stream is captured into
 * the Event
 * - the snapshot of pending work captured in an Event can be queried
 * - the client code can be made to wait until all of the work in an Event has
 * completed execution
 *
 * In addition to being used as mechanisms for synchronization, Events are also
 * used to capture the execution times associated with specific operations.
 *
 */

EXAPI etrtError_t etrtEventCreate(etrtEvent_t *event);

/**
 * @brief  Create a new Event.
 *
 * Return a new Event, with a given set of options.
 * The options that can be given include:
 * - `etrtEventDefault`: create a default event
 * - `etrtEventBlockingSync`: create an event that uses blocking
 * synchronization.  This causes calls to ::etrtEventSynchronize() to block
 * until this event completes.
 * - `etrtEventDisableTiming`: disable gathering performance timing data
 *
 * @param[in] event  A pointer to the location where the newly created Event is
 * to be written.
 * @param[in] flags  A bitmap that contains the flags that enable the desired
 * options for the Event to be created.
 * @return  etrtSuccess, etrtErrorInvalidValiue, etrtErrorLaunchFailure,
 * etrtErrorMemoryAllocation
 */
// @todo FIXME
EXAPI etrtError_t etrtEventCreateWithFlags(etrtEvent_t *event,
                                           unsigned int flags);

/**
 * @brief  Return the state of an Event.
 *
 * Take the handle for one of the caller's Events and return a value of
 * `etrtSuccess` if all of the operations associated with the Event have
 * completed.  Otherwise, a value of `etrtErrorNotReady` is returned.
 *
 * If this call is passed an Event that has yet to have an event recorded into
 * it (via a call to
 * ::etrtEventRecord()), it will return `etrtSuccess` and the Event will not be
 * changed.
 *
 * @param[in] event  The handle for an Event created by the calling process.
 * @return  etrtSuccess, etrtErrorNotReady, etrtErrorInvalidValue,
 * etrtErrorInvalidResourceHandle, etrtErrorLaunchFailure
 */
EXAPI etrtError_t etrtEventQuery(etrtEvent_t event);

/**
 * @brief  Capture a snapshot of the state of a given Stream.
 *
 * Capture the state of the given Stream, at the time of this call, into the
 * given Event. The ::etrtEventQuery() call can be used read out the state of
 * the event, and the ::etrtStreamWaitEvent() call can be used to wait for the
 * work that was captured in this event.
 *
 * Both the Event and Stream handles given in this call must be currently owned
 * by the calling process, and both the Event and Stream must be associated with
 * the same Device.
 *
 * This call can be made on the same Event multiple times, and the contents of
 * the Event will be overwritten with each call.
 *
 * @param[in] event  The handle for an Event created by the calling process.
 * @param[in] stream  The handle for one of the calling process' Streams.
 * @return  etrtSuccess, etrtErrorInvalidValue, etrtErrorInvalidResourceHandle,
 * etrtErrorLaunchFailure
 */
EXAPI etrtError_t etrtEventRecord(etrtEvent_t event, etrtStream_t stream);

/**
 * @brief  Wait for all of the work captured in an Event to complete.
 *
 * Takes the handle for an Event and blocks until all of the pending operations
 * recorded in the Event have completed execution.  This will return immediately
 * if the Event is empty.
 *
 * @param[in] event  The handle for an Event created by the calling process.
 * @return  etrtSuccess, etrtErrorInvalidValue, etrtErrorInvalidResourcHandle,
 * etrtErrorLaunchFailure
 */
EXAPI etrtError_t etrtEventSynchronize(etrtEvent_t event);

/**
 * @brief  Returns the time between a starting and ending Event.
 *
 * Takes a pair of Event handles, one that defines a starting Event and the
 * other for an Ending Event and returns the time (in milliseconds) between when
 * all of the work defined in the two Events completes.
 *
 * When using Events from Streams other than the Default Stream, the returned
 * time measurement can be larger than expected.  This is because this operation
 * executes asynchronously with respect to operations in other Streams, and
 * other operations could execute between the start and end Events.
 *
 * An ::etrtRecordEvent() call must have been made to both of the Events given
 * in this call.  If either of the Events has not yet completed when this is
 * called, it returns an `etrtErrorNotReady` indication.  If either Event was
 * created with the `etrtEventDisableTiming` flag, then this will return a
 * `etrtErrorInvalidResourceHandle`.
 *
 * @param[in] ms  A pointer to where the number of milliseconds of execution
 * time between the start and end events is to be written.
 * @param[in] start  The handle for an Event whose completion marks the start of
 * a time interval.
 * @param[in] end  The handle for an Event whose completion marks the end of a
 * time interval.
 * @return  etrtSuccess, etrtErrorNotReady, etrtErrorInvalidValue,
 * etrtErrorInvalidResourceHandle, etrtErrorLaunchFailure
 */
EXAPI etrtError_t etrtEventElapsedTime(float *ms, etrtEvent_t start,
                                       etrtEvent_t end);

/**
 * @brief  Delete an Event.
 *
 * Take the handle for one of the caller's Events and delete it -- even if the
 * Event has not completed. This call returns immediately and the Event is
 * actually deleted when the event completes.
 *
 * @param[in] event  The handle for an Event created by the calling process.
 * @return  etrtSuccess, etrtErrorInvalidValue, etrtErrorLaunchFailure
 */
EXAPI etrtError_t etrtEventDestroy(etrtEvent_t event);

/**
 * @}
 */

/**
 * @addtogroup ETCRT_KERNEL_EXECUTION ET Runtime API Kernel
 * @ingroup  ETCRT_API
 * @{
 *
 * The whole purpose of the ET SoC is to run many instances of Kernels in
 * parallel to accelerate computationally complex applications.
 *
 * The following functions are used to define a Kernel to be executed, the
 * parameters that are to be given to each instance of the Kernel, and a
 * specification of how many Kernel instances to launch.
 *
 * **Kernels**
 *
 * A Kernel can be specified in many ways (e.g., through a variety of DSLs,
 * high-level languages, or assembly code), but must ultimately be turned in a
 * block of RV64I??? (with ET ISA Extensions) machine code (by an AOT or JIT
 * compiler or an assembler).  These blocks of code (i.e., Kernels) are managed
 * (both in files and in memory) in standard ELF format, stripped of symbols,
 * and with a two entry points -- one for each of the two HARTs in a Minion.  In
 * this way, each Kernel instance can have at most two threads executing on a
 * Minion at a time.  It is anticipated that the Primary thread will perform the
 * bulk of the computation, while the Secondary thread coordinates with the
 * Primary thread and performs data pre-fetch operations. However, there are
 * restrictions that limit this to be the only use case supported.
 *
 * N.B. There are constraints on the Minion implementation that make the two
 * threads not totally identical in capability -- e.g., there is only a Register
 * File for the Primary thread in the Vector Processing Unit.
 *
 * @todo  Define and implement a mechanism for specifying Kernel thread-pairs --
 * i.e., need to be able to define the additional code for the second Minion
 * thread/HART.  Assume that both Primary and Secondary threads get the same set
 * of pre-defined stack variables.
 *
 * **Kernel Differentiation**
 *
 * The whole point of the Device is to run concurrently, multiple instances of a
 * Kernel to accelerate a particular computation on some data.  To this end,
 * each Kernel instance is given access to the memory regions where the input
 * data can be read and where the output data can be written.  Given that the
 * code that implements each of the Kernel instances is identical, some means of
 * differentiation is required to ensure that the work/data is properly
 * distributed across all of the Kernels in a given computational step. The
 * performance of the computational step to be accelerated is a function of how
 * efficiently the total work can be distributed among Kernels, and their
 * (partial) results combined to create the complete result. In a ideal world, a
 * parallelizing compiler would determine how to decompose a given computation
 * into discrete units and it would be able to generate Kernels that would make
 * the appropriate memory references to prefetch, load, and operate upon their
 * individual portion of the problem.  In the absence of such a magic compiler,
 * we rely on the programmer to define both the number and the organization of
 * Kernel instances to be executed.
 *
 * For simplicity of exposition here, from this point on, we assume that each
 * Kernel instance will be executing a single thread of control (despite the
 * fact that there will frequently be thread pairs executing concurrently on
 * each Minion).
 *
 * This API has chosen to follow the method of differentiation used by CUDA --
 * namely, a two-level, multidimensional, indexing scheme for defining the
 * number and organization of Kernel instances. This scheme defines the number
 * of Kernel instances and how they are differentiated. The lower level of
 * indexing defines the number of threads to be launched in an affinity group
 * known as a Block, while the higher level of indexing defines the number of
 * blocks that should be executed together in a Grid.
 *
 * The total number of Kernel instances for a given Kernel launch will be the
 * number of threads per Block times the number of Blocks.
 *
 * Threads within a Block are can communicate among themselves, while threads in
 * different blocks do not communicate -- therefore threads in a Block are
 * considered to be more tightly coupled, and Blocks in a Blocks within Grids
 * are considered to be largely independent of each other.
 *
 * Grid and Block values are specified for each Kernel launch call to this API,
 * and the Device execution environment uses these values to ensure that the
 * desired number of Kernel instances are dispatched to the appropriate Minions.
 * The Device execution environment also ensures that each Kernel instance is
 * given its own, unique, set of index values as input parameters in order to
 * differentiate itself. Each Kernel instance is given its unique values in the
 * following stack variables: `gridDim`, `blockDim`, `blockIdx`, and
 * `threadIdx`.  Each of these variables is a three-component vector that can
 * represent a 1D, 2D, or 3D value.  The total number of items defined by each
 * of these variables is the product of all of their dimensions -- i.e., if a
 * Kernel is launched with the values `gridDim`=(4,4,4) and `blockDim`=(8,1,1),
 * then a total of 512 Kernel instances are to be instantiated (organized as a
 * four-by-four cube of eight-element vectors of Kernels), and the first Kernel
 * instance will see: `blockIdx`=(0,0,0) and `threadIdx`=(0,0,0), the next
 * instance will see: `blockIdx`=(0,0,0) and `threadIdx`=(0,0,1), and so on
 * until the last instance that will see: `blockIdx`=(3,3,3) and
 * `threadIdx`=(7,0,0).
 *
 * In addition to these pre-defined values, the Kernel code has access to the ID
 * of the Minion, its Neighborhood, and Shire, which can also used to further
 * differentiate instances.
 *
 * The choice of indexing values is a function of the size of the computation to
 * be performed, the number of processing elements available, and their
 * structure (in terms of both memory and intercommunication). Ideally, you'd
 * like to write code that was independent of the details of the HW accelerator,
 * but that's not really practical, so there are guidelines on how to
 * structure/decompose/distribute problems to work best on a given piece of HW.
 * At a high level, Blocks map well onto Neighborhoods and Grids onto Shires,
 * but unfortunately, it's not that simple and more effort has to be put into
 * decomposing problems to achieve peak performance on the ET SoC.
 *
 * @see(ET SoC Programmer's Guide).
 *
 * **API Kernel Launch Calls**
 *
 * Clients of this API define the number of (and indexing scheme for) instances
 * of each Kernel that is to be executed, the arguments to be passed all Kernel
 * instances, and the (potentially dual-threaded) Kernel to be executed. This
 * API passes the necessary information to the currently attached Device and the
 * Device handles the dispatching of Kernel instances.
 *
 * **Kernel Distribution and Execution**
 *
 * The ::etrtLaunch() call causes the Device runtime environment to set up
 * on-chip memory and dispatch all of the desired Kernel instances.  Once
 * launched, Kernel instances run to completion, and signal the Device execution
 * environment, which in trun is responsible for managing the completion of the
 * execution of the computational step defined by the Kernel in the API call.
 *
 * The Device runtime keeps track of the state of the Minions and knows how many
 * are occupied with Kernel instances, and how many can have instances
 * dispatched upon them. If a Kernel launch call requires fewer threads than
 * there are currently available Minions, the entire set of Kernel instances are
 * dispatched and run concurrently.  In this case, it's expected that all
 * instances should complete their execution in close to the same amount of
 * time. On the other hand, if a launch calls for more instances than there are
 * available Minions, then there are several optional behaviors that can be
 * selected on a per-launch-call basis -- an `etrtErrorLaunchOutOfReources`
 * error could be returned; the call could block until until the required number
 * of Minions are available (reserving all available Minions until sufficient
 * ones are acquired), or as many instances can be dispatched as there are
 * available Minions and the execution of the individual Kernel instances are
 * serialized until all of the required work has been completed.
 *
 * @todo  Document the constraints that can be applied on kernel launches --
 * e.g., Block/Neighborhood or Grid/Shire affinity/anti-fragmentation options,
 * kernel gravity, allocation quantization, etc.
 *
 */

/**
 * @brief  Set up the indexing configuration for Kernel launches.
 *
 * Define the Grid and Block dimensions for subsequent Kernel launches, as well
 * as the maximum amount of shared memory that the Kernel instances can consume.
 *
 * @todo  Document the Kernel launch options selected by the flags argument.
 *
 * @param[in] gridDim  The higher-level (i.e., Grid) index dimensions for
 * Kernels to be launched.
 * @param[in] blockDim  The lower-level (i.e., Block) indexes dimensions Kernels
 * to be launched.
 * @param[in] sharedMem  Maximum number of bytes of shared memory that can be
 * used by the launched Kernel instances.
 * @param[in] flags  A bit vector that selects the options to be applied to the
 * subsequent Kernel launches.
 * @return  etrtSuccess, etrtErrorInvalidConfiguration,
 * etrtErrorLaunchOutOfResources
 */
// @todo FIXme
// EXAPI etrtError_t etrtConfigureCall(dim3 gridDim, dim3 blockDim, size_t
// sharedMem, unsigned int flags);
EXAPI etrtError_t etrtConfigureCall(dim3 gridDim, dim3 blockDim,
                                    size_t sharedMem, etrtStream_t stream);

/**
 * @brief  Define the inputs to be given to all instances of the next Kernel to
 * be launched.
 *
 * Define one or more blocks of data that will be given to each kernel as input
 * arguments.
 *
 * This applies to all subsequent Kernel launches, until this is called again.
 *
 * @param[in] args  Array of one or more pointers to arguments that will be
 * provided as input to all Kernel instances.
 * @param[in] sizes  Array of number of bytes in each of the given arguments
 * block in the `args` array.
 * @return  etrtSuccess, etrtErrorInvalidConfiguration
 */
// @todo FIXME
// EXAPI etrtError_t etrtSetupArgument(const void **args, size_t sizes[]);
EXAPI etrtError_t etrtSetupArgument(const void *arg, size_t size,
                                    size_t offset);

/**
 * @brief  Launch a Kernel and give this execution a name.
 *
 * Take a pointer to a Kernel and its print name, and launch it on the attached
 * Device, using the given Steam and the parameters previously defined via
 * ::etrtConfigureCall() and ::etrtSetupArgument().
 *
 * This copies the given Kernel to the current Device (unless it is already
 * cached there) and enqueue it to be executed on the current Stream, with the
 * parameters given by the most recent calls to ::etrtConfigureCall() and
 * ::etrtSetupArgument().
 *
 * This Kernel will be executed in the order in which it was enqueued on the
 * given Stream (and no ordering guarantees are made for the order of execution
 * of Kernels across different Streams).
 *
 * @param[in] kernel  Pointer to the Kernel to be launched.
 * @param[in] kernelName  Pointer to a null-terminated string that is the name
 * of the Kernel to be launched.
 * @param[in] stream  The Stream upon which the Kernels are to be executed.
 * @return  etrtSuccess, etrtErrorInvalidConfiguration, etrtErrorLaunchFailure,
 * etrtErrorLuanchTimeout, etrtErrorLaunchOutOfResources,
 * etrtErrorInvalidDeviceFunction, etrtErrorSharedObjectInitFailed,
 * etrtErrorInvalidPtx, etrtErrorNoKernelImageForDevice,
 * etrtErrorJitCompilerNotFound
 */
// @todo FIXME
// EXAPI etrtError_t etrtLaunch(const void *kernel, const char *kernelName,
// etrtStream_t stream);
EXAPI etrtError_t etrtLaunch(const void *func, const char *kernel_name);

/**
 * @}
 */

/**
 * @addtogroup ETCRT_PROFILING ET Runtime API Profiling
 * @ingroup  ETCRT_API
 * @{
 *
 * *TBD*
 */

/**
 * @brief  Initialize a profiler.
 *
 * *TBD*
 *
 * @return  etrtSuccess, etrtError
 */
EXAPI etrtError_t etrtProfileInitialize();

/**
 * @brief  Start profiling.
 *
 * *TBD*
 *
 * @return  etrtSuccess, etrtError
 */
EXAPI etrtError_t etrtProfileStart();

/**
 * @brief  Stop profiling.
 *
 * *TBD*
 *
 * @return  etrtSuccess, etrtError
 */
EXAPI etrtError_t etrtProfileStop();

/**
 * @}
 */

typedef struct ETmodule_st *etrtModule_t;

EXAPI etrtError_t etrtModuleLoad(etrtModule_t *module, const void *image,
                                 size_t image_size);
EXAPI etrtError_t etrtModuleUnload(etrtModule_t module);
EXAPI etrtError_t etrtRawLaunch(etrtModule_t module, const char *kernel_name,
                                const void *args, size_t args_size,
                                etrtStream_t stream);

#endif // ETRT_H
