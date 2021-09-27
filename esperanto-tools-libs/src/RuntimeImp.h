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

#include "CommandSender.h"
#include "EventManager.h"
#include "MemoryManager.h"
#include "ProfilerImp.h"
#include "ResponseReceiver.h"
#include "StreamManager.h"
#include "Utils.h"
#include "dma/HostBufferManager.h"
#include "runtime/IDmaBuffer.h"
#include "runtime/IRuntime.h"
#include "runtime/Types.h"
#include <algorithm>
#include <hostUtils/threadPool/ThreadPool.h>
#include <type_traits>
#include <unordered_map>

namespace rt {
class ExecutionContextCache;
class MemoryManager;
class RuntimeImp : public IRuntime, public ResponseReceiver::IReceiverServices {
public:
  explicit RuntimeImp(dev::IDeviceLayer* deviceLayer);

  std::vector<DeviceId> getDevices() final;

  LoadCodeResult loadCode(StreamId stream, const std::byte* elf, size_t elf_size) final;
  void unloadCode(KernelId kernel) final;

  std::byte* mallocDevice(DeviceId device, size_t size, uint32_t alignment = kCacheLineSize) final;
  void freeDevice(DeviceId device, std::byte* buffer) final;

  StreamId createStream(DeviceId device) final;
  void destroyStream(StreamId stream) final;

  EventId kernelLaunch(StreamId stream, KernelId kernel, const std::byte* kernel_args, size_t kernel_args_size,
                       uint64_t shire_mask, std::optional<UserTrace> userTraceConfig, bool barrier, bool flushL3) final;

  EventId memcpyHostToDevice(StreamId stream, const std::byte* src, std::byte* dst, size_t size, bool barrier) final;
  EventId memcpyDeviceToHost(StreamId stream, const std::byte* src, std::byte* dst, size_t size, bool barrier) final;

  bool waitForEvent(EventId event, std::chrono::seconds timeout = std::chrono::hours(24)) final;
  bool waitForStream(StreamId stream, std::chrono::seconds timeout = std::chrono::hours(24)) final;

  std::unique_ptr<IDmaBuffer> allocateDmaBuffer(DeviceId device, size_t size, bool writeable) final;

  EventId setupDeviceTracing(StreamId stream, uint32_t shireMask, uint32_t threadMask, uint32_t eventMask,
                             uint32_t filterMask, bool barrier) final;
  EventId startDeviceTracing(StreamId stream, std::ostream* mmOutput, std::ostream* cmOutput, bool barrier) final;

  EventId stopDeviceTracing(StreamId stream, bool barrier) final;

  EventId abortCommand(EventId commandId) final;

  IProfiler* getProfiler() final {
    return &profiler_;
  }

  void setOnStreamErrorsCallback(StreamErrorCallback callback) final;

  std::vector<StreamError> retrieveStreamErrors(StreamId stream) final;

  // IResponseServices
  std::vector<int> getDevicesWithEventsOnFly() const final;
  void onResponseReceived(const std::vector<std::byte>& response) final;

  void setMemoryManagerDebugMode(DeviceId device, bool enable);

  // this method is a helper to call eventManager dispatch and streamManager removeEvent
  void dispatch(EventId event);

private:
  void checkDevice(int device) override;
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

  struct MemcpyOp {
    enum class Direction { H2D, D2H };
    Direction direction_;
    std::vector<HostAllocation> hostMemory_;
    std::vector<std::byte*> deviceMemory_;
    std::vector<size_t> sizes_;
  };

  struct DeviceFwTracing {
    std::unique_ptr<IDmaBuffer> dmaBuffer_;
    std::ostream* mmOutput_;
    std::ostream* cmOutput_;
  };

  void processResponseError(DeviceErrorCode errorCode, EventId event);

  uint64_t getCommandSenderIdx(int deviceId, int sqIdx) const {
    return (static_cast<uint64_t>(deviceId) << 32ULL) + static_cast<uint64_t>(sqIdx);
  }

  inline void Sync([[maybe_unused]] EventId e) {
#ifdef RUNTIME_SYNCHRONOUS_MODE
    RT_VLOG(HIGH) << "Runtime running in sync mode. Waiting for event: " << static_cast<int>(e);
    waitForEvent(e);
#endif
  }

  mutable std::recursive_mutex mutex_;
  profiling::ProfilerImp profiler_;
  dev::IDeviceLayer* deviceLayer_;
  std::vector<DeviceId> devices_;
  EventManager eventManager_;
  StreamManager streamManager_;
  std::unordered_map<DeviceId, MemoryManager> memoryManagers_;
  std::unordered_map<DeviceId, HostBufferManager> hostBufferManagers_;
  std::unordered_map<KernelId, std::unique_ptr<Kernel>> kernels_;
  std::unordered_map<DeviceId, DeviceFwTracing> deviceTracing_;
  std::unique_ptr<ExecutionContextCache> executionContextCache_;
  std::unordered_map<uint64_t, CommandSender> commandSenders_;

  // using unique_ptr to not have to deal with elfio mess (the class is not friendly with modern c++)

  int nextKernelId_ = 0;

  threadPool::ThreadPool nonblockableThreadPool_{8};
  threadPool::ThreadPool blockableThreadPool_{8};
  std::unique_ptr<ResponseReceiver> responseReceiver_;
};
} // namespace rt
