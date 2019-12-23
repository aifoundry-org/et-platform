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

#include "esperanto/runtime/CodeManagement/Kernel.h"
#include "esperanto/runtime/Common/CommonTypes.h"
#include "esperanto/runtime/Core/DeviceInformation.h"
#include "esperanto/runtime/Core/DeviceTarget.h"
#include "esperanto/runtime/Core/MemoryManager.h"
#include "esperanto/runtime/DeviceAPI/Command.h"
#include "esperanto/runtime/Support/ErrorOr.h"
#include "esperanto/runtime/Support/STLHelpers.h"
#include "esperanto/runtime/etrt-bin.h"

#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>

class GetDev;
class DeviceFWTest;

namespace et_runtime {

class Event;
class EtAction;
class EtActionEvent;
class Firmware;
class FWManager;
class ModuleManager;
class Module;
class Stream;

class Device {
  friend class ::GetDev;
  friend class et_runtime::device::MemoryManager;
  friend class Module;
  friend class Stream;
  // Friend class to enable unit-testing
  friend class ::DeviceFWTest;

public:
  /// CallBack function type that a Command can register with the Device.
  using MBReadCallBack = std::function<bool(const std::vector<uint8_t> &)>;

  Device(int index);

  virtual ~Device();

  ///
  /// @brief Initialize the underlying target device
  ///
  /// Interact and initalize the target device. This is not done as part of the
  /// contructor because it is an operation that can return errors and currently
  /// we want to avoid having to throw exceptions as part of the constructor.
  /// In the future this function could be deprecated
  ///
  /// @return Return etrtSuccess or any other possbile error.
  etrtError init();

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
  etrtError resetDevice();

  /// @brief Return true if the device is alive and we can execute commands
  bool deviceAlive();

  /// @brief load the DeviceFW on the target device
  etrtError loadFirmwareOnDevice();

  ///
  /// @brief Set the path to the firmware files and load their contents
  ///
  /// @params[in] path Vector of path to the firmware files
  ///
  /// @todo This function "violates" the RIIA principle that all setup
  /// information should be passed throug the constructor. If were to do that
  /// now that would require passing the path in the DeviceManager class as
  /// well. @idoud I would like to revisit and clear this once we have the real
  /// Device-FW in the picture where we should be passsing as a runtime argument
  /// its location.
  bool setFWFilePaths(const std::vector<std::string> &paths);

  ///
  /// @brief Return reference to the underlying target specific device
  /// @todo This interface is currently used by the commands and we should find
  /// a way to hide it.
  device::DeviceTarget &getTargetDevice() { return *target_device_; }

  ///
  /// @brief Return reference to the memory manager of the device.
  ///
  /// Through this referene the user should be able to allocate memory on the
  /// device
  ///
  device::MemoryManager& mem_manager() { return *mem_manager_; }

  ///
  /// @brief  Create a new Stream.
  ///
  /// Return the handle for a newly created Stream for the caller's process.
  ///
  /// @param[in] blocking: True if this is a blocking thread
  ///
  /// @return  ErrorOr (etrtError: etrtErrorInvalidValue) or reference to @ref
  /// Stream object
  ///
  ErrorOr<Stream &> streamCreate(bool blocking);

  /// @brief Return the default stream
  Stream &defaultStream() const;

  /// @brief destroy a Stream that belongs to this device
  /// @param[in] id : Id of the stream to remove
  /// @returns etrtSuccess on success or the appropriate error
  etrtError destroyStream(StreamID ID);

  /// @brief Register a mailbox read callback
  ///
  /// This function registers a callback that updates the command with the
  /// received mailbox reponse from the Master Minion.
  /// @param[in] cmd: Pointer to the command to register the callback with
  /// @param[in] cb: Callback to be executed noce a @ref Response for this @ref
  /// Command has been received.
  bool registerMBReadCallBack(et_runtime::device_api::CommandBase *cmd,
                              const MBReadCallBack &cb);

  /// FIXME SW-1493 this function should move the Stream Class
  Event *createEvent(bool disable_timing, bool blocking_sync);

  /// FIXME SW-1493 this function should move the Stream Class
  void destroyEvent(Event *et_event);

