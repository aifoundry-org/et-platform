/*-------------------------------------------------------------------------
 * Copyright (c) 2025 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------*/
#pragma once
#include "KernelLaunchOptionsImp.h"
#include "ProfilerImp.h"
#include "Protocol.h"
#include "StreamManager.h"
#include "runtime/IMonitor.h"
#include "runtime/Types.h"
#include <hostUtils/threadPool/ThreadPool.h>

struct sockaddr_un;

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
                         const KernelLaunchOptionsImp& options) final;
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
  void connect(sockaddr_un& addr) const;

  void onProfilerChanged() final;

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

  void processDelayedResponses();

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

  // these events won't be dispatched instantly because there are callbacks being executed
  std::set<EventId> delayedEvents_;

  // Some responses can arrive before their event id is known. They are delayed here until their event id can be matched
  std::list<resp::Response> delayedResponses_;

  int socket_;
  std::atomic<req::Id> nextId_ = 0;
  bool running_ = true;
};

} // namespace rt
