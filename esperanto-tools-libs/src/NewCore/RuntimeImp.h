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
#include "runtime/IRuntime.h"
namespace rt {

class RuntimeImp : public IRuntime {
public:
  RuntimeImp(Kind kind);

  std::vector<Device> getDevices() const override;

  Kernel loadCode(Device device, std::byte* elf, size_t elf_size) override;
  void unloadCode(Kernel kernel) override;

  std::byte* mallocDevice(Device device, size_t size, int alignment = kCacheLineSize) override;
  void freeDevice(Device device, std::byte* buffer) override;

  Stream createStream(Device device) override;
  void destroyStream(Stream stream) override;

  Event kernelLaunch(Stream stream, Kernel kernel, std::byte* kernel_args, size_t kernel_args_size,
                     bool barrier = true) override;

  Event memcpyHostToDevice(Stream stream, std::byte* src, std::byte* dst, size_t size, bool barrier = false) override;
  Event memcpyDeviceToHost(Stream stream, std::byte* src, std::byte* dst, size_t size, bool barrier = true) override;

  void waitForEvent(Event event) override;
  void waitForStream(Stream stream) override;

private:
  std::unique_ptr<ITarget> target_;
};

} // namespace rt