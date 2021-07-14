/*-------------------------------------------------------------------------
 * Copyright (C) 2020, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/

#include "KernelParametersCache.h"
#include "MemoryManager.h"
#include "RuntimeImp.h"
#include "ScopedProfileEvent.h"
#include "utils.h"
#include <chrono>
#include <cstdio>
#include <device-layer/IDeviceLayer.h>
#include <elfio/elfio.hpp>
#include <esperanto/device-apis/operations-api/device_ops_api_cxx.h>
#include <esperanto/device-apis/operations-api/device_ops_api_spec.h>
#include <sstream>
#include <type_traits>

using namespace rt;
using namespace rt::profiling;

EventId RuntimeImp::kernelLaunch(StreamId streamId, KernelId kernelId, const void* kernel_args, size_t kernel_args_size,
                                 uint64_t shire_mask, bool barrier, bool flushL3) {
  std::unique_lock lock(mutex_);
  auto&& kernel = find(kernels_, kernelId)->second;
  ScopedProfileEvent profileEvent(Class::KernelLaunch, profiler_, streamId, kernelId, kernel->getLoadAddress());

  if (kernel_args_size > kBlockSize) {
    char buffer[1024];
    std::snprintf(buffer, sizeof buffer, "Maximum kernel arg size is %d", kBlockSize);
    throw Exception(buffer);
  }

  std::unique_lock lock2(streamsMutex_);
  const auto& stream = find(streams_, streamId)->second;
  lock2.unlock();

  if (stream.deviceId_ != kernel->deviceId_) {
    throw Exception("Can't execute stream and kernel associated to a different device");
  }

  KernelParametersCache::Buffer* pBuffer = nullptr;
  if (kernel_args_size > 0) {
    barrier = true; // we must wait for parameters, so barrier is true if parameters
    pBuffer = kernelParametersCache_->allocBuffer(kernel->deviceId_);
    //stage parameters in host buffer
    auto args = reinterpret_cast<const std::byte*>(kernel_args);
    std::copy(args, args + kernel_args_size, begin(pBuffer->hostBuffer_));
    memcpyHostToDevice(streamId, pBuffer->hostBuffer_.data(), pBuffer->deviceBuffer_, kernel_args_size, false);
  }
  auto event = eventManager_.getNextId();
  lock2.lock();
  find(streams_, streamId)->second.addEvent(event);
  lock2.unlock();
  if (kernel_args_size > 0) {
    kernelParametersCache_->reserveBuffer(event, pBuffer);
  }

  // todo: SW-7616 - Allocation/extraction of Error buffer
  // SW-7615 - Allocation/extraction of U mode Kernel Trace buffer
  // SW-7558 - Embed kernel parameters into kernel command to avoid DMA
  device_ops_api::device_ops_kernel_launch_cmd_t cmd;
  memset(&cmd, 0, sizeof(cmd));
  cmd.command_info.cmd_hdr.tag_id = static_cast<uint16_t>(event);
  cmd.command_info.cmd_hdr.msg_id = device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_KERNEL_LAUNCH_CMD;
  cmd.command_info.cmd_hdr.size = sizeof(cmd);
  cmd.command_info.cmd_hdr.flags = 0;
  if (barrier) {
    cmd.command_info.cmd_hdr.flags |= device_ops_api::CMD_FLAGS_BARRIER_ENABLE;
  }
  if (flushL3) {
    cmd.command_info.cmd_hdr.flags |= device_ops_api::CMD_FLAGS_KERNEL_LAUNCH_FLUSH_L3;
  }

  cmd.code_start_address = kernel->getEntryAddress();
  cmd.pointer_to_args = kernel_args_size > 0 ? reinterpret_cast<uint64_t>(pBuffer->deviceBuffer_) : 0;
  cmd.shire_mask = shire_mask;

  RT_VLOG(LOW) << "Pushing kernel Launch Command on SQ: " << stream.vq_ << " Tag id: " << std::hex
               << cmd.command_info.cmd_hdr.tag_id << ", parameters: " << cmd.pointer_to_args
               << ", PC: " << cmd.code_start_address << ", shire_mask: " << shire_mask;
  sendCommandMasterMinion(stream.vq_, static_cast<int>(stream.deviceId_), cmd, lock);
  profileEvent.setEventId(event);

  Sync(event);
  return event;
}
