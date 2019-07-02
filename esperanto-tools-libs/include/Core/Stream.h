//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#ifndef ET_RUNTIME_STREAM_H
#define ET_RUNTIME_STREAM_H

#include "Support/ErrorOr.h"

#include <chrono>
#include <memory>
#include <queue>

namespace et_runtime {

namespace device_api {
class CommandBase;
}
class AbstractMemoryPtr;
class Event;
class Kernel;

///
/// @brief Class holding the information for a Stream
///
///
/// Streams are essentially independent work queues for executing operations in
/// parallel. Operations can be issued on specific Streams, with the guarantee
/// that they execute sequentially (in the order of their issue) within the
/// Stream.  No guarantees are made on the order of execution of operations
/// among different Streams.
///
/// If new Streams are not explicitly created, execution is done on the Default
/// Stream (i.e., Stream 0). A degree of parallelism can be achieved by
/// executing sequences of asynchronous operations on a single Stream.  However,
/// additional degrees of (high-level) parallelism (and programming
/// independence) can be achieved by creating additional Streams and executing
/// sequences of operations on each of them in parallel. Streams help simplify
/// the management of independent activities of complex applications -- e.g.,
/// separate portions of an application executed by multiple (child) threads of
/// the (paraent) calling process. Synchronization and execution-cycle
/// management can be done at the level of Streams, e.g., waiting for all of the
/// operations on a particular Stream to complete, or creating/deleting an
/// entire Stream. Note that this is in addition to the finer-grained type of
/// synchronization that can be accomplished with the Event mechanism.
///
/// This API chooses to have the Default Stream follow the per-thread Stream
/// semantics of CUDA (as opposed to CUDA's Legacy Stream semantics).
class Stream {
public:
  Stream(bool is_blocking) : is_blocking_(is_blocking) { init(); }

  ///
  /// @brief initalize the stream
  void init();

  ~Stream() { assert(actions_.empty()); }

  ///
  /// @brief  Block until all of a Stream's operations have completed.
  ///
  /// Take the handle for one of the calling process's Streams and returns only
  /// when all of the operations that were issued on it have completed.
  ///
  /// TODO decide if we want to support the `etDeviceScheduleBlockingSync`
  /// option on Devices
  ///
  /// @return  etrtSuccess, etrtErrorInvalidResourceHandle
  ////
  etrtError synchronize();

  ///
  /// @brief  Asynchronously copy the contents of one region of allocated memory
  /// to another.
  ///
  /// Initiate the copying of the given number of bytes from the given source
  /// location to the given destination location.  The source and destination
  /// regions can be other either the Host or an attached Device, but both
  /// regions must be currently allocated by the calling process using this API.
  /// This call returns immediately and the transfer can be performed
  /// asynchronously with respect to the the execution of operations on other
  /// Streams.  The caller can synchronize with the completion of the transfer
  /// by using the Stream synchronization functions.
  ///
  /// @param[in] dst  A pointer to the location where the memory is to be
  /// copied.
  /// @param[in] src  A pointer to the location from which the memory is to be
  /// copied.
  /// @param[in] count  The number of bytes that should be copied.
  /// @return  etrtSuccess, etrtErrorInvalidValue,
  /// etrtErrorInvalidMemoryDirection
  ///
  ErrorOr<Event> memcpyAsync(const AbstractMemoryPtr &dst,
                             const AbstractMemoryPtr src, size_t count);

  /// @brief Create an event
  ErrorOr<std::unique_ptr<Event>> createEvent();

  ///
  /// @brief  Create a new Event.
  ///
  /// Return a new Event, with a given set of options.
  /// The options that can be given include:
  /// - `etrtEventDefault`: create a default event
  /// - `etrtEventBlockingSync`: create an event that uses blocking
  /// synchronization.  This causes calls to ::etrtEventSynchronize() to block
  /// until this event completes.
  /// - `etrtEventDisableTiming`: disable gathering performance timing data
  ///
  /// @param[in] flags  A bitmap that contains the flags that enable the desired
  /// options for the Event to be created.
  /// @return  etrtSuccess, etrtErrorInvalidValiue, etrtErrorLaunchFailure,
  /// etrtErrorMemoryAllocation
  ///
  ErrorOr<std::unique_ptr<Event>> createEventWithFlags(unsigned int flags);

