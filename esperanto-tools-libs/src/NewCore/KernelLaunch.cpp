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
#include "TargetSilicon.h"
#include "TargetSysEmu.h"
#include "utils.h"
#include <chrono>
#include <cstdio>
#include <elfio/elfio.hpp>
#include <esperanto/device-api/device_api_rpc_types_non_privileged.h>
#include <esperanto/device-api/device_api_spec_non_privileged.h>
#include <sstream>

using namespace rt;
using namespace rt::profiling;

EventId RuntimeImp::kernelLaunch(StreamId streamId, KernelId kernelId, const void* kernel_args,
                                 size_t kernel_args_size, uint64_t shire_mask, bool barrier) {
  ScopedProfileEvent profileEvent(Class::KernelLaunch, profiler_, streamId);
  if (kernel_args_size > kMinAllocationSize) {
    char buffer[1024];
    std::snprintf(buffer, sizeof buffer, "Maximum kernel arg size is %d", kMinAllocationSize);
    throw Exception(buffer);
  }

  auto&& stream = find(streams_, streamId)->second;
  auto&& kernel = find(kernels_, kernelId)->second;

  if (stream.deviceId_ != kernel->deviceId_) {
    throw Exception("Can't execute stream and kernel associated to a different device");
  }

  void* pBuffer = nullptr;
  if (kernel_args_size > 0) {
    barrier = true; // we must wait for parameters, so barrier is true if parameters
    pBuffer = kernelParametersCache_->allocBuffer(kernel->deviceId_);
    // TODO fix this properly once SW-5098 is done  !
    std::byte stageParams[1024];
    assert(kernel_args_size <= sizeof stageParams);
    memcpy(stageParams, kernel_args, kernel_args_size);
    auto memcpyEvt = memcpyHostToDevice(streamId, stageParams, pBuffer, sizeof stageParams);
    stream.lastEventId_ = memcpyEvt;
  }
  auto event = eventManager_.getNextId();

  if (kernel_args_size > 0) {
    kernelParametersCache_->reserveBuffer(event, pBuffer);
  }

  // all old stuff from commandsGen here, except tracing
  using namespace std::chrono;
  auto time = steady_clock::now();
  auto cmdId = static_cast<uint64_t>(event);

  kernel_launch_cmd_t cmd_info_;
  cmd_info_.command_info.message_id = MBOX_DEVAPI_NON_PRIVILEGED_MID_KERNEL_LAUNCH_CMD;
  cmd_info_.command_info.command_id = cmdId;
  cmd_info_.command_info.host_timestamp = duration_cast<milliseconds>(time.time_since_epoch()).count();
  cmd_info_.command_info.stream_id = static_cast<uint32_t>(streamId);
  cmd_info_.code_start_address = kernel->getEntryAddress();
  cmd_info_.pointer_to_args = reinterpret_cast<uint64_t>(pBuffer);
  cmd_info_.shire_mask = shire_mask;

  RT_DLOG(INFO) << "Writing execute kernel command into mailbox. Command id: " << std::hex << cmdId
                << ", parameters: " <<  cmd_info_.pointer_to_args
                << ", PC: "<< cmd_info_.code_start_address << ", shire_mask: " << shire_mask;
  if (!target_->writeMailbox(&cmd_info_, sizeof(cmd_info_))) {
    throw Exception("Error writing command to mailbox");
  }
  stream.lastEventId_ = event;
  // TODO we need support for VQs, so we must explictly wait till the kernel has been already executed and the event
  // dispatched
  waitForEvent(event);
  return event;
}
