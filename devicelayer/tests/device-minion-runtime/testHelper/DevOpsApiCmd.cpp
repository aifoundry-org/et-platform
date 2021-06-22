//******************************************************************************
// Copyright (C) 2020 Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "IDevOpsApiCmd.h"
#include <iostream>

// NOTE: This file can later be auto-generated using device-ops-api yaml files if needed

std::unordered_map<device_ops_api::tag_id_t, uint32_t> IDevOpsApiCmd::rspStorage_;
std::atomic<device_ops_api::tag_id_t> IDevOpsApiCmd::tagId_ = 0x1;

bool IDevOpsApiCmd::addRspEntry(device_ops_api::tag_id_t tagId, uint32_t expectedRsp) {
  auto it = rspStorage_.find(tagId);
  if (it != rspStorage_.end()) {
    return false;
  }

  rspStorage_.emplace(tagId, expectedRsp);
  return true;
}

uint32_t IDevOpsApiCmd::getExpectedRsp(device_ops_api::tag_id_t tagId) {
  auto it = rspStorage_.find(tagId);
  if (it == rspStorage_.end()) {
    return 0; // Default response
  }

  return it->second;
}

void IDevOpsApiCmd::deleteRspEntry(device_ops_api::tag_id_t tagId) {
  rspStorage_.erase(tagId);
}

/*
 * Device Ops Api Echo Command creation and it's handling
 */
std::unique_ptr<IDevOpsApiCmd> IDevOpsApiCmd::createEchoCmd(device_ops_api::cmd_flags_e flag) {
  auto tagId = tagId_++;
  // Add default expected response `0` for command that does not have expected response
  if (!addRspEntry(tagId, 0)) {
    return nullptr;
  }
  return std::make_unique<EchoCmd>(tagId, flag);
}

EchoCmd::EchoCmd(device_ops_api::tag_id_t tagId, device_ops_api::cmd_flags_e flag) {
  cmd_.command_info.cmd_hdr.tag_id = tagId;
  cmd_.command_info.cmd_hdr.msg_id = device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_ECHO_CMD;
  cmd_.command_info.cmd_hdr.size = sizeof(cmd_);
  cmd_.command_info.cmd_hdr.flags = flag;

  TEST_VLOG(1) << "Created Echo Command (tagId: " << std::hex << tagId << ")";
}

EchoCmd::~EchoCmd() {
  deleteRspEntry(cmd_.command_info.cmd_hdr.tag_id);
}

IDevOpsApiCmd::CmdType EchoCmd::whoAmI() {
  return CmdType::ECHO_CMD;
}

std::byte* EchoCmd::getCmdPtr() {
  return templ::bit_cast<std::byte*>(&cmd_);
}

size_t EchoCmd::getCmdSize() {
  return sizeof(cmd_);
}

device_ops_api::tag_id_t EchoCmd::getCmdTagId() {
  return cmd_.command_info.cmd_hdr.tag_id;
}

/*
 * Device Ops Api compatibility command creation and handling
 */
std::unique_ptr<IDevOpsApiCmd> IDevOpsApiCmd::createApiCompatibilityCmd(device_ops_api::cmd_flags_e flag, uint8_t major,
                                                                        uint8_t minor, uint8_t patch) {
  auto tagId = tagId_++;
  // Add default expected response `0` for command that does not have expected response
  if (!addRspEntry(tagId, 0)) {
    return nullptr;
  }
  return std::make_unique<ApiCompatibilityCmd>(tagId, flag, major, minor, patch);
}

ApiCompatibilityCmd::ApiCompatibilityCmd(device_ops_api::tag_id_t tagId, device_ops_api::cmd_flags_e flag,
                                         uint8_t major, uint8_t minor, uint8_t patch) {
  cmd_.command_info.cmd_hdr.tag_id = tagId;
  cmd_.command_info.cmd_hdr.msg_id = device_ops_api::DEV_OPS_API_MID_CHECK_DEVICE_OPS_API_COMPATIBILITY_CMD;
  cmd_.command_info.cmd_hdr.size = sizeof(cmd_);
  cmd_.command_info.cmd_hdr.flags = flag;
  cmd_.major = major;
  cmd_.major = minor;
  cmd_.major = patch;

  TEST_VLOG(1) << "Created Api Compatibility Command (tagId: " << std::hex << tagId << ")";
}

