/*-------------------------------------------------------------------------
 * Copyright (C) 2020, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/

#include "RuntimeImp.h"
#include "TargetSilicon.h"
#include "TargetSysEmu.h"

using namespace rt;
RuntimeImp::RuntimeImp(Kind kind) {
  switch (kind) {
  case Kind::SysEmu:
    target_ = std::make_unique<TargetSysEmu>();
    break;
  case Kind::Silicon:
    target_ = std::make_unique<TargetSilicon>();
    break;
  default:
    throw Exception("Not implemented");
  }
}

std::vector<Device> RuntimeImp::getDevices() const { return target_->getDevices(); }

Kernel RuntimeImp::loadCode(Device device, std::byte* elf, size_t elf_size) { throw Exception("Not implemented yet"); }

void RuntimeImp::unloadCode(Kernel kernel) { throw Exception("Not implemented yet"); }

std::byte* RuntimeImp::mallocDevice(Device device, size_t size, int alignment) {
  throw Exception("Not implemented yet");
}
void RuntimeImp::freeDevice(Device device, std::byte* buffer) { throw Exception("Not implemented yet"); }

Stream RuntimeImp::createStream(Device device) { throw Exception("Not implemented yet"); }
void RuntimeImp::destroyStream(Stream stream) { throw Exception("Not implemented yet"); }

Event RuntimeImp::kernelLaunch(Stream stream, Kernel kernel, std::byte* kernel_args, size_t kernel_args_size,
                               bool barrier) {
  throw Exception("Not implemented yet");
}

Event RuntimeImp::memcpyHostToDevice(Stream stream, std::byte* src, std::byte* dst, size_t size, bool barrier) {
  throw Exception("Not implemented yet");
}
Event RuntimeImp::memcpyDeviceToHost(Stream stream, std::byte* src, std::byte* dst, size_t size, bool barrier) {
  throw Exception("Not implemented yet");
}

void RuntimeImp::waitForEvent(Event event) { throw Exception("Not implemented yet"); }
void RuntimeImp::waitForStream(Stream stream) { throw Exception("Not implemented yet"); }