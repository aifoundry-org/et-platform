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

#include "Core/CodeModule.h"
#include "Core/DeviceInformation.h"
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
  Device()
      : mem_manager_(std::unique_ptr<et_runtime::device::MemoryManager>(
            new et_runtime::device::MemoryManager(*this))) {
    initDeviceThread();
  }

  virtual ~Device() {
    // Must stop device thread first in case it have non-empty streams
    uninitDeviceThread();
    uninitObjects();
  }

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

  void deviceThread();
  bool isLocked() {
    if (mutex_.try_lock()) {
      mutex_.unlock();
      return false;
    } else {
      return true;
    }
  }

  void notifyDeviceThread() {
    assert(isLocked());
    cond_var_.notify_one();
  }

  bool isPtrAllocedHost(const void *ptr) {
    return mem_manager_->isPtrAllocedHost(ptr);
  }
  bool isPtrAllocedDev(const void *ptr) {
    return mem_manager_->isPtrAllocedDev(ptr);
  }

  bool isPtrInDevRegion(const void *ptr) {
    return mem_manager_->isPtrInDevRegion(ptr);
  }

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
  void destroyEvent(EtEvent *et_event) {
    assert(stl_count(event_storage_, et_event) == 1);
    stl_remove(event_storage_, et_event);
  }
  et_runtime::Module *createModule() {
    auto new_module = new et_runtime::Module();
    module_storage_.emplace_back(new_module);
    return new_module;
  }
  void destroyModule(et_runtime::Module *et_module) {
    assert(stl_count(module_storage_, et_module) == 1);
    stl_remove(module_storage_, et_module);
  }

  void addAction(EtStream *et_stream, et_runtime::EtAction *et_action) {
    // FIXME: all blocking streams can synchronize through EtActionEventWaiter
    if (et_stream->isBlocking()) {
      defaultStream_->addCommand(et_action);
    } else {
      et_stream->addCommand(et_action);
    }
    notifyDeviceThread();
  }

  etrtError_t mallocHost(void **ptr, size_t size);
  etrtError_t freeHost(void *ptr);
  etrtError_t malloc(void **devPtr, size_t size);
  etrtError_t free(void *devPtr);
  etrtError_t pointerGetAttributes(struct etrtPointerAttributes *attributes,
                                   const void *ptr);

  void appendLaunchConf(const et_runtime::EtLaunchConf &conf) {
    launch_confs_.push_back(conf);
  }

  etrtError_t setupArgument(const void *arg, size_t size, size_t offset);
  etrtError_t launch(const void *func, const char *kernel_name);
  // FIXME pass module_id
  etrtError_t rawLaunch(et_runtime::Module *module, const char *kernel_name,
                        const void *args, size_t args_size,
                        etrtStream_t stream);
  // FIXME pass module_id
  etrtError_t moduleLoad(et_runtime::Module *module, const void *image,
                         size_t image_size);
  etrtError_t moduleUnload(et_runtime::Module *module);

private:
  void initDeviceThread();
  void uninitDeviceThread();
  void uninitObjects();

  std::unique_ptr<et_runtime::device::MemoryManager> mem_manager_;
  bool device_thread_exit_requested_ = false;
  std::thread device_thread_;
  std::mutex mutex_;
  std::condition_variable
      cond_var_; // used to inform deviceThread about new requests
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
};
} // namespace et_runtime

/*
 * Helper class to get device object and lock it in RAII manner.
 *
 * FIXME The following should move inside the device-manager where we should be
 * returning the actively used device as set by the C api
 */
class GetDev {
public:
  GetDev() : dev(getEtDevice()) { dev.mutex_.lock(); }

  ~GetDev() { dev.mutex_.unlock(); }

  et_runtime::Device *operator->() { return &dev; }

private:
  et_runtime::Device &getEtDevice() {
    static et_runtime::Device et_device_;
    return et_device_;
  }

  et_runtime::Device &dev;
};

#endif // ET_RUNTIME_DEVICE_H
