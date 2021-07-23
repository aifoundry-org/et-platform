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
#include "StreamManager.h"
#include "runtime/IRuntime.h"
#include "utils.h"
#include <algorithm>
#include <mutex>
#include <thread>
#include <type_traits>
#include <unordered_map>

using namespace std::chrono_literals;
namespace rt {
class KernelParametersCache;
class MemoryManager;
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
                       uint64_t shire_mask, bool barrier, bool flushL3) override;

  EventId memcpyHostToDevice(StreamId stream, const void* src, void* dst, size_t size, bool barrier) override;
  EventId memcpyDeviceToHost(StreamId stream, const void* src, void* dst, size_t size, bool barrier) override;

  bool waitForEvent(EventId event, std::chrono::seconds timeout = std::chrono::hours(24)) override;
  bool waitForStream(StreamId stream, std::chrono::seconds timeout = std::chrono::hours(24)) override;

  DmaBuffer allocateDmaBuffer(DeviceId device, size_t size, bool writeable) final;

  EventId setupDeviceTracing(StreamId stream, uint32_t shireMask, uint32_t threadMask, uint32_t eventMask,
                             uint32_t filterMask, bool barrier) override;
  EventId startDeviceTracing(StreamId stream, std::ostream* mmOutput, std::ostream* cmOutput, bool barrier) override;

  EventId stopDeviceTracing(StreamId stream, bool barrier) override;

  IProfiler* getProfiler() override {
    return &profiler_;
  }

  void onStreamErrors(StreamErrorCallback callback) override;

  std::vector<StreamError> retrieveStreamErrors(StreamId stream) override;

  // IResponseServices
  std::vector<int> getDevicesWithEventsOnFly() const override;
  void onResponseReceived(const std::vector<std::byte>& response) override;

  void setMemoryManagerDebugMode(DeviceId device, bool enable);

private:
  struct Kernel {
    Kernel(DeviceId deviceId, void* deviceBuffer, uint64_t entryPoint)
      : deviceId_(deviceId)
      , deviceBuffer_(deviceBuffer)
      , entryPoint_(entryPoint) {
      RT_VLOG(LOW) << std::hex << "Kernel loaded at device: " << static_cast<std::underlying_type_t<DeviceId>>(deviceId)
                   << " at address: " << deviceBuffer_ << " with entry point: " << entryPoint;
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

  struct DeviceFwTracing {
    DmaBuffer dmaBuffer_;
    std::ostream* mmOutput_;
    std::ostream* cmOutput_;
  };

  inline void Sync([[maybe_unused]] EventId e) {
#ifdef RUNTIME_SYNCHRONOUS_MODE
    RT_VLOG(HIGH) << "Runtime running in sync mode. Waiting for event: " << static_cast<int>(e);
    waitForEvent(e);
#endif
  }
  template <typename Command, typename Lock>
  void sendCommandMasterMinion(int sqIdx, int deviceId, Command& command, Lock& lock, bool isDma = false) {
    bool done = false;
    while (!done) {
      done = deviceLayer_->sendCommandMasterMinion(deviceId, sqIdx, reinterpret_cast<std::byte*>(&command),
                                                   sizeof(command), isDma);
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
  }

  dev::IDeviceLayer* deviceLayer_;
  std::vector<DeviceId> devices_;
  std::unordered_map<DeviceId, MemoryManager> memoryManagers_;
  std::unordered_map<DeviceId, std::unique_ptr<DmaBufferManager>> dmaBufferManagers_;
  std::unordered_map<DeviceId, DeviceFwTracing> deviceTracing_;

  EventManager eventManager_;
  StreamManager streamManager_;

  // using unique_ptr to not have to deal with elfio mess (the class is not friendly with modern c++)
  std::unordered_map<KernelId, std::unique_ptr<Kernel>> kernels_;

  int nextKernelId_ = 0;
  int nextStreamId_ = 0;

  mutable std::recursive_mutex mutex_;

  profiling::ProfilerImp profiler_;
  std::unique_ptr<KernelParametersCache> kernelParametersCache_;
  std::unique_ptr<ResponseReceiver> responseReceiver_;
};
} // namespace rt