ApiCompatibilityCmd::~ApiCompatibilityCmd() {
  deleteRspEntry(cmd_.command_info.cmd_hdr.tag_id);
}

IDevOpsApiCmd::CmdType ApiCompatibilityCmd::whoAmI() {
  return CmdType::API_COMPATIBILITY_CMD;
}

std::byte* ApiCompatibilityCmd::getCmdPtr() {
  return templ::bit_cast<std::byte*>(&cmd_);
}

size_t ApiCompatibilityCmd::getCmdSize() {
  return sizeof(cmd_);
}

device_ops_api::tag_id_t ApiCompatibilityCmd::getCmdTagId() {
  return cmd_.command_info.cmd_hdr.tag_id;
}

/*
 * Device Ops Api Firmware Version command creation and handling
 */
std::unique_ptr<IDevOpsApiCmd> IDevOpsApiCmd::createFwVersionCmd(device_ops_api::cmd_flags_e flag,
                                                                 uint8_t firmwareType) {
  auto tagId = tagId_++;
  // Add default expected response `0` for command that does not have expected response
  if (!addRspEntry(tagId, 0)) {
    return nullptr;
  }
  return std::make_unique<FwVersionCmd>(tagId, flag, firmwareType);
}

FwVersionCmd::FwVersionCmd(device_ops_api::tag_id_t tagId, device_ops_api::cmd_flags_e flag, uint8_t firmwareType) {
  cmd_.command_info.cmd_hdr.tag_id = tagId;
  cmd_.command_info.cmd_hdr.msg_id = device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_DEVICE_FW_VERSION_CMD;
  cmd_.command_info.cmd_hdr.size = sizeof(cmd_);
  cmd_.command_info.cmd_hdr.flags = flag;
  cmd_.firmware_type = firmwareType;

  TEST_VLOG(1) << "Created Firmware Version Command (tagId: " << std::hex << tagId << ")";
}

FwVersionCmd::~FwVersionCmd() {
  deleteRspEntry(cmd_.command_info.cmd_hdr.tag_id);
}

IDevOpsApiCmd::CmdType FwVersionCmd::whoAmI() {
  return CmdType::FW_VERSION_CMD;
}

std::byte* FwVersionCmd::getCmdPtr() {
  return templ::bit_cast<std::byte*>(&cmd_);
}

size_t FwVersionCmd::getCmdSize() {
  return sizeof(cmd_);
}

device_ops_api::tag_id_t FwVersionCmd::getCmdTagId() {
  return cmd_.command_info.cmd_hdr.tag_id;
}

/*
 * Device Ops Api Data Write command creation and handling
 */
std::unique_ptr<IDevOpsApiCmd> IDevOpsApiCmd::createDataWriteCmd(device_ops_api::cmd_flags_e flag, uint64_t devPhysAddr,
                                                                 uint64_t hostVirtAddr, uint64_t hostPhysAddr,
                                                                 uint64_t dataSize,
                                                                 device_ops_api::dev_ops_api_dma_response_e status) {
  auto tagId = tagId_++;
  if (!addRspEntry(tagId, status)) {
    return nullptr;
  }
  return std::make_unique<DataWriteCmd>(tagId, flag, devPhysAddr, hostVirtAddr, hostPhysAddr, dataSize);
}

