#ifndef ETTEE_ET_DEVICE_H
#define ETTEE_ET_DEVICE_H

#include "C-API/etrt.h"
// FIXME remove this header and use forward declarations
#include "Core/Commands.h"
#include "Core/MemoryManager.h"
#include "Core/CodeModule.h"
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

namespace et_runtime {
class EtActionEvent;
class EtActionWrite;
} // namespace et_runtime

// Launch Configuration.
struct EtLaunchConf {
  dim3 gridDim;
  dim3 blockDim;
  EtStream *etStream = nullptr;
  std::vector<uint8_t> args_buff;
};

// Loaded to device kernels ELF binary descriptor.
struct EtLoadedKernelsBin {
  void *devPtr = nullptr; // base device address of loaded binary
  et_runtime::EtActionEvent *actionEvent =
      nullptr; // used for synchronize with load completion
};

class GetDev;

/*
 * Represents one Esperanto device.
 */
class EtDevice {
  friend class GetDev;
  friend class et_runtime::device::MemoryManager;

public:
  EtDevice()
      : mem_manager_(std::unique_ptr<et_runtime::device::MemoryManager>(
            new et_runtime::device::MemoryManager(*this))) {
    initDeviceThread();
  }

  virtual ~EtDevice() {
    // Must stop device thread first in case it have non-empty streams
    uninitDeviceThread();
    uninitObjects();
  }

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
  et_runtime::Module *getModule(et_runtime::Module* module) {
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

  void appendLaunchConf(const EtLaunchConf &conf) {
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
  std::vector<EtLaunchConf> launch_confs_;
  // FIXME SW-257
  std::vector<std::unique_ptr<et_runtime::Module>> module_storage_;
  std::map<const void *, EtLoadedKernelsBin>
      loaded_kernels_bin_; // key is id; there are 2 cases now:
                           // - Esperanto registered ELF (from fat binary)
                           // - dynamically loaded module
};

/*
 * Helper class to get device object and lock it in RAII manner.
 */
class GetDev {
public:
  GetDev() : dev(getEtDevice()) { dev.mutex_.lock(); }

  ~GetDev() { dev.mutex_.unlock(); }

  EtDevice *operator->() { return &dev; }

private:
  EtDevice &getEtDevice() {
    static EtDevice et_device_;
    return et_device_;
  }

  EtDevice &dev;
};

#endif // ETTEE_ET_DEVICE_H
