#include "et_device.h"
#include "utils.h"
#include <assert.h>
#include <memory.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

#define INCLUDE_FOR_HOST
#include "../kernels/sys_inc.h"
#undef INCLUDE_FOR_HOST

bool EtMemRegion::isPtrAlloced(const void *ptr) {
  if (alloced_ptrs.count(ptr) > 0) {
    return true;
  }
  for (auto i : alloced_ptrs) {
    if (i.first <= ptr) {
      if ((const char *)i.first + i.second > ptr) {
        return true;
      }
    } else {
      return false;
    }
  }
  return false;
}

void *EtMemRegion::alloc(size_t size) {
  if (size == 0) {
    return nullptr;
  }

  uintptr_t currBeg = align_up((uintptr_t)region_base, kAlign);

  for (auto it : alloced_ptrs) {
    uintptr_t currEnd = (uintptr_t)it.first;
    uintptr_t nextBeg = align_up((uintptr_t)it.first + it.second, kAlign);

    assert(currEnd >= currBeg);
    if (currEnd - currBeg >= size) {
      void *ptr = (void *)currBeg;
      alloced_ptrs[ptr] = size;
      return ptr;
    }

    currBeg = nextBeg;
  }

  uintptr_t regionEnd = (uintptr_t)region_base + region_size;
  assert(regionEnd >= currBeg);
  if (regionEnd - currBeg >= size) {
    void *ptr = (void *)currBeg;
    alloced_ptrs[ptr] = size;
    return ptr;
  }

  THROW("Can't alloc memory");
}

void EtMemRegion::free(void *ptr) {
  if (ptr == nullptr) {
    return;
  }
  assert(alloced_ptrs.count(ptr));
  alloced_ptrs.erase(ptr);
}

void EtMemRegion::print() {
  printf("start: %p\n", region_base);
  for (auto it : alloced_ptrs) {
    printf("[%p - %p)\n", it.first, (uint8_t *)it.first + it.second);
  }
  printf("end: %p\n", (uint8_t *)region_base + region_size);
}

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

void EtDevice::initMemRegions() {
  void *host_base = mmap(nullptr, kHostMemRegionSize, PROT_READ | PROT_WRITE,
                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  PERROR_IF(host_base == MAP_FAILED);
  host_mem_region.reset(new EtMemRegion(host_base, kHostMemRegionSize));

  void *kDevMemBaseAddr = (void *)GLOBAL_MEM_REGION_BASE;
  size_t kDevMemRegionSize = GLOBAL_MEM_REGION_SIZE;
  void *kKernelsDevMemBaseAddr = (void *)EXEC_MEM_REGION_BASE;
  size_t kKernelsDevMemRegionSize = EXEC_MEM_REGION_SIZE;

  void *dev_base = mmap(kDevMemBaseAddr, kDevMemRegionSize, PROT_NONE,
                        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  PERROR_IF(dev_base == MAP_FAILED);
  THROW_IF(dev_base != kDevMemBaseAddr, "Cannot allocate dev memory.");
  dev_mem_region.reset(new EtMemRegion(dev_base, kDevMemRegionSize));

  void *kernels_dev_base =
      mmap(kKernelsDevMemBaseAddr, kKernelsDevMemRegionSize, PROT_NONE,
           MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  PERROR_IF(kernels_dev_base == MAP_FAILED);
  THROW_IF(kernels_dev_base != kKernelsDevMemBaseAddr,
           "Cannot allocate kernels dev memory.");
  kernels_dev_mem_region.reset(
      new EtMemRegion(kernels_dev_base, kKernelsDevMemRegionSize));

  EtActionConfigure *actionConfigure = nullptr;
  EtActionEvent *actionEvent = nullptr;
  {
    std::lock_guard<std::mutex> lk(mutex);

    assert(defaultStream == nullptr);
    defaultStream = createStream(false);

    actionConfigure =
        new EtActionConfigure(dev_base, kDevMemRegionSize, kernels_dev_base,
                              kKernelsDevMemRegionSize);
    actionConfigure->incRefCounter();

    actionEvent = new EtActionEvent();
    actionEvent->incRefCounter();

    addAction(defaultStream, actionConfigure);
    addAction(defaultStream, actionEvent);
  }

  actionEvent->observerWait();

  if (actionConfigure->isLocalMode()) {
    PERROR_IF(mprotect(dev_base, kDevMemRegionSize, PROT_READ | PROT_WRITE) ==
              -1);
    PERROR_IF(mprotect(kernels_dev_base, kKernelsDevMemRegionSize,
                       PROT_READ | PROT_WRITE) == -1);
  }

  EtAction::decRefCounter(actionConfigure);
  EtAction::decRefCounter(actionEvent);
}

void EtDevice::uninitMemRegions() {
  munmap(host_mem_region->region_base, host_mem_region->region_size);
  host_mem_region.reset(nullptr);

  munmap(dev_mem_region->region_base, dev_mem_region->region_size);
  dev_mem_region.reset(nullptr);
}