DataWriteCmd::DataWriteCmd(device_ops_api::tag_id_t tagId, device_ops_api::cmd_flags_e flag, uint64_t devPhysAddr,
                           uint64_t hostVirtAddr, uint64_t hostPhysAddr, uint64_t dataSize) {
  cmd_.command_info.cmd_hdr.tag_id = tagId;
  cmd_.command_info.cmd_hdr.msg_id = device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_DATA_WRITE_CMD;
  cmd_.command_info.cmd_hdr.size = sizeof(cmd_);
  cmd_.command_info.cmd_hdr.flags = flag;
  cmd_.dst_device_phy_addr = devPhysAddr;
  cmd_.src_host_virt_addr = hostVirtAddr;
  cmd_.src_host_phy_addr = hostPhysAddr;
  cmd_.size = dataSize;

  TEST_VLOG(1) << "Created Data Write Command (tagId: " << std::hex << tagId << ")";
}

DataWriteCmd::~DataWriteCmd() {
  deleteRspEntry(cmd_.command_info.cmd_hdr.tag_id);
}

IDevOpsApiCmd::CmdType DataWriteCmd::whoAmI() {
  return CmdType::DATA_WRITE_CMD;
}

std::byte* DataWriteCmd::getCmdPtr() {
  return templ::bit_cast<std::byte*>(&cmd_);
}

size_t DataWriteCmd::getCmdSize() {
  return sizeof(cmd_);
}

device_ops_api::tag_id_t DataWriteCmd::getCmdTagId() {
  return cmd_.command_info.cmd_hdr.tag_id;
}

/*
 * Device Ops Api Data Read command creation and handling
 */
std::unique_ptr<IDevOpsApiCmd> IDevOpsApiCmd::createDataReadCmd(device_ops_api::cmd_flags_e flag, uint64_t devPhysAddr,
                                                                uint64_t hostVirtAddr, uint64_t hostPhysAddr,
                                                                uint64_t dataSize,
                                                                device_ops_api::dev_ops_api_dma_response_e status) {
  auto tagId = tagId_++;
  if (!addRspEntry(tagId, status)) {
    return nullptr;
  }
  return std::make_unique<DataReadCmd>(tagId, flag, devPhysAddr, hostVirtAddr, hostPhysAddr, dataSize);
}

DataReadCmd::DataReadCmd(device_ops_api::tag_id_t tagId, device_ops_api::cmd_flags_e flag, uint64_t devPhysAddr,
                         uint64_t hostVirtAddr, uint64_t hostPhysAddr, uint64_t dataSize) {
  cmd_.command_info.cmd_hdr.tag_id = tagId;
  cmd_.command_info.cmd_hdr.msg_id = device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_DATA_READ_CMD;
  cmd_.command_info.cmd_hdr.size = sizeof(cmd_);
  cmd_.command_info.cmd_hdr.flags = flag;
  cmd_.src_device_phy_addr = devPhysAddr;
  cmd_.dst_host_virt_addr = hostVirtAddr;
  cmd_.dst_host_phy_addr = hostPhysAddr;
  cmd_.size = dataSize;

  TEST_VLOG(1) << "Created Data Read Command (tagId: " << std::hex << tagId << ")";
}

DataReadCmd::~DataReadCmd() {
  deleteRspEntry(cmd_.command_info.cmd_hdr.tag_id);
}

IDevOpsApiCmd::CmdType DataReadCmd::whoAmI() {
  return CmdType::DATA_READ_CMD;
}

std::byte* DataReadCmd::getCmdPtr() {
  return templ::bit_cast<std::byte*>(&cmd_);
}

size_t DataReadCmd::getCmdSize() {
  return sizeof(cmd_);
}

device_ops_api::tag_id_t DataReadCmd::getCmdTagId() {
  return cmd_.command_info.cmd_hdr.tag_id;
}

/*
 * Device Ops Api DMA Writelist command creation and handling
 */
std::unique_ptr<IDevOpsApiCmd> IDevOpsApiCmd::createDmaWriteListCmd(device_ops_api::cmd_flags_e flag,
                                                                    device_ops_api::dma_write_node list[],
                                                                    uint32_t nodeCount,
                                                                    device_ops_api::dev_ops_api_dma_response_e status) {
  auto tagId = tagId_++;
  if (!addRspEntry(tagId, status) || !nodeCount) {
    return nullptr;
  }
  return std::make_unique<DmaWriteListCmd>(tagId, flag, list, nodeCount);
}

