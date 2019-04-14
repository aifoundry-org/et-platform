#ifndef ETTEE_ET_DEVICE_H
#define ETTEE_ET_DEVICE_H

#include "C-API/etrt.h"
#include "Core/MemoryManager.h"
#include "et_event.h"
#include "et_stream.h"
#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <stddef.h>
#include <thread>
#include <unordered_map>
#include <vector>


// Launch Configuration.
struct EtLaunchConf {
  dim3 gridDim;
  dim3 blockDim;
  EtStream *etStream = nullptr;
  std::vector<uint8_t> args_buff;
};

// Dynamically loaded module descriptor.
struct EtModule {
  std::unordered_map<std::string, size_t>
      kernel_offset; // kernel name -> kernel entry point offset
  std::unordered_map<std::string, size_t>
      raw_kernel_offset; // raw-kernel name -> kernel entry point offset
};

// Loaded to device kernels ELF binary descriptor.
struct EtLoadedKernelsBin {
  void *devPtr = nullptr; // base device address of loaded binary
  EtActionEvent *actionEvent =
      nullptr; // used for synchronize with load completion
};

/*
 * Represents one Esperanto device.
 */
class EtDevice {
public:
  EtDevice()
      : mem_manager_(std::unique_ptr<et_runtime::device::MemoryManager>(
            new et_runtime::device::MemoryManager(*this))) {
    initDeviceThread();
  }

  void initDeviceThread();
  void uninitDeviceThread();
  void uninitObjects();

  virtual ~EtDevice() {
    // Must stop device thread first in case it have non-empty streams
    uninitDeviceThread();
    uninitObjects();
  }

  void deviceThread();

  std::unique_ptr<et_runtime::device::MemoryManager> mem_manager_;
  bool device_thread_exit_requested = false;
  std::thread device_thread;
  std::mutex mutex;
  std::condition_variable
      cond_var; // used to inform deviceThread about new requests

  bool isLocked() {
    if (mutex.try_lock()) {
      mutex.unlock();
      return false;
    } else {
      return true;
    }
  }

  void notifyDeviceThread() {
    assert(isLocked());
    cond_var.notify_one();
  }



  EtStream *defaultStream = nullptr;
  std::vector<std::unique_ptr<EtStream>> stream_storage;
  std::vector<std::unique_ptr<EtEvent>> event_storage;
  std::vector<EtLaunchConf> launch_confs;
  std::vector<std::unique_ptr<EtModule>> module_storage;
  std::map<const void *, EtLoadedKernelsBin>
      loaded_kernels_bin; // key is id; there are 2 cases now:
                          // - Esperanto registered ELF (from fat binary)
                          // - dynamically loaded module

  bool isPtrAllocedHost(const void *ptr) {
    return mem_manager_->host_mem_region->isPtrAlloced(ptr);
  }
  bool isPtrAllocedDev(const void *ptr) {
    return mem_manager_->dev_mem_region->isPtrAlloced(ptr);
  }

  bool isPtrInDevRegion(const void *ptr) {
    return mem_manager_->dev_mem_region->isPtrInRegion(ptr);
  }

  EtStream *getStream(etrtStream_t stream) {
    EtStream *et_stream = reinterpret_cast<EtStream *>(stream);
    if (et_stream == nullptr) {
      return defaultStream;
    }
    assert(stl_count(stream_storage, et_stream));
    return et_stream;
  }
  EtEvent *getEvent(etrtEvent_t event) {
    EtEvent *et_event = reinterpret_cast<EtEvent *>(event);
    assert(stl_count(event_storage, et_event));
    return et_event;
  }
  EtModule *getModule(etrtModule_t module) {
    EtModule *et_module = reinterpret_cast<EtModule *>(module);
    assert(stl_count(module_storage, et_module));
    return et_module;
  }
  EtStream *createStream(bool is_blocking) {
    EtStream *new_stream = new EtStream(is_blocking);
    stream_storage.emplace_back(new_stream);
    return new_stream;
  }
  void destroyStream(EtStream *et_stream) {
    assert(stl_count(stream_storage, et_stream) == 1);
    stl_remove(stream_storage, et_stream);
  }
  EtEvent *createEvent(bool disable_timing, bool blocking_sync) {
    EtEvent *new_event = new EtEvent(disable_timing, blocking_sync);
    event_storage.emplace_back(new_event);
    return new_event;
  }
  void destroyEvent(EtEvent *et_event) {
    assert(stl_count(event_storage, et_event) == 1);
    stl_remove(event_storage, et_event);
  }
  EtModule *createModule() {
    EtModule *new_module = new EtModule();
    module_storage.emplace_back(new_module);
    return new_module;
  }
  void destroyModule(EtModule *et_module) {
    assert(stl_count(module_storage, et_module) == 1);
    stl_remove(module_storage, et_module);
  }

  void addAction(EtStream *et_stream, EtAction *et_action) {
    // FIXME: all blocking streams can synchronize through EtActionEventWaiter
    if (et_stream->isBlocking()) {
      defaultStream->actions.push(et_action);
    } else {
      et_stream->actions.push(et_action);
    }
    notifyDeviceThread();
  }

  etrtError_t mallocHost(void **ptr, size_t size);
  etrtError_t freeHost(void *ptr);
  etrtError_t malloc(void **devPtr, size_t size);
  etrtError_t free(void *devPtr);
  etrtError_t pointerGetAttributes(struct etrtPointerAttributes *attributes,
                                   const void *ptr);
};

/*
 * Helper class to get device object and lock it in RAII manner.
 */
class GetDev {
public:
  GetDev() : dev(getEtDevice()) { dev.mutex.lock(); }

  ~GetDev() { dev.mutex.unlock(); }

  EtDevice *operator->() { return &dev; }

private:
  EtDevice &getEtDevice() {
    static EtDevice et_device;
    return et_device;
  }

  EtDevice &dev;
};

#endif // ETTEE_ET_DEVICE_H
