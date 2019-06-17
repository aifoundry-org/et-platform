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
#include "Core/CodeModule.h"
#include "Core/DeviceInformation.h"
#include "Core/DeviceTarget.h"
#include "Core/Kernel.h"
#include "Core/MemoryManager.h"
#include "Support/ErrorOr.h"
#include "Support/STLHelpers.h"

// FIXME the following should be removed
#include "et_event.h"
#include "et_stream.h"

#include <memory>
#include <thread>

// Fixme this class shold be removed.
class EtStream;
class EtEvent;
class GetDev;

namespace et_runtime {

class Stream;
class AbstractMemoryPtr;
class HostMemoryPtr;
class DeviceMemoryPtr;

// Loaded to device kernels ELF binary descriptor.
struct EtLoadedKernelsBin {
  void *devPtr = nullptr; // base device address of loaded binary
  et_runtime::EtActionEvent *actionEvent =
      nullptr; // used for synchronize with load completion
};

class Device {
  friend class ::GetDev;
  friend class et_runtime::device::MemoryManager;

public:
  Device();

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
  //  etrtError memcpy(AbstractMemoryPtr *dst, const AbstractMemoryPtr *src,
  //                   size_t count);

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

  /// @brief Return true if the device is alive and we can execute commands
  bool deviceAlive();

  void deviceExecute();


  /// @brief Set the path the the bootrom and load its contents
  ///
  /// @params[in] path  Path to the bootrom file
  ///
  /// @todo This function "violates" the RIIA principle that all setup
  /// information should be passed throug the constructor. If were to do that
  /// now that would require passing the path in the DeviceManager class as
  /// well. @idoud I would like to revisit and clear this once we have the real
  /// Device-FW in the picture where we should be passsing as a runtime argument
  /// its location.
  bool setBootRom(const std::string &path);

  /// @brief Return reference to the underlying bootrom data
  std::vector<uint8_t> &getBootRom() { return bootrom_data_; }

  /// @brief Return reference to the underlying target specific device
  /// @todo This interface is currently used by the commands and we should find
  /// a way to hide it.
  device::DeviceTarget &getTargetDevice() { return *target_device_; }

  bool isPtrAllocedHost(const void *ptr) {
    return mem_manager_->isPtrAllocedHost(ptr);
  }
  bool isPtrAllocedDev(const void *ptr) {
    return mem_manager_->isPtrAllocedDev(ptr);
  }

  bool isPtrInDevRegion(const void *ptr) {
    return mem_manager_->isPtrInDevRegion(ptr);
  }

  EtStream *defaultStream() const { return defaultStream_; }
  EtStream *getStream(etrtStream_t stream) {
    EtStream *et_stream = reinterpret_cast<EtStream *>(stream);
    if (et_stream == nullptr) {
      return defaultStream_;
    }
    assert(stl_count(stream_storage_, et_stream));
    return et_stream;
  }
  EtEvent *getEvent(etrtEvent_t event) {
    EtEvent *et_event = reinterpret_cast<EtEvent *>(event);
    assert(stl_count(event_storage_, et_event));
    return et_event;
  }
  // FIXME create a module_id stop passing pointers arround
  et_runtime::Module *getModule(et_runtime::Module *module) {
    assert(stl_count(module_storage_, module));
    return module;
  }
  EtStream *createStream(bool is_blocking) {
    EtStream *new_stream = new EtStream(is_blocking);
    stream_storage_.emplace_back(new_stream);
    return new_stream;
  }
  void destroyStream(EtStream *et_stream) {
    assert(stl_count(stream_storage_, et_stream) == 1);
    stl_remove(stream_storage_, et_stream);
  }
  EtEvent *createEvent(bool disable_timing, bool blocking_sync) {
    EtEvent *new_event = new EtEvent(disable_timing, blocking_sync);
    event_storage_.emplace_back(new_event);
    return new_event;
  }
  etrtError_t streamSynchronize(etrtStream_t stream);
  void destroyEvent(EtEvent *et_event) {
    assert(stl_count(event_storage_, et_event) == 1);
    stl_remove(event_storage_, et_event);
  }

  et_runtime::Module *createModule(const std::string &name);

  void destroyModule(et_runtime::Module *et_module);

  void addAction(EtStream *et_stream, et_runtime::EtAction *et_action) {
    // FIXME: all blocking streams can synchronize through EtActionEventWaiter
    if (et_stream->isBlocking()) {
      defaultStream_->addCommand(et_action);
    } else {
      et_stream->addCommand(et_action);
    }
    deviceExecute();
  }

  etrtError_t mallocHost(void **ptr, size_t size);
  etrtError_t freeHost(void *ptr);
  etrtError_t malloc(void **devPtr, size_t size);
  etrtError_t free(void *devPtr);
  etrtError_t pointerGetAttributes(struct etrtPointerAttributes *attributes,
                                   const void *ptr);

  etrtError_t memcpyAsync(void *dst, const void *src, size_t count,
                          enum etrtMemcpyKind kind, etrtStream_t stream);
  etrtError_t memcpy(void *dst, const void *src, size_t count,
                     enum etrtMemcpyKind kind);
  etrtError_t memset(void *devPtr, int value, size_t count);

  void appendLaunchConf(const et_runtime::EtLaunchConf &conf) {
    launch_confs_.push_back(conf);
  }

  etrtError_t setupArgument(const void *arg, size_t size, size_t offset);
  etrtError_t launch(const void *func, const char *kernel_name);
  // FIXME pass module_id
  etrtError_t rawLaunch(et_runtime::Module *module, const char *kernel_name,
                        const void *args, size_t args_size,
                        etrtStream_t stream);

  ErrorOr<et_runtime::Module *> moduleLoad(const std::string &name,
                                           const std::string &path);
  etrtError_t moduleUnload(et_runtime::Module *module);

private:
  void initDeviceThread();
  void uninitDeviceThread();
  void uninitObjects();

  std::vector<uint8_t> bootrom_;
  std::unique_ptr<et_runtime::device::DeviceTarget> target_device_;
  std::unique_ptr<et_runtime::device::MemoryManager> mem_manager_;
  bool device_thread_exit_requested_ = false;
  EtStream *defaultStream_ = nullptr;
  std::vector<std::unique_ptr<EtStream>> stream_storage_;
  std::vector<std::unique_ptr<EtEvent>> event_storage_;
  std::vector<et_runtime::EtLaunchConf> launch_confs_;
  // FIXME SW-257
  std::vector<std::unique_ptr<et_runtime::Module>> module_storage_;
  std::map<const void *, EtLoadedKernelsBin>
      loaded_kernels_bin_; // key is id; there are 2 cases now:
                           // - Esperanto registered ELF (from fat binary)
                           // - dynamically loaded module
  std::string bootrom_path_;                ///< Path to the bootrom file
  std::vector<unsigned char> bootrom_data_; ///< Data of the bootrom file
};
} // namespace et_runtime

#endif // ET_RUNTIME_DEVICE_H