DmaWriteListCmd::DmaWriteListCmd(device_ops_api::tag_id_t tagId, device_ops_api::cmd_flags_e flag,
                                 device_ops_api::dma_write_node list[], uint32_t nodeCount) {
  cmdMem_.resize(sizeof(*cmd_) + sizeof(list[0]) * nodeCount);
  cmd_ = templ::bit_cast<device_ops_api::device_ops_dma_writelist_cmd_t*>(cmdMem_.data());
  cmd_->command_info.cmd_hdr.tag_id = tagId;
  cmd_->command_info.cmd_hdr.msg_id = device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_DMA_WRITELIST_CMD;
  cmd_->command_info.cmd_hdr.size = cmdMem_.size();
  cmd_->command_info.cmd_hdr.flags = flag;
  memcpy(cmd_->list, list, sizeof(list[0]) * nodeCount);

  TEST_VLOG(1) << "Created DMA Writelist Command (tagId: " << std::hex << tagId << ", nodeCount: " << nodeCount << ")";
}

DmaWriteListCmd::~DmaWriteListCmd() {
  deleteRspEntry(cmd_->command_info.cmd_hdr.tag_id);
}

IDevOpsApiCmd::CmdType DmaWriteListCmd::whoAmI() {
  return CmdType::DMA_WRITELIST_CMD;
}

std::byte* DmaWriteListCmd::getCmdPtr() {
  return templ::bit_cast<std::byte*>(cmd_);
}

size_t DmaWriteListCmd::getCmdSize() {
  return cmd_->command_info.cmd_hdr.size;
}

device_ops_api::tag_id_t DmaWriteListCmd::getCmdTagId() {
  return cmd_->command_info.cmd_hdr.tag_id;
}

/*
 * Device Ops Api DMA Readlist command creation and handling
 */
std::unique_ptr<IDevOpsApiCmd> IDevOpsApiCmd::createDmaReadListCmd(device_ops_api::cmd_flags_e flag,
                                                                   device_ops_api::dma_read_node list[],
                                                                   uint32_t nodeCount,
                                                                   device_ops_api::dev_ops_api_dma_response_e status) {
  auto tagId = tagId_++;
  if (!addRspEntry(tagId, status) || !nodeCount) {
    return nullptr;
  }
  return std::make_unique<DmaReadListCmd>(tagId, flag, list, nodeCount);
}

DmaReadListCmd::DmaReadListCmd(device_ops_api::tag_id_t tagId, device_ops_api::cmd_flags_e flag,
                               device_ops_api::dma_read_node list[], uint32_t nodeCount) {
  cmdMem_.resize(sizeof(*cmd_) + sizeof(list[0]) * nodeCount);
  cmd_ = templ::bit_cast<device_ops_api::device_ops_dma_readlist_cmd_t*>(cmdMem_.data());
  cmd_->command_info.cmd_hdr.tag_id = tagId;
  cmd_->command_info.cmd_hdr.msg_id = device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_DMA_READLIST_CMD;
  cmd_->command_info.cmd_hdr.size = cmdMem_.size();
  cmd_->command_info.cmd_hdr.flags = flag;
  memcpy(cmd_->list, list, sizeof(list[0]) * nodeCount);

  TEST_VLOG(1) << "Created DMA Readlist Command (tagId: " << std::hex << tagId << ", nodeCount: " << nodeCount << ")";
}

DmaReadListCmd::~DmaReadListCmd() {
  deleteRspEntry(cmd_->command_info.cmd_hdr.tag_id);
}

IDevOpsApiCmd::CmdType DmaReadListCmd::whoAmI() {
  return CmdType::DMA_READLIST_CMD;
}

