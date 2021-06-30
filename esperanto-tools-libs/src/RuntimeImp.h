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

#include "DmaBufferManager.h"
#include "EventManager.h"
#include "MemoryManager.h"
#include "ProfilerImp.h"
#include "ResponseReceiver.h"
#include "runtime/IRuntime.h"
#include "utils.h"
#include <mutex>
#include <thread>
#include <unordered_map>

using namespace std::chrono_literals;
namespace rt {
class KernelParametersCache;
class MailboxReader;
class MemoryManager;
class ITarget;
class RuntimeImp : public IRuntime, public ResponseReceiver::IReceiverServices {
public:
  RuntimeImp(dev::IDeviceLayer* deviceLayer);
  ~RuntimeImp();

  std::vector<DeviceId> getDevices() override;

  KernelId loadCode(DeviceId device, const void* elf, size_t elf_size) override;
  void unloadCode(KernelId kernel) override;
 
  void* mallocDevice(DeviceId device, size_t size, uint32_t alignment = kCacheLineSize) override;
  void freeDevice(DeviceId device, void* buffer) override;

  StreamId createStream(DeviceId device) override;
  void destroyStream(StreamId stream) override;

  EventId kernelLaunch(StreamId stream, KernelId kernel, const void* kernel_args, size_t kernel_args_size,
                       uint64_t shire_mask, bool barrier = true, bool flushL3 = true) override;

  EventId memcpyHostToDevice(StreamId stream, const void* src, void* dst, size_t size,
                             bool barrier = false) override;
  EventId memcpyDeviceToHost(StreamId stream, const void* src, void* dst, size_t size,
                             bool barrier = true) override;

  void waitForEvent(EventId event) override;
  bool waitForEvent(EventId event, std::chrono::milliseconds timeout) override;
  void waitForStream(StreamId stream) override;
  bool waitForStream(StreamId stream, std::chrono::milliseconds timeout) override;

  std::unique_ptr<DmaBuffer> allocateDmaBuffer(DeviceId device, size_t size, bool writeable) final;

  EventId setupDeviceTracing(StreamId stream, uint32_t shireMask, uint32_t threadMask, uint32_t eventMask,
                             uint32_t filterMask, bool barrier = true) override;
  EventId startDeviceTracing(StreamId stream, std::ostream* mmOutput, std::ostream* cmOutput,
                             bool barrier = true) override;

  EventId stopDeviceTracing(StreamId stream, bool barrier = true) override;

  IProfiler* getProfiler() override {
    return &profiler_;
  }
  // IResponseServices
  std::vector<int> getDevicesWithEventsOnFly() const override;
  void onResponseReceived(const std::vector<std::byte>& response) override;

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

    uint64_t getLoadAddress() const {
      return reinterpret_cast<uint64_t>(deviceBuffer_);
    }
    DeviceId deviceId_;
    void* deviceBuffer_;
    uint64_t entryPoint_;
  };

  struct Stream {
    Stream(DeviceId deviceId, int vq)
      : deviceId_(deviceId)
      , vq_(vq) {
    }
    DeviceId deviceId_;
    EventId lastEventId_; // last submitted event to this stream
    int vq_;
  };

  struct QueueHelper {
    explicit QueueHelper(int count)
      : queueCount_(count) {
    }
    int nextQueue() {
      auto res = nextQueue_;
      nextQueue_ = (nextQueue_ + 1) % queueCount_;
      return res;
    }
    int nextQueue_ = 0;
    int queueCount_;
  };

  struct DeviceFwTracing {
    std::unique_ptr<DmaBuffer> dmaBuffer_;
    std::ostream* mmOutput_;
    std::ostream* cmOutput_;
  };

  template <typename Command, typename Lock>
  void sendCommandMasterMinion(Stream& stream, EventId event, Command&& command, Lock& lock, bool isDma = false) {
    auto sqIdx = stream.vq_;
    auto device = static_cast<int>(stream.deviceId_);
    bool done = false;
    while (!done) {
      done = deviceLayer_->sendCommandMasterMinion(device, sqIdx,
                                                  reinterpret_cast<std::byte*>(&command), sizeof(command), isDma);
      if (!done) {
        lock.unlock();
        RT_VLOG(LOW) << "Submission queue " << sqIdx
                     << " is full. Can't send command now, blocking the thread till an event has been dispatched.";
        auto events = eventManager_.getOnflyEvents();
        if (events.empty()) {
          throw Exception("Submission queue is full but there are not on-fly events. There could be a firmware bug.");
        }        
        waitForEvent(*events.begin(), 1s);
        lock.lock();
      }
    }
    stream.lastEventId_ = event;
  }

  dev::IDeviceLayer* deviceLayer_;
  std::vector<DeviceId> devices_;
  std::vector<QueueHelper> queueHelpers_;
  std::unordered_map<DeviceId, MemoryManager> memoryManagers_;
  std::unordered_map<DeviceId, std::unique_ptr<DmaBufferManager>> dmaBufferManagers_;
  std::unordered_map<DeviceId, DeviceFwTracing> deviceTracing_;
  std::unordered_map<StreamId, Stream> streams_;

  EventManager eventManager_;

  // using unique_ptr to not have to deal with elfio mess (the class is not friendly with modern c++)
  std::unordered_map<KernelId, std::unique_ptr<Kernel>> kernels_;

  int nextKernelId_ = 0;
  int nextStreamId_ = 0;

  mutable std::recursive_mutex mutex_;

  // this mutex should be used when accessing stream related internals
  mutable std::mutex streamsMutex_;

  profiling::ProfilerImp profiler_;
  std::unique_ptr<KernelParametersCache> kernelParametersCache_;
  std::unique_ptr<ResponseReceiver> responseReceiver_;
};
} // namespace rt
