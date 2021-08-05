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
#include "Utils.h"
#include "runtime/IRuntime.h"
#include "runtime/Types.h"
#include <algorithm>
#include <mutex>
#include <thread>
#include <type_traits>
#include <unordered_map>

using namespace std::chrono_literals;
namespace rt {
class ExecutionContextCache;
class MemoryManager;
class RuntimeImp : public IRuntime, public ResponseReceiver::IReceiverServices {
public:
  RuntimeImp(dev::IDeviceLayer* deviceLayer);
  ~RuntimeImp();

  std::vector<DeviceId> getDevices() override;

  KernelId loadCode(DeviceId device, const std::byte* elf, size_t elf_size) override;
  void unloadCode(KernelId kernel) override;

  std::byte* mallocDevice(DeviceId device, size_t size, uint32_t alignment = kCacheLineSize) override;
  void freeDevice(DeviceId device, std::byte* buffer) override;

  StreamId createStream(DeviceId device) override;
  void destroyStream(StreamId stream) override;

  EventId kernelLaunch(StreamId stream, KernelId kernel, const std::byte* kernel_args, size_t kernel_args_size,
                       uint64_t shire_mask, bool barrier, bool flushL3) override;

  EventId memcpyHostToDevice(StreamId stream, const std::byte* src, std::byte* dst, size_t size, bool barrier) override;
  EventId memcpyDeviceToHost(StreamId stream, const std::byte* src, std::byte* dst, size_t size, bool barrier) override;

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

  void setOnStreamErrorsCallback(StreamErrorCallback callback) override;

  std::vector<StreamError> retrieveStreamErrors(StreamId stream) override;

  // IResponseServices
  std::vector<int> getDevicesWithEventsOnFly() const override;
  void onResponseReceived(const std::vector<std::byte>& response) override;

  void setMemoryManagerDebugMode(DeviceId device, bool enable);

private:
  struct Kernel {
    Kernel(DeviceId deviceId, std::byte* deviceBuffer, uint64_t entryPoint)
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
    std::byte* deviceBuffer_;
    uint64_t entryPoint_;
  };

  struct DeviceFwTracing {
    DmaBuffer dmaBuffer_;
    std::ostream* mmOutput_;
    std::ostream* cmOutput_;
  };

  void processResponseError(DeviceErrorCode errorCode, EventId event);

  inline void Sync([[maybe_unused]] EventId e) {
#ifdef RUNTIME_SYNCHRONOUS_MODE
    RT_VLOG(HIGH) << "Runtime running in sync mode. Waiting for event: " << static_cast<int>(e);
    waitForEvent(e);
#endif
  }

  dev::IDeviceLayer* deviceLayer_;
  std::vector<DeviceId> devices_;
  std::unordered_map<DeviceId, MemoryManager> memoryManagers_;
  std::unordered_map<DeviceId, std::unique_ptr<DmaBufferManager>> dmaBufferManagers_;
  std::unordered_map<DeviceId, DeviceFwTracing> deviceTracing_;

  EventManager eventManager_;
  StreamManager streamManager_;

  template <typename Command, typename Lock>
  void sendCommandMasterMinion(int sqIdx, int deviceId, Command& command, Lock& lock, bool isDma = false) {
    bool done = false;
    while (!done) {
      done = deviceLayer_->sendCommandMasterMinion(deviceId, sqIdx, reinterpret_cast<std::byte*>(&command),
                                                   sizeof(command), isDma);
      if (!done) {
        lock.unlock();
        RT_LOG(INFO) << "Submission queue " << sqIdx
                     << " is full. Can't send command now, blocking the thread till an event has been dispatched.";
        if (!streamManager_.hasEventsOnFly(DeviceId{deviceId}))
          throw Exception("Submission queue is full but there are not on-fly events. There could be a firmware bug.");
        uint64_t sq_bitmap;
        bool cq_available;
        deviceLayer_->waitForEpollEventsMasterMinion(deviceId, sq_bitmap, cq_available, std::chrono::seconds(1));
        if (sq_bitmap & (1UL << sqIdx)) {
          RT_LOG(WARNING) << "Submission queue " << sqIdx
                          << " is still unavailable after waitForEpoll, trying again nevertheless";
        } else {
          RT_VLOG(LOW) << "Submission queue " << sqIdx << " available.";
        }
        lock.lock();
      }
    }
  }

  // using unique_ptr to not have to deal with elfio mess (the class is not friendly with modern c++)
  std::unordered_map<KernelId, std::unique_ptr<Kernel>> kernels_;

  int nextKernelId_ = 0;
  int nextStreamId_ = 0;

  mutable std::recursive_mutex mutex_;

  profiling::ProfilerImp profiler_;
  std::unique_ptr<ExecutionContextCache> executionContextCache_;
  std::unique_ptr<ResponseReceiver> responseReceiver_;
  // TODO refactor this shit... these are the response threads which must be waited at some point
  std::vector<std::thread> threadsToJoin_;
};
} // namespace rt
