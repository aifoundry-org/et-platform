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

#include "EventManager.h"
#include "MemoryManager.h"
#include "ProfilerImp.h"
#include "ResponseReceiver.h"
#include "runtime/IRuntime.h"
#include "utils.h"
#include <mutex>
#include <thread>
#include <unordered_map>

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
  bool waitForEvent(EventId event, std::chrono::seconds timeout) override;
  void waitForStream(StreamId stream) override;
  bool waitForStream(StreamId stream, std::chrono::seconds timeout) override;

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


  template<typename Command, typename Lock>
  void sendCommandMasterMinion(Stream& stream, EventId event, Command&& command, Lock& lock) {
    auto sqIdx = stream.vq_;
    auto device = static_cast<int>(stream.deviceId_);
    bool done = false;
    while (!done) {
      done = deviceLayer_->sendCommandMasterMinion(device, sqIdx,
                                                  reinterpret_cast<std::byte*>(&command), sizeof(command));
      if (!done) {
        lock.unlock();
        RT_LOG(INFO) << "Submission queue " << sqIdx
                     << " is full. Can't send command now, blocking the thread till an event has been dispatched.";
        uint64_t sq_bitmap;
        bool cq_available;
        auto events = eventManager_.getOnflyEvents();
        if (events.empty()) {
          throw Exception("Submission queue is full but there are not on-fly events. There could be a firmware bug.");
        }
        /*RT_DLOG(INFO) << "SendCommandMasterMinion: Waiting for event " << static_cast<int>(*events.begin())
                      << " for 1 second";*/
        RT_DLOG(INFO) << "Waiting for 100ms";
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        // waitForEvent(*events.begin());
        lock.lock();
      }
    }
    stream.lastEventId_ = event;
  }

  dev::IDeviceLayer* deviceLayer_;
  std::vector<DeviceId> devices_;
  std::vector<QueueHelper> queueHelpers_;
  std::unordered_map<DeviceId, MemoryManager> memoryManagers_;
  std::unordered_map<StreamId, Stream> streams_;

  EventManager eventManager_;

  // using unique_ptr to not have to deal with elfio mess (the class is not friendly with modern c++)
  std::unordered_map<KernelId, std::unique_ptr<Kernel>> kernels_;
  std::unique_ptr<ResponseReceiver> responseReceiver_;

  int nextKernelId_ = 0;
  int nextStreamId_ = 0;

  std::recursive_mutex mutex_;

  profiling::ProfilerImp profiler_;
  std::unique_ptr<KernelParametersCache> kernelParametersCache_;
};
} // namespace rt
