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

#include "Common/CommonTypes.h"
#include "Core/DeviceInformation.h"
#include "Core/DeviceTarget.h"
#include "Core/Kernel.h"
#include "Core/MemoryManager.h"
#include "Support/ErrorOr.h"
#include "Support/STLHelpers.h"

#include <memory>
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

// Loaded to device kernels ELF binary descriptor.
struct EtLoadedKernelsBin {
  void *devPtr = nullptr; // base device address of loaded binary
  et_runtime::EtActionEvent *actionEvent =
      nullptr; // used for synchronize with load completion
};

class Device {
  friend class ::GetDev;
  friend class et_runtime::device::MemoryManager;
  friend class Module;
  // Friend class to enable unit-testing
  friend class ::DeviceFWTest;

public:
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

  /// @brief Send the commands to the device
  void deviceExecute();

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

  /// @brief Return the default stream
  Stream *defaultStream() const;

  /// @brief Return the stream with type StreamID
  /// FIXME SW-1294
  Stream *getStream(Stream *stream);

  /// @Brief create a new Stream
  Stream *createStream(bool is_blocking);

  /// FIXME SW-1291 remove the following function
  etrtError streamSynchronize(Stream *stream);

  /// FIXME SW-1294
  void destroyStream(Stream *et_stream);

  /// FIXME SW-1291 this function should move the Stream Class
  Event *createEvent(bool disable_timing, bool blocking_sync);

  /// FIXME SW-1291 this function should move the Stream Class
  void destroyEvent(Event *et_event);

  /// FIXME SW-1291 this function should move the Stream Class
  Event *getEvent(Event *event);

  /// FIXME SW-1291 this function should move the Stream Class
  void
  addCommand(Stream *et_stream,
             std::shared_ptr<et_runtime::device_api::CommandBase> et_action);

  /// FIXME SW-1291 this function should move the Function Launch Class
  void appendLaunchConf(const et_runtime::EtLaunchConf &conf) {
    launch_confs_.push_back(conf);
  }

  /// FIXME SW-1291 this function should move the Function Launch Class
  etrtError setupArgument(const void *arg, size_t size, size_t offset);

  /// FIXME SW-1291 this function should move the Function Launch Class
  // FIXME pass module_id
  etrtError rawLaunch(et_runtime::ModuleID module_id, const char *kernel_name,
                      const void *args, size_t args_size, Stream *stream);

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

  et_runtime::Module *getModule(et_runtime::ModuleID mid);

  ErrorOr<et_runtime::ModuleID> moduleLoad(const std::string &name,
                                           const std::string &path);
  etrtError moduleUnload(et_runtime::ModuleID mid);

private:
  void initDeviceThread();
  void uninitDeviceThread();
  void uninitObjects();

  et_runtime::Module &createModule(const std::string &name);
  void destroyModule(et_runtime::ModuleID modue);

  int device_index_;
  std::unique_ptr<et_runtime::device::DeviceTarget> target_device_;
  std::unique_ptr<et_runtime::FWManager> fw_manager_;
  std::unique_ptr<et_runtime::device::MemoryManager> mem_manager_;
  std::unique_ptr<et_runtime::ModuleManager> module_manager_;
  bool device_thread_exit_requested_ = false;
  Stream *defaultStream_ = nullptr;
  std::vector<std::unique_ptr<Stream>> stream_storage_;
  std::vector<std::unique_ptr<Event>> event_storage_;
  std::vector<et_runtime::EtLaunchConf> launch_confs_;
  // FIXME: remove the following
  std::map<const void *, EtLoadedKernelsBin>
      loaded_kernels_bin_; // key is id; there are 2 cases now:
                           // - Esperanto registered ELF (from fat binary)
                           // - dynamically loaded module
};
} // namespace et_runtime

#endif // ET_RUNTIME_DEVICE_H
