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
#include "runtime/IRuntime.h"
#include "runtime/Types.h"
#include <string_view>
#include <thread>
#include <vector>

namespace rt {
class Client : public IRuntime {
public:
  explicit Client(const std::string& socketPath);

  std::vector<DeviceId> getDevices() override;
  std::byte* mallocDevice(DeviceId device, size_t size, uint32_t alignment = kCacheLineSize) override;
  void freeDevice(DeviceId device, std::byte* buffer) override;
  StreamId createStream(DeviceId device) override;
  void destroyStream(StreamId stream) override;
  LoadCodeResult loadCode(StreamId stream, const std::byte* elf, size_t elf_size) override;
  void unloadCode(KernelId kernel) override;
  EventId kernelLaunch(StreamId stream, KernelId kernel, const std::byte* kernel_args, size_t kernel_args_size,
                       uint64_t shire_mask, bool barrier = true, bool flushL3 = false,
                       std::optional<UserTrace> userTraceConfig = std::nullopt) override;
  EventId memcpyHostToDevice(StreamId stream, const std::byte* h_src, std::byte* d_dst, size_t size,
                             bool barrier = false) override;

  EventId memcpyDeviceToHost(StreamId stream, const std::byte* d_src, std::byte* h_dst, size_t size,
                             bool barrier = true) override;

  EventId memcpyHostToDevice(StreamId stream, MemcpyList memcpyList, bool barrier = false) override;

  EventId memcpyDeviceToHost(StreamId stream, MemcpyList memcpyList, bool barrier = true) override;

  bool waitForEvent(EventId event, std::chrono::seconds timeout = std::chrono::hours(24)) override;

  bool waitForStream(StreamId stream, std::chrono::seconds timeout = std::chrono::hours(24)) override;

  std::vector<StreamError> retrieveStreamErrors(StreamId stream) override;

  void setOnStreamErrorsCallback(StreamErrorCallback callback) override;

  IProfiler* getProfiler() override;

  EventId memcpyHostToDevice(StreamId, const IDmaBuffer*, std::byte*, size_t, bool) override {
    throw Exception("Not implemented.");
  }

  EventId memcpyDeviceToHost(StreamId, const std::byte*, IDmaBuffer*, size_t, bool) override {
    throw Exception("Not implemented.");
  }

  std::unique_ptr<IDmaBuffer> allocateDmaBuffer(DeviceId, size_t, bool) override {
    throw Exception("Not implemented.");
  }

  EventId setupDeviceTracing(StreamId, uint32_t, uint32_t, uint32_t, uint32_t, bool) override {
    throw Exception("Not implemented.");
  }

  EventId startDeviceTracing(StreamId, std::ostream*, std::ostream*, bool) override {
    throw Exception("Not implemented.");
  }

  EventId stopDeviceTracing(StreamId, bool) override {
    throw Exception("Not implemented.");
  }

  EventId abortCommand(EventId commandId, std::chrono::milliseconds timeout = std::chrono::milliseconds(5000)) override;

  EventId abortStream(StreamId streamId) override;

private:
  int socket_;
  bool running_ = true;
  std::thread listener_;
};
} // namespace rt