std::byte* DmaReadListCmd::getCmdPtr() {
  return templ::bit_cast<std::byte*>(cmd_);
}

size_t DmaReadListCmd::getCmdSize() {
  return cmd_->command_info.cmd_hdr.size;
}

device_ops_api::tag_id_t DmaReadListCmd::getCmdTagId() {
  return cmd_->command_info.cmd_hdr.tag_id;
}

/*
 * Device Ops Api Kernel Launch command creation and handling
 */
std::unique_ptr<IDevOpsApiCmd> IDevOpsApiCmd::createKernelLaunchCmd(
  device_ops_api::cmd_flags_e flag, uint64_t codeStartAddr, uint64_t ptrToArgs, uint64_t exceptionBuffer,
  uint64_t shireMask, uint64_t traceBuffer, void* argumentPayload, uint32_t sizeOfArgPayload,
  device_ops_api::dev_ops_api_kernel_launch_response_e status, std::string kernelName) {
  auto tagId = tagId_++;
  if (!addRspEntry(tagId, status)) {
    return nullptr;
  }
  return std::make_unique<KernelLaunchCmd>(tagId, flag, codeStartAddr, ptrToArgs, exceptionBuffer, shireMask,
                                           traceBuffer, argumentPayload, sizeOfArgPayload, kernelName);
}

KernelLaunchCmd::KernelLaunchCmd(device_ops_api::tag_id_t tagId, device_ops_api::cmd_flags_e flag,
                                 uint64_t codeStartAddr, uint64_t ptrToArgs, uint64_t exceptionBuffer,
                                 uint64_t shireMask, uint64_t traceBuffer, void* argumentPayload,
                                 uint32_t sizeOfArgPayload, std::string kernelName) {
  cmdMem_.resize(sizeof(*cmd_) + sizeOfArgPayload);
  kernelName_ = kernelName;
  cmd_ = templ::bit_cast<device_ops_api::device_ops_kernel_launch_cmd_t*>(cmdMem_.data());
  cmd_->command_info.cmd_hdr.tag_id = tagId;
  cmd_->command_info.cmd_hdr.msg_id = device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_KERNEL_LAUNCH_CMD;
  cmd_->command_info.cmd_hdr.size = cmdMem_.size();
  cmd_->command_info.cmd_hdr.flags = flag;
  cmd_->code_start_address = codeStartAddr;
  cmd_->pointer_to_args = ptrToArgs;
  cmd_->exception_buffer = exceptionBuffer;
  cmd_->shire_mask = shireMask;
  cmd_->kernel_trace_buffer = traceBuffer;
  if (sizeOfArgPayload > 0) {
    memcpy(cmd_->argument_payload, argumentPayload, sizeOfArgPayload);
  }

  TEST_VLOG(1) << "Created Kernel Launch Command (tagId: " << std::hex << tagId << ")";
}

KernelLaunchCmd::~KernelLaunchCmd() {
  deleteRspEntry(cmd_->command_info.cmd_hdr.tag_id);
}

IDevOpsApiCmd::CmdType KernelLaunchCmd::whoAmI() {
  return CmdType::KERNEL_LAUNCH_CMD;
}

std::byte* KernelLaunchCmd::getCmdPtr() {
  return templ::bit_cast<std::byte*>(cmd_);
}

size_t KernelLaunchCmd::getCmdSize() {
  return cmd_->command_info.cmd_hdr.size;
}

device_ops_api::tag_id_t KernelLaunchCmd::getCmdTagId() {
  return cmd_->command_info.cmd_hdr.tag_id;
}

std::string KernelLaunchCmd::getKernelName() {
  return kernelName_;
}

uint16_t KernelLaunchCmd::getCmdFlags() {
  return cmd_->command_info.cmd_hdr.flags;
}

uint64_t KernelLaunchCmd::getShireMask() {
  return cmd_->shire_mask;
}

/*
 * Device Ops Api Kernel Abort command creation and handling
 */
