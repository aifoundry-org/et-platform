/*-------------------------------------------------------------------------
 * Copyright (C) 2020, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/

#pragma once

#include "ProfilerImp.h"

#include "EventManager.h"
#include "MemoryManager.h"
#include "runtime/IRuntime.h"
#include <mutex>
#include <thread>
#include <unordered_map>

namespace rt {
class KernelParametersCache;
class MailboxReader;
class MemoryManager;
class ITarget;
class RuntimeImp : public IRuntime {
public:
  RuntimeImp(Kind kind);
  ~RuntimeImp();

  std::vector<DeviceId> getDevices() override;

  KernelId loadCode(DeviceId device, const void* elf, size_t elf_size) override;
  void unloadCode(KernelId kernel) override;

  void* mallocDevice(DeviceId device, size_t size, int alignment = kCacheLineSize) override;
  void freeDevice(DeviceId device, void* buffer) override;

  StreamId createStream(DeviceId device) override;
  void destroyStream(StreamId stream) override;

  EventId kernelLaunch(StreamId stream, KernelId kernel, const void* kernel_args, size_t kernel_args_size,
                       uint64_t shire_mask, bool barrier = true) override;

  EventId memcpyHostToDevice(StreamId stream, const void* src, void* dst, size_t size,
                             bool barrier = false) override;
  EventId memcpyDeviceToHost(StreamId stream, const void* src, void* dst, size_t size,
                             bool barrier = true) override;

  void waitForEvent(EventId event) override;
  void waitForStream(StreamId stream) override;

  IProfiler* getProfiler() override {
    return &profiler_;
  }

private:
  struct Kernel {
    Kernel(DeviceId deviceId, void* deviceBuffer, uint64_t entryPoint)
      : deviceId_(deviceId)
      , deviceBuffer_(deviceBuffer)
      , entryPoint_(entryPoint) {
    }

    uint64_t getEntryAddress() const {
      return reinterpret_cast<uint64_t>(deviceBuffer_) + entryPoint_;
    }
    DeviceId deviceId_;
    void* deviceBuffer_;
    uint64_t entryPoint_;
  };

  struct Stream {
    //#TODO we will add later VQs information see epic SW-4377
    Stream(DeviceId deviceId, int vq)
      : deviceId_(deviceId)
      , vq_(vq) {
    }
    DeviceId deviceId_;
    EventId lastEventId_; // last submitted event to this stream
    int vq_;
  };

  std::unique_ptr<MailboxReader> mailboxReader_;
  std::unique_ptr<ITarget> target_;
  std::vector<DeviceId> devices_;
  std::unordered_map<DeviceId, MemoryManager> memoryManagers_;
  std::unordered_map<StreamId, Stream> streams_;
  std::unique_ptr<KernelParametersCache> kernelParametersCache_;

  EventManager eventManager_;

  // using unique_ptr to not have to deal with elfio mess (the class is not friendly with modern c++)
  std::unordered_map<KernelId, std::unique_ptr<Kernel>> kernels_;

  int nextKernelId_ = 0;
  int nextStreamId_ = 0;

  std::recursive_mutex mutex_;

  profiling::ProfilerImp profiler_;  
};
} // namespace rt