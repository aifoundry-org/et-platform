#ifndef ETTEE_ET_DEVICE_H
#define ETTEE_ET_DEVICE_H

#include <stddef.h>
#include <memory>
#include <map>
#include <vector>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "et_stream.h"
#include "et_event.h"
#include "etrt.h"


// Region of host or device memory region.
struct EtMemRegion
{
    void *region_base;
    size_t region_size;
    std::map<const void *, size_t> alloced_ptrs; // alloced ptr -> size of alloced area

    EtMemRegion(void *ptr, size_t size) : region_base(ptr), region_size(size) {}

    static constexpr size_t kAlign = 1 << 20;  // 1M
    bool isPtrAlloced(const void *ptr);
    void *alloc(size_t size);
    void free(void *ptr);
    void print();
    bool isPtrInRegion(const void *ptr) {
        return ptr >= region_base && (uintptr_t)ptr < (uintptr_t)region_base + region_size;
    }
};

// Launch Configuration.
struct EtLaunchConf
{
    dim3 gridDim;
    dim3 blockDim;
    EtStream *etStream = nullptr;
    std::vector<uint8_t> args_buff;
};

// Dynamically loaded module descriptor.
struct EtModule
{
    std::unordered_map<std::string, size_t> kernel_offset; // kernel name -> kernel entry point offset
    std::unordered_map<std::string, size_t> raw_kernel_offset; // raw-kernel name -> kernel entry point offset
};

// Loaded to device kernels ELF binary descriptor.
struct EtLoadedKernelsBin
{
    void *devPtr = nullptr;               // base device address of loaded binary
    EtActionEvent *actionEvent = nullptr; // used for synchronize with load completion
};

/*
 * Represents one Esperanto device.
 */
class EtDevice
{
public:
    EtDevice() {
        initDeviceThread();
        initMemRegions();
    }

    void initDeviceThread();
    void uninitDeviceThread();
    void initMemRegions();
    void uninitMemRegions();
    void uninitObjects();

    virtual ~EtDevice() {
        // Must stop device thread first in case it have non-empty streams
        uninitDeviceThread();
        uninitObjects();
        uninitMemRegions();
    }

    void deviceThread();

    bool device_thread_exit_requested = false;
    std::thread device_thread;
    std::mutex mutex;
    std::condition_variable cond_var; // used to inform deviceThread about new requests

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

    static constexpr size_t kHostMemRegionSize = (1 << 20) * 256;

    std::unique_ptr<EtMemRegion> host_mem_region;
    std::unique_ptr<EtMemRegion> dev_mem_region;
    std::unique_ptr<EtMemRegion> kernels_dev_mem_region;


    EtStream *defaultStream = nullptr;
    std::vector<std::unique_ptr<EtStream>> stream_storage;
    std::vector<std::unique_ptr<EtEvent>> event_storage;
    std::vector<EtLaunchConf> launch_confs;
    std::vector<std::unique_ptr<EtModule>> module_storage;
    std::map<const void *, EtLoadedKernelsBin> loaded_kernels_bin; // key is id; there are 2 cases now:
                                                                   // - Esperanto registered ELF (from fat binary)
                                                                   // - dynamically loaded module

    bool isPtrAllocedHost(const void *ptr)  { return host_mem_region->isPtrAlloced( ptr); }
    bool isPtrAllocedDev(const void *ptr) { return dev_mem_region->isPtrAlloced( ptr); }

    bool isPtrInDevRegion(const void *ptr) { return dev_mem_region->isPtrInRegion(ptr); }

    EtStream *getStream(etrtStream_t stream) {
        EtStream *et_stream = reinterpret_cast<EtStream *>(stream);
        if ( et_stream == nullptr )
        {
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
        if ( et_stream->isBlocking() )
        {
            defaultStream->actions.push(et_action);
        } else
        {
            et_stream->actions.push(et_action);
        }
        notifyDeviceThread();
    }
};

#include <memory>

extern EtDevice* my_et_device;
/*
 * Helper class to get device object and lock it in RAII manner.
 */
class GetDev
{
public:
    GetDev() : dev(getEtDevice()) {
        dev->mutex.lock();
    }

    ~GetDev() {
        dev->mutex.unlock();
    }

    void destroy() {
        dev->mutex.unlock();
        delete my_et_device;
        my_et_device = NULL;
    }

    EtDevice *operator->() {
        return dev;
    }

private:
    EtDevice* getEtDevice()
    {
        if(my_et_device == NULL) {
            my_et_device = new EtDevice();
        }
        return my_et_device;
    }

    EtDevice *dev;
};


#endif  // ETTEE_ET_DEVICE_H