std::unique_ptr<IDevOpsApiCmd>
IDevOpsApiCmd::createKernelAbortCmd(device_ops_api::cmd_flags_e flag, device_ops_api::tag_id_t kernelToAbortTagId,
                                    device_ops_api::dev_ops_api_kernel_abort_response_e status) {
  auto tagId = tagId_++;
  if (!addRspEntry(tagId, status)) {
    return nullptr;
  }
  return std::make_unique<KernelAbortCmd>(tagId, flag, kernelToAbortTagId);
}

KernelAbortCmd::KernelAbortCmd(device_ops_api::tag_id_t tagId, device_ops_api::cmd_flags_e flag,
                               device_ops_api::tag_id_t kernelToAbortTagId) {
  cmd_.command_info.cmd_hdr.tag_id = tagId;
  cmd_.command_info.cmd_hdr.msg_id = device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_KERNEL_ABORT_CMD;
  cmd_.command_info.cmd_hdr.size = sizeof(cmd_);
  cmd_.command_info.cmd_hdr.flags = flag;
  cmd_.kernel_launch_tag_id = kernelToAbortTagId;

  TEST_VLOG(1) << "Created Kernel Abort Command (tagId: " << std::hex << tagId << ")";
}

KernelAbortCmd::~KernelAbortCmd() {
  deleteRspEntry(cmd_.command_info.cmd_hdr.tag_id);
}

IDevOpsApiCmd::CmdType KernelAbortCmd::whoAmI() {
  return CmdType::KERNEL_ABORT_CMD;
}

std::byte* KernelAbortCmd::getCmdPtr() {
  return templ::bit_cast<std::byte*>(&cmd_);
}

size_t KernelAbortCmd::getCmdSize() {
  return sizeof(cmd_);
}

device_ops_api::tag_id_t KernelAbortCmd::getCmdTagId() {
  return cmd_.command_info.cmd_hdr.tag_id;
}

/*
 * Device Ops Api trace configuration command creation and handling
 */
std::unique_ptr<IDevOpsApiCmd>
IDevOpsApiCmd::createTraceRtConfigCmd(device_ops_api::cmd_flags_e flag, uint32_t shire_mask, uint32_t thread_mask,
                                      uint32_t event_mask, uint32_t filter_mask,
                                      device_ops_api::dev_ops_trace_rt_config_response_e status) {
  auto tagId = tagId_++;
  if (!addRspEntry(tagId, status)) {
    return nullptr;
  }
  return std::make_unique<TraceRtConfigCmd>(tagId, flag, shire_mask, thread_mask, event_mask, filter_mask);
}

TraceRtConfigCmd::TraceRtConfigCmd(device_ops_api::tag_id_t tagId, device_ops_api::cmd_flags_e flag,
                                   uint32_t shire_mask, uint32_t thread_mask, uint32_t event_mask,
                                   uint32_t filter_mask) {

  cmd_.command_info.cmd_hdr.tag_id = tagId;
  cmd_.command_info.cmd_hdr.msg_id = device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_TRACE_RT_CONFIG_CMD;
  cmd_.command_info.cmd_hdr.size = sizeof(cmd_);
  cmd_.command_info.cmd_hdr.flags = flag;
  cmd_.shire_mask = shire_mask;
  cmd_.thread_mask = thread_mask;
  cmd_.event_mask = event_mask;
  cmd_.filter_mask = filter_mask;
}

TraceRtConfigCmd::~TraceRtConfigCmd() {
  deleteRspEntry(cmd_.command_info.cmd_hdr.tag_id);
}

std::byte* TraceRtConfigCmd::getCmdPtr() {
  return templ::bit_cast<std::byte*>(&cmd_);
}

IDevOpsApiCmd::CmdType TraceRtConfigCmd::whoAmI() {
  return CmdType::TRACE_CONFIG_CMD;
}

size_t TraceRtConfigCmd::getCmdSize() {
  return sizeof(cmd_);
}

