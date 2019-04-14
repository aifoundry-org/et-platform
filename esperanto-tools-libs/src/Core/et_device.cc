#include "et_device.h"
#include "Core/MemoryManager.h"
#include "utils.h"
#include <assert.h>
#include <memory.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

#define INCLUDE_FOR_HOST
#include "../kernels/sys_inc.h"
#undef INCLUDE_FOR_HOST

using namespace et_runtime::device;

void EtDevice::deviceThread() {
  // fprintf(stderr, "Hello from EtDevice::deviceThread()\n");

  CardProxy card_proxy_s;
  CardProxy *card_proxy = &card_proxy_s;
  const char *mode = getenv("ETT_MODE");
  mode = mode ? mode : "";
  if (!strcmp(mode, "0") || !strcmp(mode, "local")) {
    card_proxy = nullptr; // i.e. local mode
  } else if (!strcmp(mode, "dev") || !strcmp(mode, "device")) {
    cpOpen(card_proxy, true); // i.e. use real device
  } else if (!strcmp(mode, "card-emu")) {
    cpOpen(card_proxy, false); // i.e. connect to card-emu
  } else {
    cpOpen(card_proxy, false); // by default connect to card-emu for now
  }

  while (true) {
    std::unique_lock<std::mutex> lk(mutex);

    while (true) {
      if (device_thread_exit_requested) {
        if (card_proxy) {
          cpClose(card_proxy);
        }
        return;
      }

      EtAction *actionToExecute = nullptr;
      for (auto &it : stream_storage) {
        EtStream *stream = it.get();
        if (!stream->actions.empty()) {
          EtAction *action = stream->actions.front();
          if (action->readyForExecution()) {
            actionToExecute = action;
            stream->actions.pop();
            break;
          }
        }
      }

      // if there is no action then we are going to wait on condition variable
      if (actionToExecute == nullptr) {
        break;
      }

      // execute action without mutex
      lk.unlock();
      actionToExecute->execute(card_proxy);
      EtAction::decRefCounter(actionToExecute);
      lk.lock();
    }

    cond_var.wait(lk);
  }
}

/**
 * Reset internal objects if user code has not destroyed them
 */
void EtDevice::uninitObjects() {
  for (auto &it : stream_storage) {
    EtStream *stream = it.get();
    while (!stream->actions.empty()) {
      EtAction *act = stream->actions.front();
      stream->actions.pop();
      EtAction::decRefCounter(act);
    }
  }
  for (auto &it : event_storage) {
    EtEvent *event = it.get();
    event->resetAction();
  }
}

void EtDevice::initDeviceThread() {
  std::thread th(&EtDevice::deviceThread, this); // starting new thread
  device_thread.swap(th); // move thread handler to class field
  assert(!device_thread_exit_requested);
}

void EtDevice::uninitDeviceThread() {
  assert(!isLocked());
  {
    std::lock_guard<std::mutex> lk(mutex);
    device_thread_exit_requested = true;
  }
  cond_var.notify_one();
  device_thread.join();
}

etrtError_t EtDevice::mallocHost(void **ptr, size_t size) {
  *ptr = mem_manager_->host_mem_region->alloc(size);
  return etrtSuccess;
}

etrtError_t EtDevice::freeHost(void *ptr) {
  mem_manager_->host_mem_region->free(ptr);
  return etrtSuccess;
}

etrtError_t EtDevice::malloc(void **devPtr, size_t size) {
  *devPtr = mem_manager_->dev_mem_region->alloc(size);
  return etrtSuccess;
}

etrtError_t EtDevice::free(void *devPtr) {
  mem_manager_->dev_mem_region->free(devPtr);
  return etrtSuccess;
}

etrtError_t
EtDevice::pointerGetAttributes(struct etrtPointerAttributes *attributes,
                               const void *ptr) {
  void *p = (void *)ptr;
  attributes->device = 0;
  attributes->isManaged = false;
  if (this->isPtrAllocedHost(p)) {
    attributes->memoryType = etrtMemoryTypeHost;
    attributes->devicePointer = nullptr;
    attributes->hostPointer = p;
  } else if (this->isPtrAllocedDev(p)) {
    attributes->memoryType = etrtMemoryTypeDevice;
    attributes->devicePointer = p;
    attributes->hostPointer = nullptr;
  } else {
    THROW("Unexpected pointer");
  }
  return etrtSuccess;
}
