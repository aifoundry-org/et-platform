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

#include "ITarget.h"
#include "MemoryManager.h"
#include "runtime/IRuntime.h"
#include <elfio/elfio.hpp>
#include <unordered_map>

namespace rt {

class RuntimeImp : public IRuntime {
public: 
  RuntimeImp(Kind kind);

  std::vector<DeviceId> getDevices() const override;

  KernelId loadCode(DeviceId device, std::byte* elf, size_t elf_size) override;
  void unloadCode(KernelId kernel) override;

  std::byte* mallocDevice(DeviceId device, size_t size, int alignment = kCacheLineSize) override;
  void freeDevice(DeviceId device, std::byte* buffer) override;

  StreamId createStream(DeviceId device) override;
  void destroyStream(StreamId stream) override;

  EventId kernelLaunch(StreamId stream, KernelId kernel, std::byte* kernel_args, size_t kernel_args_size,
                       bool barrier = true) override;

  EventId memcpyHostToDevice(StreamId stream, std::byte* src, std::byte* dst, size_t size,
                             bool barrier = false) override;
  EventId memcpyDeviceToHost(StreamId stream, std::byte* src, std::byte* dst, size_t size,
                             bool barrier = true) override;

  void waitForEvent(EventId event) override;
  void waitForStream(StreamId stream) override;

private:
struct Kernel {
  Kernel(DeviceId deviceId, std::byte* elfData, size_t elfSize, std::byte* deviceBuffer);
  
  ELFIO::elfio elf_;
  DeviceId deviceId_;
  std::byte* deviceBuffer_;
};

struct Stream {
  //#TODO we will add later VQs information see epic SW-4377
  Stream(DeviceId deviceId, int vq)
    : deviceId_(deviceId)
    , vq_(vq) {
  }
  DeviceId deviceId_;
  std::underlying_type_t<EventId> lastEventId_; // last submitted event to this stream
  int vq_;
};
  std::unique_ptr<ITarget> target_;
  std::vector<DeviceId> devices_;
  std::unordered_map<DeviceId, MemoryManager> memoryManagers_;
  std::unordered_map<StreamId, Stream> streams_;

  // using unique_ptr to not have to deal with elfio mess (the class is not friendly with modern c++)
  std::unordered_map<KernelId, std::unique_ptr<Kernel>> kernels_;
  int nextKernelId_ = 0;
  int nextStreamId_ = 0;
};
} // namespace rt