device_ops_api::tag_id_t TraceRtConfigCmd::getCmdTagId() {
  return cmd_.command_info.cmd_hdr.tag_id;
}

/*
 * Device Ops Api trace control command creation and handling
 */

std::unique_ptr<IDevOpsApiCmd>
IDevOpsApiCmd::createTraceRtControlCmd(device_ops_api::cmd_flags_e flag, uint32_t rt_type, uint32_t control,
                                       device_ops_api::dev_ops_trace_rt_control_response_e status) {
  auto tagId = tagId_++;
  if (!addRspEntry(tagId, status)) {
    return nullptr;
  }
  return std::make_unique<TraceRtControlCmd>(tagId, flag, rt_type, control);
}

TraceRtControlCmd::TraceRtControlCmd(device_ops_api::tag_id_t tagId, device_ops_api::cmd_flags_e flag, uint32_t rt_type,
                                     uint32_t control) {

  cmd_.command_info.cmd_hdr.tag_id = tagId;
  cmd_.command_info.cmd_hdr.msg_id = device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_TRACE_RT_CONTROL_CMD;
  cmd_.command_info.cmd_hdr.size = sizeof(cmd_);
  cmd_.command_info.cmd_hdr.flags = flag;
  cmd_.rt_type = rt_type;
  cmd_.control = control;
}

TraceRtControlCmd::~TraceRtControlCmd() {
  deleteRspEntry(cmd_.command_info.cmd_hdr.tag_id);
}

std::byte* TraceRtControlCmd::getCmdPtr() {
  return templ::bit_cast<std::byte*>(&cmd_);
}

IDevOpsApiCmd::CmdType TraceRtControlCmd::whoAmI() {
  return CmdType::TRACE_CONTROL_CMD;
}

size_t TraceRtControlCmd::getCmdSize() {
  return sizeof(cmd_);
}

device_ops_api::tag_id_t TraceRtControlCmd::getCmdTagId() {
  return cmd_.command_info.cmd_hdr.tag_id;
}

/*
 * Device Ops Api Custom command creation and handling. Custom command can be any of device Ops API
 * commands.
 */
std::unique_ptr<IDevOpsApiCmd> IDevOpsApiCmd::createCustomCmd(std::byte* cmdPtr, size_t cmdSize, uint32_t status) {
  if (cmdSize < sizeof(device_ops_api::cmd_header_t)) {
    return nullptr;
  }

  const auto* customCmd = templ::bit_cast<device_ops_api::cmd_header_t*>(cmdPtr);
  if (!addRspEntry(customCmd->cmd_hdr.tag_id, status)) {
    return nullptr;
  }

  return std::make_unique<CustomCmd>(cmdPtr, cmdSize);
}

CustomCmd::CustomCmd(std::byte* cmdPtr, size_t cmdSize) {
  cmd_ = cmdPtr;
  auto* customCmd = templ::bit_cast<device_ops_api::cmd_header_t*>(cmd_);
  customCmd->cmd_hdr.size = cmdSize;

  TEST_VLOG(1) << "Created Custom Command (tagId: " << std::hex << customCmd->cmd_hdr.tag_id << ")";
}

CustomCmd::~CustomCmd() {
  const auto* customCmd = templ::bit_cast<device_ops_api::cmd_header_t*>(cmd_);
  deleteRspEntry(customCmd->cmd_hdr.tag_id);
}

IDevOpsApiCmd::CmdType CustomCmd::whoAmI() {
  return CmdType::CUSTOM_CMD;
}

std::byte* CustomCmd::getCmdPtr() {
  return cmd_;
}

size_t CustomCmd::getCmdSize() {
  const auto* customCmd = templ::bit_cast<device_ops_api::cmd_header_t*>(cmd_);
  return customCmd->cmd_hdr.size;
}

device_ops_api::tag_id_t CustomCmd::getCmdTagId() {
  const auto* customCmd = templ::bit_cast<device_ops_api::cmd_header_t*>(cmd_);
  return customCmd->cmd_hdr.tag_id;
}