  /// FIXME SW-1493 this function should move the Stream Class
  Event *getEvent(Event *event);

  /// Memory initialization and manipulation on the device
  /// FIXME SW-1293
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
  //  etrtError memcpy(AbstractMemoryPtr *dst, const AbstractMemoryPtr *src,
  //                   size_t count);

  /// FIXME SW-1293
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

  /// FIXME SW-1293
  etrtError memcpyAsync(void *dst, const void *src, size_t count,
                        enum etrtMemcpyKind kind, Stream *stream);

  /// FIXME SW-1293
  etrtError memcpy(void *dst, const void *src, size_t count,
                   enum etrtMemcpyKind kind);

  /// FIXME SW-1293
  etrtError memset(void *devPtr, int value, size_t count);

  /// @brief Unload a module from the device
  ///
  /// @param[in] ID of the code module to unload
  ///
  /// @return Success or error of the operation
  etrtError moduleUnload(et_runtime::CodeModuleID mid);

private:
  void initDeviceThread();
  void uninitDeviceThread();
  void uninitObjects();

  /// @brief add a command to the device's command qeuue.
  ///
  /// This function is private and it is meant to be called only by
  /// friend-classes
  /// @param[in] cmd : Reference to a shared pointer to the `ref Command
  /// @returns True if operation was succesfull.
  bool addCommand(std::shared_ptr<et_runtime::device_api::CommandBase> &cmd);

  /// @brief Send the commands to the device
  void deviceExecute();

  /// @brief Read and handle MB messages from the Device. This function is
  /// running in a separate thread that reads and blocks on the Device waiting
  /// for responses back form the Device
  void deviceListener();

  int device_index_; ///< Index of the device in the system: e.g. it is device
                     ///< #1 as enumerated by the OS.
  std::unique_ptr<et_runtime::device::DeviceTarget>
      target_device_; ///< Pointer to the device implementation class
  std::unique_ptr<et_runtime::FWManager>
      fw_manager_; ///< Pointer to the firwmware manager class that is
                   ///< responsible for managing and loading the FW on the
                   ///< device
  std::unique_ptr<et_runtime::device::MemoryManager>
      mem_manager_; ///< Memory manager of the device
  bool device_thread_exit_requested_ =
      false; ///< This is true if we have requested to terminate the simulation
  Stream *defaultStream_ =
      nullptr; ///< Pointer to the default @ref et_runtime::Stream of the device
  std::vector<Stream *>
      stream_storage_; ///< vector of @ref et_runtime::Stream pointers, of the
                       ///< ones registered/created by this device.
  std::vector<std::unique_ptr<Event>>
      event_storage_; ///< Vector of pending, non handled events from the Device

  /// Command queue and related variables
  /// The Device has a queue of commands that that it needs to execute in the
  /// CommandExecution thread. Each stream, the moment it creates a new command,
  /// it also enqueues it inside the device command queue. The commands are
  /// executed serially across streams, and in the order they are inserted in
  /// the bellow queue
  std::queue<std::shared_ptr<et_runtime::device_api::CommandBase>>
      cmd_queue_;                    ///< Queue of commands
  std::mutex cmd_queue_mutex_;       ///< Mutex that protects the command qeuue
  std::condition_variable queue_cv_; ///< Conditional variable that blocks until
                                     ///< the queue has a command to execute

  /// Mailbox Response callback tracking. Each command registers a callback that
  /// gets executed once a Response for the respective command has been
  /// received.
  using CallBackMap =
      std::map<CommandID, std::tuple<et_runtime::device_api::CommandBase *,
                                     MBReadCallBack>>;

  CallBackMap cb_map_ =
      {}; ///< Map that keeps track of the registered DeviceAPI commands
          ///< that expect a @ref Response back.
  /// Threads that the Device maintains
  std::thread command_executor_; ///< Thread responsible for executing the
                                 ///< enqueued @ref BaseCommand.

  std::atomic_bool
      terminate_cmd_executor_; ///< Set to true when we decide to terminate the thread

  std::thread device_reader_; ///< Thread responsible for reading the mailbox
                              ///< for responses.
};
} // namespace et_runtime

#endif // ET_RUNTIME_DEVICE_H
