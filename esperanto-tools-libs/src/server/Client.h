/*-------------------------------------------------------------------------
 * Copyright (C) 2022, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/
#pragma once
#include "ProfilerImp.h"
#include "Protocol.h"
#include "StreamManager.h"
#include "runtime/IMonitor.h"
#include "runtime/Types.h"
#include <hostUtils/threadPool/ThreadPool.h>

namespace rt {
class Client : public IRuntime, public IMonitor {
public:
  explicit Client(const std::string& socketPath);

  ~Client() final;

  std::vector<DeviceId> doGetDevices() final;
  std::byte* doMallocDevice(DeviceId device, size_t size, uint32_t alignment = kCacheLineSize) final;
  void doFreeDevice(DeviceId device, std::byte* buffer) final;
  StreamId doCreateStream(DeviceId device) final;
  void doDestroyStream(StreamId stream) final;
  LoadCodeResult doLoadCode(StreamId stream, const std::byte* elf, size_t elf_size) final;
  void doUnloadCode(KernelId kernel) final;
  EventId doKernelLaunch(StreamId stream, KernelId kernel, const std::byte* kernel_args, size_t kernel_args_size,
                         uint64_t shire_mask, bool barrier = true, bool flushL3 = false,
                         std::optional<UserTrace> userTraceConfig = std::nullopt) final;
  EventId doMemcpyHostToDevice(StreamId stream, const std::byte* h_src, std::byte* d_dst, size_t size, bool barrier,
                               const CmaCopyFunction&) final;

  EventId doMemcpyDeviceToHost(StreamId stream, const std::byte* d_src, std::byte* h_dst, size_t size, bool barrier,
                               const CmaCopyFunction&) final;

  EventId doMemcpyHostToDevice(StreamId stream, MemcpyList memcpyList, bool barrier, const CmaCopyFunction&) final;

  EventId doMemcpyDeviceToHost(StreamId stream, MemcpyList memcpyList, bool barrier, const CmaCopyFunction&) final;

  EventId doMemcpyDeviceToDevice(StreamId streamSrc, DeviceId deviceDst, const std::byte* d_src, std::byte* d_dst,
                                 size_t size, bool barrier) final;
  EventId doMemcpyDeviceToDevice(DeviceId deviceSrc, StreamId streamDst, const std::byte* d_src, std::byte* d_dst,
                                 size_t size, bool barrier) final;

  bool doWaitForEvent(EventId event, std::chrono::seconds timeout = std::chrono::hours(24)) final;

  bool doWaitForStream(StreamId stream, std::chrono::seconds timeout = std::chrono::hours(24)) final;

  std::vector<StreamError> doRetrieveStreamErrors(StreamId stream) final;

  void doSetOnStreamErrorsCallback(StreamErrorCallback callback) final;

  DeviceProperties doGetDeviceProperties(DeviceId device) const final;

  void doSetOnKernelAbortedErrorCallback(const KernelAbortedCallback& callback) final {
    SpinLock lock(mutex_);
    kernelAbortCallback_ = callback;
  }

  EventId doAbortCommand(EventId commandId, std::chrono::milliseconds timeout = std::chrono::milliseconds(5000)) final;

  EventId doAbortStream(StreamId streamId) final;

  DmaInfo doGetDmaInfo(DeviceId deviceId) const final;

  bool doIsP2PEnabled(DeviceId one, DeviceId other) const final;

  // IMonitor implementation
  size_t getCurrentClients() final;

  std::unordered_map<DeviceId, uint64_t> getFreeMemory() final;

  std::unordered_map<DeviceId, uint32_t> getWaitingCommands() final;

  std::unordered_map<DeviceId, uint32_t> getAliveEvents() final;

private:
  template <typename Payload> resp::Response::Payload_t sendRequestAndWait(req::Type type, Payload payload) {
    auto reqId = getNextId();
    sendRequest({type, reqId, std::move(payload)});
    return waitForResponse(reqId);
  }
  void dispatch(EventId event);
  void handShake();

  void sendRequest(const req::Request& request);
  void processResponse(const resp::Response& response);

  void responseProcessor();
  req::Id getNextId();
  resp::Response::Payload_t waitForResponse(req::Id);

  EventId registerEvent(const resp::Response::Payload_t& payload, StreamId stream);
  void registerEvent(EventId evt, StreamId stream);

  // store dmaInfo and deviceConfig for each device

  struct DeviceLayerProperties {
    DmaInfo dmaInfo_;
    DeviceProperties deviceProperties_;
  };

  struct Waiter {
    template <typename Lock> void wait(Lock& lock) {
      while (!valid_) {
        RT_VLOG(MID) << "Valid is false, waiting again. This: " << this << " valid: " << valid_
                     << " condVar addr: " << &condVar_;
        condVar_.wait(lock);
      }
      RT_VLOG(MID) << "Woke up";
    }
    void notify(resp::Response::Payload_t payload) {
      payload_ = std::move(payload);
      valid_ = true;
      RT_VLOG(MID) << "Waking up. This: " << this << " valid: " << valid_ << " condVar addr: " << &condVar_;
      condVar_.notify_all();
    }
    resp::Response::Payload_t payload_;
    std::condition_variable condVar_;
    bool valid_ = false;
  };

  std::vector<DeviceId> devices_;
  std::vector<DeviceLayerProperties> deviceLayerProperties_;
  std::unordered_map<resp::Id, std::unique_ptr<Waiter>> responseWaiters_;
  std::unordered_map<EventId, StreamId> eventToStream_;
  std::unordered_map<StreamId, std::vector<EventId>> streamToEvents_;
  std::unordered_map<StreamId, std::vector<StreamError>> streamErrors_;
  resp::P2PCompatibility p2pCompatibility_;

  std::condition_variable eventSync_;
  std::thread listener_;
  std::mutex mutex_;

  StreamErrorCallback streamErrorCallback_;
  KernelAbortedCallback kernelAbortCallback_;
  threadPool::ThreadPool tp_{2};

  int socket_;
  std::atomic<req::Id> nextId_ = 0;
  bool running_ = true;
};
} // namespace rt