  ///
  /// @brief  Capture a snapshot of the state of a given Stream.
  ///
  /// Capture the state of the given Stream, at the time of this call, into the
  /// given Event. The ::etrtEventQuery() call can be used read out the state of
  /// the event, and the ::etrtStreamWaitEvent() call can be used to wait for
  /// the work that was captured in this event.
  ///
  /// Both the Event and Stream handles given in this call must be currently
  /// owned by the calling process, and both the Event and Stream must be
  /// associated with the same Device.
  ///
  /// This call can be made on the same Event multiple times, and the contents
  /// of the Event will be overwritten with each call.
  ///
  /// @param[in] event  The handle for an Event created by the calling process.
  /// @param[in] stream  The handle for one of the calling process' Streams.
  /// @return  etrtSuccess, etrtErrorInvalidValue,
  /// etrtErrorInvalidResourceHandle, etrtErrorLaunchFailure
  ///
  ErrorOr<Event> eventRecord();

  ///
  /// @brief  Have the Stream wait for an Event before continuing with its
  /// execution.
  ///
  /// Take an Event handle, and issue an operation that will cause the execution
  /// of subsequently issued operations on the Stream to block until the given
  /// Event is received.
  ///
  /// @param[in] event  The handle for one of the calling process' Events.
  /// @param[in] flags  \todo add flag description
  /// @return  etrtSuccess, etrtErrorInvalidValue,
  /// etrtErrorInvalidResourceHandle
  ////
  void waitEvent(const Event &event, unsigned int flags);

  ///
  /// @brief  Returns the last error from an API call.
  ///
  /// Returns the last ET Runtime error generated by calls to the API, or value
  /// of etrtSuccess if no error occurred since the last time this was called.
  ///
  /// TODO define how this works with respect to other client processes --
  /// probably  caller-specific
  ///
  /// @return  etrtSuccess, etrtErrorMissingConfiguration,
  /// etrtErrorMemoryAllocation, etrtErrorInitializationError,
  /// etrtErrorLaunchFailure, etrtErrorLaunchTimeout,
  /// etrtErrorLaunchOutOfResources, etrtErrorInvalidDeviceFunction,
  /// etrtErrorInvalidConfiguration, etrtErrorInvalidDevice,
  /// etrtErrorInvalidValue,  etrtErrorInvalidPitchValue,
  /// etrtErrorInvalidSymbol, etrtErrorUnmapBufferObjectFailed,
  /// etrtErrorInvalidDevicePointer, etrtErrorInvalidTexture,
  /// etrtErrorInvalidTextureBinding, etrtErrorInvalidChannelDescriptor,
  /// etrtErrorInvalidMemcpyDirection, etrtErrorInvalidFilterSetting,
  /// etrtErrorInvalidNormSetting, etrtErrorUnknown,
  /// etrtErrorInvalidResourceHandle, etrtErrorInsufficientDriver,
  /// etrtErrorNoDevice, etrtErrorSetOnActiveProcess, etrtErrorStartupFailure,
  /// etrtErrorInvalidPtx, etrtErrorNoKernelImageForDevice,
  /// etrtErrorJitCompilerNotFound
  ////
  etrtError lastError();

  /// @brief Return the elasped time in miliseconds between 2 events.
  ErrorOr<std::chrono::milliseconds> elaspedTime(const Event &start,
                                                 const Event &end);

  /// @brief Create a kernel that is going to be lauched as part of the
  ErrorOr<Kernel> createKernel();

  bool isBlocking() { return is_blocking_; }
  /// @brief Add a command to execute in the command Queue.
  void addCommand(std::shared_ptr<et_runtime::device_api::CommandBase> action);
  /// @brief Return True iff the command queue is empty
  bool noCommands() const { return actions_.empty(); }
  /// @brief Return pointer to the command in the front of the queue
  std::shared_ptr<et_runtime::device_api::CommandBase> frontCommand() {
    return actions_.front();
  }
  /// @brief Remove front command
  void popCommand() { actions_.pop(); }

private:
  bool is_blocking_;
  std::queue<std::shared_ptr<et_runtime::device_api::CommandBase>> actions_;
};
} // namespace et_runtime

#endif // ET_RUNTIME_STREAM_H
