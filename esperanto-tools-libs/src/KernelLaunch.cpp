/*-------------------------------------------------------------------------
 * Copyright (C) 2020, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/

#include "ExecutionContextCache.h"
#include "MemoryManager.h"
#include "RuntimeImp.h"
#include "ScopedProfileEvent.h"
#include "Utils.h"
#include "runtime/Types.h"
#include <chrono>
#include <cstdio>
#include <device-layer/IDeviceLayer.h>
#include <elfio/elfio.hpp>
#include <esperanto/device-apis/device_apis_message_types.h>
#include <esperanto/device-apis/operations-api/device_ops_api_cxx.h>
#include <esperanto/device-apis/operations-api/device_ops_api_spec.h>
#include <sstream>
#include <type_traits>

using namespace rt;
using namespace rt::profiling;

EventId RuntimeImp::kernelLaunch(StreamId streamId, KernelId kernelId, const std::byte* kernel_args,
                                 size_t kernel_args_size, uint64_t shire_mask, std::optional<UserTrace> userTraceConfig,
                                 bool barrier, bool flushL3) {
  std::unique_lock lock(mutex_);
  const auto& kernel = find(kernels_, kernelId)->second;
  ScopedProfileEvent profileEvent(Class::KernelLaunch, profiler_, streamId, kernelId, kernel->getLoadAddress());

  if (kernel_args_size > kBlockSize) {
    char buffer[1024];
    std::snprintf(buffer, sizeof buffer, "Maximum kernel arg size is %d", kBlockSize);
    throw Exception(buffer);
  }
  auto maxSizeKernelEmbeddingParameters = static_cast<uint64_t>(DEVICE_OPS_KERNEL_LAUNCH_ARGS_PAYLOAD_MAX);
  if (userTraceConfig) {
    maxSizeKernelEmbeddingParameters -= sizeof(UserTrace);
  }
  RT_LOG_IF(WARNING, kernel_args_size > maxSizeKernelEmbeddingParameters)
    << "Kernel args size larger than " << maxSizeKernelEmbeddingParameters
    << " implies an extra DMA transfer; try to send less parameters to achieve maximum performance.";

  auto streamInfo = streamManager_.getStreamInfo(streamId);

  if (DeviceId{streamInfo.device_} != kernel->deviceId_) {
    throw Exception("Can't execute stream and kernel associated to a different device");
  }

  // TODO: SW-7615 - Allocation/extraction of U mode Kernel Trace buffer
  bool kernelArgsFit = kernel_args_size <= maxSizeKernelEmbeddingParameters;
  auto optionalArgSize = kernelArgsFit ? kernel_args_size : 0;
  if (userTraceConfig) {
    optionalArgSize += sizeof(UserTrace);
  }

  std::vector<std::byte> cmdBase(sizeof(device_ops_api::device_ops_kernel_launch_cmd_t) + optionalArgSize);

  memset(cmdBase.data(), 0, sizeof(cmdBase));

  auto cmdPtr = reinterpret_cast<device_ops_api::device_ops_kernel_launch_cmd_t*>(cmdBase.data());

  auto pBuffer = executionContextCache_->allocBuffer(kernel->deviceId_);
  auto pPayload = reinterpret_cast<std::byte*>(cmdPtr->argument_payload);
  if (userTraceConfig) {
    memcpy(pPayload, &*userTraceConfig, sizeof(UserTrace));
    pPayload += sizeof(UserTrace);
  }
  if (kernelArgsFit) {
    std::copy(kernel_args, kernel_args + kernel_args_size, pPayload);
  } else {
    barrier = true; // we must wait for parameters, so barrier is true if parameters
    // stage parameters in host buffer
    std::copy(kernel_args, kernel_args + kernel_args_size, begin(pBuffer->hostBuffer_));
    memcpyHostToDevice(streamId, pBuffer->hostBuffer_.data(), pBuffer->getParametersPtr(), kernel_args_size, false);
  }
  auto event = eventManager_.getNextId();
  streamManager_.addEvent(streamId, event);
  executionContextCache_->reserveBuffer(event, pBuffer);

  cmdPtr->command_info.cmd_hdr.tag_id = static_cast<uint16_t>(event);
  cmdPtr->command_info.cmd_hdr.msg_id = device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_KERNEL_LAUNCH_CMD;
  cmdPtr->command_info.cmd_hdr.size = sizeof(device_ops_api::device_ops_kernel_launch_cmd_t);
  if (optionalArgSize > 0) {
    auto size = cmdPtr->command_info.cmd_hdr.size + optionalArgSize;
    cmdPtr->command_info.cmd_hdr.size = static_cast<msg_size_t>(size);
  }
  cmdPtr->command_info.cmd_hdr.flags = 0;
  if (barrier) {
    cmdPtr->command_info.cmd_hdr.flags |= device_ops_api::CMD_FLAGS_BARRIER_ENABLE;
  }
  if (flushL3) {
    cmdPtr->command_info.cmd_hdr.flags |= device_ops_api::CMD_FLAGS_KERNEL_LAUNCH_FLUSH_L3;
  }
  if (userTraceConfig) {
    cmdPtr->command_info.cmd_hdr.flags |= device_ops_api::CMD_FLAGS_COMPUTE_KERNEL_TRACE_ENABLE;
  }
  if (kernelArgsFit) {
    // This should replaced with-> device_ops_api::EMBED_KERNEL_ARGS_IN_OPT_PAYLOAD
    cmdPtr->command_info.cmd_hdr.flags |= (1 << 5);
  }

  // This will be removed once Device API has been updated
  cmdPtr->pointer_to_trace_cfg = 0;
  cmdPtr->exception_buffer = reinterpret_cast<uint64_t>(pBuffer->getExceptionContextPtr());
  cmdPtr->code_start_address = kernel->getEntryAddress();
  cmdPtr->pointer_to_args = reinterpret_cast<uint64_t>(pBuffer->getParametersPtr());
  cmdPtr->shire_mask = shire_mask;

  RT_VLOG(LOW) << "Pushing kernel Launch Command on SQ: " << streamInfo.vq_
               << " EventId: " << cmdPtr->command_info.cmd_hdr.tag_id << std::hex << ", parameters: 0x"
               << cmdPtr->pointer_to_args << ", PC: 0x" << cmdPtr->code_start_address << ", shire_mask: 0x"
               << shire_mask;
  auto& commandSender = find(commandSenders_, getCommandSenderIdx(streamInfo.device_, streamInfo.vq_))->second;
  commandSender.send(Command{cmdBase, commandSender, false, true});
  profileEvent.setEventId(event);

  Sync(event);
  return event;
}
