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
#include "Observer.h"
#include "ProfilerImp.h"
#include "ResponseReceiver.h"
#include "StreamManager.h"
#include "Utils.h"
#include "dma/CmaManager.h"
#include "dma/IDmaBuffer.h"
#include "runtime/IProfileEvent.h"
#include "runtime/IRuntime.h"
#include "runtime/Types.h"
#include <algorithm>
#include <hostUtils/threadPool/ThreadPool.h>
#include <limits>
#include <optional>
#include <type_traits>
#include <unordered_map>

namespace rt {
class ExecutionContextCache;
class MemoryManager;

struct DeviceApiVersion {
  using type = uint32_t;
  constexpr static type INVALID = std::numeric_limits<type>::max();
  bool isValid() const {
    return major != INVALID && minor != INVALID && patch != INVALID;
  }
  uint32_t major = INVALID;
  uint32_t minor = INVALID;
  uint32_t patch = INVALID;
};

class RuntimeImp : public IRuntime, public ResponseReceiver::IReceiverServices, public patterns::Subject<EventId> {
public:
  explicit RuntimeImp(dev::IDeviceLayer* deviceLayer, Options options);

  std::vector<DeviceId> doGetDevices() final;

  DeviceProperties doGetDeviceProperties(DeviceId device) const final;

  LoadCodeResult doLoadCode(StreamId stream, const std::byte* elf, size_t elf_size) final;
  void doUnloadCode(KernelId kernel) final;

  std::byte* doMallocDevice(DeviceId device, size_t size, uint32_t alignment = kCacheLineSize) final;
  void doFreeDevice(DeviceId device, std::byte* buffer) final;

  StreamId doCreateStream(DeviceId device) final;
  void doDestroyStream(StreamId stream) final;

  EventId doKernelLaunch(StreamId stream, KernelId kernel, const std::byte* kernel_args, size_t kernel_args_size,
                         uint64_t shire_mask, bool barrier, bool flushL3,
                         std::optional<UserTrace> userTraceConfig) final;

  EventId doMemcpyHostToDevice(StreamId stream, const std::byte* src, std::byte* dst, size_t size, bool barrier,
                               const CmaCopyFunction& cmaCopyFunction) final;
  EventId doMemcpyDeviceToHost(StreamId stream, const std::byte* src, std::byte* dst, size_t size, bool barrier,
                               const CmaCopyFunction& cmaCopyFunction) final;
  EventId doMemcpyHostToDevice(StreamId stream, MemcpyList memcpyList, bool barrier,
                               const CmaCopyFunction& cmaCopyFunction) final;
  EventId doMemcpyDeviceToHost(StreamId stream, MemcpyList memcpyList, bool barrier,
                               const CmaCopyFunction& cmaCopyFunction) final;

  bool doWaitForEvent(EventId event, std::chrono::seconds timeout = std::chrono::hours(24)) final;
  bool doWaitForStream(StreamId stream, std::chrono::seconds timeout = std::chrono::hours(24)) final;

  EventId doAbortCommand(EventId commandId, std::chrono::milliseconds timeout = std::chrono::milliseconds(5000)) final;

  EventId doAbortStream(StreamId streamId) final;

  void doSetOnStreamErrorsCallback(StreamErrorCallback callback) final;

  void doSetOnKernelAbortedErrorCallback(const KernelAbortedCallback& callback) final;

  std::vector<StreamError> doRetrieveStreamErrors(StreamId stream) final;

  DmaInfo doGetDmaInfo(DeviceId deviceId) const final;

  ~RuntimeImp() final;

  // IResponseServices
  void getDevicesWithEventsOnFly(std::vector<int>& outResult) const final;
  void onResponseReceived(const std::vector<std::byte>& response) final;

  // this method is a helper to call eventManager dispatch and streamManager removeEvent
  void dispatch(EventId event);

  // these methods are intended for debugging, internal use only
  void setMemoryManagerDebugMode(DeviceId device, bool enable);
  void setCheckMemcpyDeviceAddress(bool value) {
    checkMemcpyDeviceAddress_ = value;
  }
  void setSentCommandCallback(DeviceId device, CommandSender::CommandSentCallback callback);

private:
  friend ExecutionContextCache;

  void checkDevice(int device) override;

  void onProfilerChanged() override;

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
    std::unique_ptr<IDmaBuffer> dmaBuffer_;
    std::ostream* mmOutput_;
    std::ostream* cmOutput_;
  };
  struct ResponseError {
    struct KernelLaunchErrorExtra {
      std::byte* exceptionPtr_ = nullptr;
      std::byte* traceBufferPtr_ = nullptr;
      uint64_t cm_shire_mask;
    };
    DeviceErrorCode errorCode_;
    EventId event_;
    std::optional<KernelLaunchErrorExtra> kernelLaunchErrorExtra_ = std::nullopt;
  };
  void processResponseError(const ResponseError& error);

  void abortDevice(DeviceId d);

  void checkDeviceApi(DeviceId d);

  void checkList(int device, const MemcpyList& list) const;

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
  dev::IDeviceLayer* deviceLayer_;
  std::unique_ptr<CmaManager> cmaManager_;
  std::vector<DeviceId> devices_;
  StreamManager streamManager_;
  std::unordered_map<DeviceId, MemoryManager> memoryManagers_;
  std::unordered_map<KernelId, std::unique_ptr<Kernel>> kernels_;
  std::unordered_map<DeviceId, DeviceFwTracing> deviceTracing_;
  std::unique_ptr<ExecutionContextCache> executionContextCache_;
  std::unordered_map<uint64_t, CommandSender> commandSenders_;

  int nextKernelId_ = 0;

  std::unique_ptr<ResponseReceiver> responseReceiver_;
  threadPool::ThreadPool tp_{8};
  EventManager eventManager_;
  bool running_ = false;
  bool checkMemcpyDeviceAddress_ = false;
  DeviceApiVersion deviceApiVersion_;
  KernelAbortedCallback kernelAbortedCallback_;
};
} // namespace rt
