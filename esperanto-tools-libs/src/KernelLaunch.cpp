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
#include "KernelLaunchOptionsImp.h"
#include "MemoryManager.h"
#include "RuntimeImp.h"
#include "ScopedProfileEvent.h"
#include "Utils.h"
#include "runtime/Types.h"
#include <chrono>
#include <cmath>
#include <cstdio>
#include <device-layer/IDeviceLayer.h>
#include <elfio/elfio.hpp>
#include <esperanto/device-apis/operations-api/device_ops_api_cxx.h>
#include <esperanto/device-apis/operations-api/device_ops_api_spec.h>
#include <ios>
#include <sstream>
#include <type_traits>

using namespace rt;
using namespace rt::profiling;

EventId RuntimeImp::doKernelLaunch(StreamId streamId, KernelId kernelId, const std::byte* kernel_args,
                                   size_t kernel_args_size, const KernelLaunchOptionsImp& options) {
  SpinLock lock(mutex_);
  const auto& kernel = find(kernels_, kernelId)->second;
  auto cfg = deviceLayer_->getDeviceConfig(static_cast<int>(kernel->deviceId_));
  auto validMask = cfg.computeMinionShireMask_;
  if (~validMask & options.shireMask_ || !(validMask & options.shireMask_)) {
    std::stringstream ss;
    ss << "Shiremask is invalid. Valid selectable values for shire mask are: 0x" << std::hex << validMask;
    throw Exception(ss.str());
  }

  if (kernel_args_size > kBlockSize) {
    char buffer[1024];
    std::snprintf(buffer, sizeof buffer, "Maximum kernel arg size is %d", kBlockSize);
    throw Exception(buffer);
  }

  constexpr auto ETSOC_THREADS_PER_SHIRE = 64;
  auto activeThreads = (size_t)__builtin_popcountll(options.shireMask_) * ETSOC_THREADS_PER_SHIRE;
  if (options.stackConfig_ && (options.stackConfig_->totalSize_ / activeThreads) < SIZE_4K) {
    std::stringstream ss;
    ss << "Stack size is too small, at least 4096 bytes per active thread (" << activeThreads << ") are needed";
    throw Exception(ss.str());
  }

  auto maxSizeKernelEmbeddingParameters = static_cast<uint64_t>(DEVICE_OPS_KERNEL_LAUNCH_ARGS_PAYLOAD_MAX);
  if (options.userTraceConfig_) {
    maxSizeKernelEmbeddingParameters -= sizeof(UserTrace);
  }
  if (options.stackConfig_) {
    maxSizeKernelEmbeddingParameters -= sizeof(StackConfiguration);
  }
  RT_LOG_IF(WARNING, kernel_args_size > maxSizeKernelEmbeddingParameters)
    << "Kernel args size larger than " << maxSizeKernelEmbeddingParameters
    << " implies an extra DMA transfer; try to send less parameters to achieve maximum performance.";

  auto streamInfo = streamManager_.getStreamInfo(streamId);

  if (DeviceId{streamInfo.device_} != kernel->deviceId_) {
    throw Exception("Can't execute stream and kernel associated to a different device");
  }

  bool kernelArgsFit = kernel_args_size <= maxSizeKernelEmbeddingParameters;
  auto optionalArgSize = kernelArgsFit ? kernel_args_size : 0;
  if (options.userTraceConfig_) {
    optionalArgSize += sizeof(UserTrace);
  }
  if (options.stackConfig_) {
    optionalArgSize += sizeof(StackConfiguration);
  }

  std::vector<std::byte> cmdBase(sizeof(device_ops_api::device_ops_kernel_launch_cmd_t) + optionalArgSize);

  memset(cmdBase.data(), 0, sizeof(cmdBase));

  auto cmdPtr = reinterpret_cast<device_ops_api::device_ops_kernel_launch_cmd_t*>(cmdBase.data());

  auto pBuffer = executionContextCache_->allocBuffer(kernel->deviceId_);
  auto pPayload = reinterpret_cast<std::byte*>(cmdPtr->argument_payload);
  if (options.userTraceConfig_) {
    memcpy(pPayload, &*options.userTraceConfig_, sizeof(UserTrace));
    pPayload += sizeof(UserTrace);
  }
  if (options.stackConfig_) {
    device_ops_api::kernel_user_stack_cfg_t stackCfg;

    auto memManager = memoryManagers_.at(kernel->deviceId_);
    auto rawStackBase = reinterpret_cast<std::byte*>(options.stackConfig_->baseAddress_);
    stackCfg.stack_base_offset = memManager.compressPointer(rawStackBase, std::log2(SIZE_4K));
    stackCfg.stack_size = static_cast<uint32_t>(options.stackConfig_->totalSize_ / SIZE_4K);

    memcpy(pPayload, &stackCfg, sizeof(device_ops_api::kernel_user_stack_cfg_t));
    pPayload += sizeof(device_ops_api::kernel_user_stack_cfg_t);
  }
  if (kernelArgsFit) {
    std::copy(kernel_args, kernel_args + kernel_args_size, pPayload);
  } else {
    // we must wait for parameters, but we will use kenelArgsFit instead of modified barrier user option.
    // stage parameters in host buffer
    std::copy(kernel_args, kernel_args + kernel_args_size, begin(pBuffer->hostBuffer_));
    doMemcpyHostToDevice(streamId, pBuffer->hostBuffer_.data(), pBuffer->getParametersPtr(), kernel_args_size, false,
                         defaultCmaCopyFunction);
  }
  auto event = eventManager_.getNextId();
  streamManager_.addEvent(streamId, event);
  executionContextCache_->reserveBuffer(event, pBuffer);
  if (!options.coreDumpFilePath_.empty()) {
    coreDumper_.addKernelExecution(options.coreDumpFilePath_, kernelId, event);
  }

  cmdPtr->command_info.cmd_hdr.tag_id = static_cast<uint16_t>(event);
  cmdPtr->command_info.cmd_hdr.msg_id = device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_KERNEL_LAUNCH_CMD;
  cmdPtr->command_info.cmd_hdr.size = sizeof(device_ops_api::device_ops_kernel_launch_cmd_t);
  if (optionalArgSize > 0) {
    auto size = cmdPtr->command_info.cmd_hdr.size + optionalArgSize;
    cmdPtr->command_info.cmd_hdr.size = static_cast<device_ops_api::msg_size_t>(size);
  }
  cmdPtr->command_info.cmd_hdr.flags = 0;
  if (!kernelArgsFit || options.barrier_) {
    cmdPtr->command_info.cmd_hdr.flags |= device_ops_api::CMD_FLAGS_BARRIER_ENABLE;
  }
  if (options.flushL3_) {
    cmdPtr->command_info.cmd_hdr.flags |= device_ops_api::CMD_FLAGS_KERNEL_LAUNCH_FLUSH_L3;
  }
  if (options.userTraceConfig_) {
    cmdPtr->command_info.cmd_hdr.flags |= device_ops_api::CMD_FLAGS_COMPUTE_KERNEL_TRACE_ENABLE;
  }
  if (kernelArgsFit) {
    // This should replaced with-> device_ops_api::EMBED_KERNEL_ARGS_IN_OPT_PAYLOAD
    cmdPtr->command_info.cmd_hdr.flags |= (1 << 5);
  }
  if (options.stackConfig_) {
    cmdPtr->command_info.cmd_hdr.flags |= device_ops_api::CMD_FLAGS_KERNEL_LAUNCH_USER_STACK_CFG;
  }

  cmdPtr->exception_buffer = reinterpret_cast<uint64_t>(pBuffer->getExceptionContextPtr());
  cmdPtr->code_start_address = kernel->getEntryAddress();
  cmdPtr->pointer_to_args = reinterpret_cast<uint64_t>(pBuffer->getParametersPtr());
  cmdPtr->shire_mask = options.shireMask_;

  RT_VLOG(LOW) << "Pushing kernel Launch Command on SQ: " << streamInfo.vq_
               << " EventId: " << cmdPtr->command_info.cmd_hdr.tag_id << std::hex << ", parameters: 0x"
               << cmdPtr->pointer_to_args << ", PC: 0x" << cmdPtr->code_start_address << ", shireMask: 0x"
               << options.shireMask_;
  auto& commandSender = find(commandSenders_, getCommandSenderIdx(streamInfo.device_, streamInfo.vq_))->second;
  commandSender.send(Command{cmdBase, commandSender, event, event, streamId, false, true});

  Sync(event);
  return event;
}
