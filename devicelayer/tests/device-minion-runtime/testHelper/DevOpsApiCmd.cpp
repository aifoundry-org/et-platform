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
#include <bitset>
#include <iostream>

using namespace dev::dl_tests;

std::unordered_map<CmdTag, std::unique_ptr<IDevOpsApiCmd>> IDevOpsApiCmd::cmds_;
std::atomic<CmdTag> IDevOpsApiCmd::tagId_ = 0x1;

IDevOpsApiCmd* IDevOpsApiCmd::getDevOpsApiCmd(CmdTag tagId) {
  if (tagId <= cmds_.size()) {
    return cmds_[tagId].get();
  }
  return nullptr;
}

CmdTag IDevOpsApiCmd::cloneDevOpsApiCmd(CmdTag origCmdTagId) {
  auto devOpsApiCmd = getDevOpsApiCmd(origCmdTagId);
  if (!devOpsApiCmd) {
    throw Exception("Command with tagId: " + std::to_string(origCmdTagId) + " does not exist!");
  }
  auto tagId = tagId_++;
  switch (devOpsApiCmd->whoAmI()) {
  case CmdType::ECHO_CMD:
    cmds_.emplace(tagId, std::make_unique<EchoCmd>(tagId, static_cast<const EchoCmd*>(devOpsApiCmd)));
    break;
  case CmdType::API_COMPATIBILITY_CMD:
    cmds_.emplace(tagId,
                  std::make_unique<ApiCompatibilityCmd>(tagId, static_cast<const ApiCompatibilityCmd*>(devOpsApiCmd)));
    break;
  case CmdType::FW_VERSION_CMD:
    cmds_.emplace(tagId, std::make_unique<FwVersionCmd>(tagId, static_cast<const FwVersionCmd*>(devOpsApiCmd)));
    break;
  case CmdType::DATA_WRITE_CMD:
    cmds_.emplace(tagId, std::make_unique<DataWriteCmd>(tagId, static_cast<const DataWriteCmd*>(devOpsApiCmd)));
    break;
  case CmdType::DATA_READ_CMD:
    cmds_.emplace(tagId, std::make_unique<DataReadCmd>(tagId, static_cast<const DataReadCmd*>(devOpsApiCmd)));
    break;
  case CmdType::DMA_WRITELIST_CMD:
    cmds_.emplace(tagId, std::make_unique<DmaWriteListCmd>(tagId, static_cast<const DmaWriteListCmd*>(devOpsApiCmd)));
    break;
  case CmdType::DMA_READLIST_CMD:
    cmds_.emplace(tagId, std::make_unique<DmaReadListCmd>(tagId, static_cast<const DmaReadListCmd*>(devOpsApiCmd)));
    break;
  case CmdType::KERNEL_LAUNCH_CMD:
    cmds_.emplace(tagId, std::make_unique<KernelLaunchCmd>(tagId, static_cast<const KernelLaunchCmd*>(devOpsApiCmd)));
    break;
  case CmdType::KERNEL_ABORT_CMD:
    cmds_.emplace(tagId, std::make_unique<KernelAbortCmd>(tagId, static_cast<const KernelAbortCmd*>(devOpsApiCmd)));
    break;
  case CmdType::TRACE_CONFIG_CMD:
    cmds_.emplace(tagId, std::make_unique<TraceRtConfigCmd>(tagId, static_cast<const TraceRtConfigCmd*>(devOpsApiCmd)));
    break;
  case CmdType::TRACE_CONTROL_CMD:
    cmds_.emplace(tagId,
                  std::make_unique<TraceRtControlCmd>(tagId, static_cast<const TraceRtControlCmd*>(devOpsApiCmd)));
    break;
  case CmdType::CUSTOM_CMD:
    cmds_.emplace(tagId, std::make_unique<CustomCmd>(tagId, static_cast<const CustomCmd*>(devOpsApiCmd)));
    break;
  default:
    throw Exception("Unknown command type!");
  }
  return tagId;
}

void IDevOpsApiCmd::deleteDevOpsApiCmds() {
  cmds_.clear();
  tagId_ = 0x1;
}

/*
 * Device Ops Api Echo Command
 */
EchoCmd::EchoCmd(CmdTag tagId, const std::tuple<device_ops_api::cmd_flags_e /*flags*/>& args) {
  cmd_.command_info.cmd_hdr.tag_id = tagId;
  cmd_.command_info.cmd_hdr.msg_id = device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_ECHO_CMD;
  cmd_.command_info.cmd_hdr.size = sizeof(cmd_);
  cmd_.command_info.cmd_hdr.flags = std::get<0>(args);
}

EchoCmd::EchoCmd(CmdTag tagId, const EchoCmd* orig) {
  memcpy(static_cast<void*>(&cmd_), static_cast<const void*>(&orig->cmd_), sizeof(cmd_));
  cmd_.command_info.cmd_hdr.tag_id = tagId;
}

std::byte* EchoCmd::getCmdPtr() {
  return templ::bit_cast<std::byte*>(&cmd_);
}

size_t EchoCmd::getCmdSize() {
  return sizeof(cmd_);
}

CmdTag EchoCmd::getCmdTagId() {
  return cmd_.command_info.cmd_hdr.tag_id;
}

CmdType EchoCmd::whoAmI() {
  return CmdType::ECHO_CMD;
}

CmdStatus EchoCmd::setRsp(const std::vector<std::byte>& rsp) {
  if (cmdStatus_ != CmdStatus::CMD_RSP_NOT_RECEIVED) {
    return cmdStatus_ = CmdStatus::CMD_RSP_DUPLICATE;
  }
  if (rsp.size() < sizeof(rsp_)) {
    return cmdStatus_;
  }

  memcpy(static_cast<void*>(&rsp_), rsp.data(), sizeof(rsp_));

  if (rsp_.response_info.rsp_hdr.tag_id != cmd_.command_info.cmd_hdr.tag_id ||
      rsp_.response_info.rsp_hdr.msg_id != device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_ECHO_RSP) {
    return cmdStatus_;
  }

  return cmdStatus_ = CmdStatus::CMD_SUCCESSFUL;
}

std::string EchoCmd::printSummary() {
  std::stringstream summary;
  summary << "\n"
          << whoAmI() << " (tagId: " << getCmdTagId() << ")\n\t" << cmdStatus_ << "\n\tCmd Flags: 0b"
          << std::bitset<sizeof(cmd_.command_info.cmd_hdr.flags) * 8>(cmd_.command_info.cmd_hdr.flags) << std::endl;
  if (cmdStatus_ != CmdStatus::CMD_RSP_NOT_RECEIVED) {
    summary << "Response\n\tdevice_cmd_start_ts: " << rsp_.device_cmd_start_ts << std::endl;
  }
  return summary.str();
}

/*
 * Device Ops Api compatibility command
 */
ApiCompatibilityCmd::ApiCompatibilityCmd(CmdTag tagId,
                                         const std::tuple<device_ops_api::cmd_flags_e /*flags*/, uint8_t /*major*/,
                                                          uint8_t /*minor*/, uint8_t /*patch*/>& args) {
  const auto& [flags, major, minor, patch] = args;
  cmd_.command_info.cmd_hdr.tag_id = tagId;
  cmd_.command_info.cmd_hdr.msg_id = device_ops_api::DEV_OPS_API_MID_CHECK_DEVICE_OPS_API_COMPATIBILITY_CMD;
  cmd_.command_info.cmd_hdr.size = sizeof(cmd_);
  cmd_.command_info.cmd_hdr.flags = flags;
  cmd_.major = major;
  cmd_.minor = minor;
  cmd_.patch = patch;
}

ApiCompatibilityCmd::ApiCompatibilityCmd(CmdTag tagId, const ApiCompatibilityCmd* orig) {
  memcpy(static_cast<void*>(&cmd_), static_cast<const void*>(&orig->cmd_), sizeof(cmd_));
  cmd_.command_info.cmd_hdr.tag_id = tagId;
}

std::byte* ApiCompatibilityCmd::getCmdPtr() {
  return templ::bit_cast<std::byte*>(&cmd_);
}

size_t ApiCompatibilityCmd::getCmdSize() {
  return sizeof(cmd_);
}

CmdTag ApiCompatibilityCmd::getCmdTagId() {
  return cmd_.command_info.cmd_hdr.tag_id;
}

CmdType ApiCompatibilityCmd::whoAmI() {
  return CmdType::API_COMPATIBILITY_CMD;
}

CmdStatus ApiCompatibilityCmd::setRsp(const std::vector<std::byte>& rsp) {
  if (cmdStatus_ != CmdStatus::CMD_RSP_NOT_RECEIVED) {
    return cmdStatus_ = CmdStatus::CMD_RSP_DUPLICATE;
  }
  if (rsp.size() < sizeof(rsp_)) {
    return cmdStatus_;
  }

  memcpy(static_cast<void*>(&rsp_), rsp.data(), sizeof(rsp_));

  if (rsp_.response_info.rsp_hdr.tag_id != cmd_.command_info.cmd_hdr.tag_id ||
      rsp_.response_info.rsp_hdr.msg_id != device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_API_COMPATIBILITY_RSP) {
    return cmdStatus_;
  }

  return cmdStatus_ = CmdStatus::CMD_SUCCESSFUL;
}

std::string ApiCompatibilityCmd::printSummary() {
  std::stringstream summary;
  summary << "\n"
          << whoAmI() << " (tagId: " << getCmdTagId() << ")\n\t" << cmdStatus_ << "\n\tCmd Flags: 0b"
          << std::bitset<sizeof(cmd_.command_info.cmd_hdr.flags) * 8>(cmd_.command_info.cmd_hdr.flags)
          << "\n\tmajor: " << cmd_.major << "\n\tminor: " << cmd_.minor << "patch: " << cmd_.patch << std::endl;
  if (cmdStatus_ != CmdStatus::CMD_RSP_NOT_RECEIVED) {
    summary << "Response\n\tmajor: " << rsp_.major << "\n\tminor: " << rsp_.minor << "\n\tpatch: " << rsp_.patch
            << std::endl;
  }
  return summary.str();
}

/*
 * Device Ops Api Firmware Version command
 */
FwVersionCmd::FwVersionCmd(CmdTag tagId,
                           const std::tuple<device_ops_api::cmd_flags_e /*flags*/, uint8_t /*firmwareType*/>& args) {
  const auto& [flags, firmwareType] = args;
  cmd_.command_info.cmd_hdr.tag_id = tagId;
  cmd_.command_info.cmd_hdr.msg_id = device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_DEVICE_FW_VERSION_CMD;
  cmd_.command_info.cmd_hdr.size = sizeof(cmd_);
  cmd_.command_info.cmd_hdr.flags = flags;
  cmd_.firmware_type = firmwareType;
}

FwVersionCmd::FwVersionCmd(CmdTag tagId, const FwVersionCmd* orig) {
  memcpy(static_cast<void*>(&cmd_), static_cast<const void*>(&orig->cmd_), sizeof(cmd_));
  cmd_.command_info.cmd_hdr.tag_id = tagId;
}

std::byte* FwVersionCmd::getCmdPtr() {
  return templ::bit_cast<std::byte*>(&cmd_);
}

size_t FwVersionCmd::getCmdSize() {
  return sizeof(cmd_);
}

CmdTag FwVersionCmd::getCmdTagId() {
  return cmd_.command_info.cmd_hdr.tag_id;
}

CmdType FwVersionCmd::whoAmI() {
  return CmdType::FW_VERSION_CMD;
}

CmdStatus FwVersionCmd::setRsp(const std::vector<std::byte>& rsp) {
  if (cmdStatus_ != CmdStatus::CMD_RSP_NOT_RECEIVED) {
    return cmdStatus_ = CmdStatus::CMD_RSP_DUPLICATE;
  }
  if (rsp.size() < sizeof(rsp_)) {
    return cmdStatus_;
  }

  memcpy(static_cast<void*>(&rsp_), rsp.data(), sizeof(rsp_));

  if (rsp_.response_info.rsp_hdr.tag_id != cmd_.command_info.cmd_hdr.tag_id ||
      rsp_.response_info.rsp_hdr.msg_id != device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_FW_VERSION_RSP) {
    return cmdStatus_;
  }

  return cmdStatus_ = CmdStatus::CMD_SUCCESSFUL;
}

std::string FwVersionCmd::printSummary() {
  std::stringstream summary;
  summary << "\n"
          << whoAmI() << " (tagId: " << getCmdTagId() << ")\n\t" << cmdStatus_ << "\n\tCmd Flags: 0b"
          << std::bitset<sizeof(cmd_.command_info.cmd_hdr.flags) * 8>(cmd_.command_info.cmd_hdr.flags)
          << "\n\tfirmware_type: " << cmd_.firmware_type << std::endl;
  summary << "\n" << whoAmI() << " (tagId: " << getCmdTagId() << ")\n\t" << cmdStatus_ << std::endl;
  if (cmdStatus_ != CmdStatus::CMD_RSP_NOT_RECEIVED) {
    summary << "Response\n\tmajor: " << rsp_.major << "\n\tminor: " << rsp_.minor << "\n\tpatch: " << rsp_.patch
            << "\n\tfirmware_type: " << rsp_.type << std::endl;
  }
  return summary.str();
}

/*
 * Device Ops Api Data Write command
 */
DataWriteCmd::DataWriteCmd(CmdTag tagId,
                           const std::tuple<device_ops_api::cmd_flags_e /*flags*/, uint64_t /*devPhysAddr*/,
                                            uint64_t /*hostVirtAddr*/, uint64_t /*dataSize*/,
                                            device_ops_api::dev_ops_api_dma_response_e /*expStatus*/>& args) {
  const auto& [flags, devPhysAddr, hostVirtAddr, dataSize, expStatus] = args;
  cmd_.command_info.cmd_hdr.tag_id = tagId;
  cmd_.command_info.cmd_hdr.msg_id = device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_DATA_WRITE_CMD;
  cmd_.command_info.cmd_hdr.size = sizeof(cmd_);
  cmd_.command_info.cmd_hdr.flags = flags;
  cmd_.dst_device_phy_addr = devPhysAddr;
  cmd_.src_host_virt_addr = hostVirtAddr;
  cmd_.size = dataSize;
  expStatus_ = expStatus;
}

DataWriteCmd::DataWriteCmd(CmdTag tagId, const DataWriteCmd* orig) {
  memcpy(static_cast<void*>(&cmd_), static_cast<const void*>(&orig->cmd_), sizeof(cmd_));
  cmd_.command_info.cmd_hdr.tag_id = tagId;
}

std::byte* DataWriteCmd::getCmdPtr() {
  return templ::bit_cast<std::byte*>(&cmd_);
}

size_t DataWriteCmd::getCmdSize() {
  return sizeof(cmd_);
}

CmdTag DataWriteCmd::getCmdTagId() {
  return cmd_.command_info.cmd_hdr.tag_id;
}

CmdType DataWriteCmd::whoAmI() {
  return CmdType::DATA_WRITE_CMD;
}

CmdStatus DataWriteCmd::setRsp(const std::vector<std::byte>& rsp) {
  if (cmdStatus_ != CmdStatus::CMD_RSP_NOT_RECEIVED) {
    return cmdStatus_ = CmdStatus::CMD_RSP_DUPLICATE;
  }

  if (rsp.size() < sizeof(rsp_)) {
    return cmdStatus_;
  }

  memcpy(static_cast<void*>(&rsp_), rsp.data(), sizeof(rsp_));

  if (rsp_.response_info.rsp_hdr.tag_id != cmd_.command_info.cmd_hdr.tag_id ||
      rsp_.response_info.rsp_hdr.msg_id != device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_DATA_WRITE_RSP) {
    return cmdStatus_;
  }

  if (rsp_.status == expStatus_) {
    cmdStatus_ = CmdStatus::CMD_SUCCESSFUL;
  } else if (rsp_.status == device_ops_api::DEV_OPS_API_DMA_RESPONSE_TIMEOUT_IDLE_CHANNEL_UNAVAILABLE) {
    cmdStatus_ = CmdStatus::CMD_TIMED_OUT;
  } else {
    cmdStatus_ = CmdStatus::CMD_FAILED;
  }

  return cmdStatus_;
}

uint32_t DataWriteCmd::getRspStatusCode() const {
  if (cmdStatus_ == CmdStatus::CMD_RSP_NOT_RECEIVED) {
    throw Exception("No response received!");
  }
  return rsp_.status;
}

std::string DataWriteCmd::printSummary() {
  std::stringstream summary;
  summary << "\n"
          << whoAmI() << " (tagId: " << getCmdTagId() << ")\n\t" << cmdStatus_ << "\n\tCmd Flags: 0b"
          << std::bitset<sizeof(cmd_.command_info.cmd_hdr.flags) * 8>(cmd_.command_info.cmd_hdr.flags)
          << "\n\tsrc_host_virt_addr: 0x" << std::hex << cmd_.src_host_virt_addr
          << "\n\tsize: " << std::dec << cmd_.size << std::endl;
  if (cmdStatus_ != CmdStatus::CMD_RSP_NOT_RECEIVED) {
    summary << "Response\n\tdevice_cmd_start_ts: " << rsp_.device_cmd_start_ts
            << "\n\tdevice_cmd_wait_dur: " << rsp_.device_cmd_wait_dur
            << "\n\tdevice_cmd_execute_dur: " << rsp_.device_cmd_execute_dur << "\n\tstatus code: " << rsp_.status
            << "\n\texpected status code: " << expStatus_ << std::endl;
  }

  return summary.str();
}

/*
 * Device Ops Api Data Read command
 */
DataReadCmd::DataReadCmd(CmdTag tagId,
                         const std::tuple<device_ops_api::cmd_flags_e /*flags*/, uint64_t /*devPhysAddr*/,
                                          uint64_t /*hostVirtAddr*/, uint64_t /*dataSize*/,
                                          device_ops_api::dev_ops_api_dma_response_e /*expStatus*/>& args) {
  const auto& [flags, devPhysAddr, hostVirtAddr, dataSize, expStatus] = args;
  cmd_.command_info.cmd_hdr.tag_id = tagId;
  cmd_.command_info.cmd_hdr.msg_id = device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_DATA_READ_CMD;
  cmd_.command_info.cmd_hdr.size = sizeof(cmd_);
  cmd_.command_info.cmd_hdr.flags = flags;
  cmd_.src_device_phy_addr = devPhysAddr;
  cmd_.dst_host_virt_addr = hostVirtAddr;
  cmd_.size = dataSize;
  expStatus_ = expStatus;
}

DataReadCmd::DataReadCmd(CmdTag tagId, const DataReadCmd* orig) {
  memcpy(static_cast<void*>(&cmd_), static_cast<const void*>(&orig->cmd_), sizeof(cmd_));
  cmd_.command_info.cmd_hdr.tag_id = tagId;
}

std::byte* DataReadCmd::getCmdPtr() {
  return templ::bit_cast<std::byte*>(&cmd_);
}

size_t DataReadCmd::getCmdSize() {
  return sizeof(cmd_);
}

CmdTag DataReadCmd::getCmdTagId() {
  return cmd_.command_info.cmd_hdr.tag_id;
}

CmdType DataReadCmd::whoAmI() {
  return CmdType::DATA_READ_CMD;
}

CmdStatus DataReadCmd::setRsp(const std::vector<std::byte>& rsp) {
  if (cmdStatus_ != CmdStatus::CMD_RSP_NOT_RECEIVED) {
    return cmdStatus_ = CmdStatus::CMD_RSP_DUPLICATE;
  }

  if (rsp.size() < sizeof(rsp_)) {
    return cmdStatus_;
  }

  memcpy(static_cast<void*>(&rsp_), rsp.data(), sizeof(rsp_));

  if (rsp_.response_info.rsp_hdr.tag_id != cmd_.command_info.cmd_hdr.tag_id ||
      rsp_.response_info.rsp_hdr.msg_id != device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_DATA_READ_RSP) {
    return cmdStatus_;
  }

  if (rsp_.status == expStatus_) {
    cmdStatus_ = CmdStatus::CMD_SUCCESSFUL;
  } else if (rsp_.status == device_ops_api::DEV_OPS_API_DMA_RESPONSE_TIMEOUT_IDLE_CHANNEL_UNAVAILABLE) {
    cmdStatus_ = CmdStatus::CMD_TIMED_OUT;
  } else {
    cmdStatus_ = CmdStatus::CMD_FAILED;
  }

  return cmdStatus_;
}

uint32_t DataReadCmd::getRspStatusCode() const {
  if (cmdStatus_ == CmdStatus::CMD_RSP_NOT_RECEIVED) {
    throw Exception("No response received!");
  }
  return rsp_.status;
}

std::string DataReadCmd::printSummary() {
  std::stringstream summary;
  summary << "\n"
          << whoAmI() << " (tagId: " << getCmdTagId() << ")\n\t" << cmdStatus_ << "\n\tCmd Flags: 0b"
          << std::bitset<sizeof(cmd_.command_info.cmd_hdr.flags) * 8>(cmd_.command_info.cmd_hdr.flags)
          << "\n\tdst_host_virt_addr: 0x" << std::hex << cmd_.dst_host_virt_addr
          << "\n\tsize: " << std::dec << cmd_.size << std::endl;
  if (cmdStatus_ != CmdStatus::CMD_RSP_NOT_RECEIVED) {
    summary << "Response\n\tdevice_cmd_start_ts: " << rsp_.device_cmd_start_ts
            << "\n\tdevice_cmd_wait_dur: " << rsp_.device_cmd_wait_dur
            << "\n\tdevice_cmd_execute_dur: " << rsp_.device_cmd_execute_dur << "\n\tstatus code: " << rsp_.status
            << "\n\texpected status code: " << expStatus_ << std::endl;
  }

  return summary.str();
}

/*
 * Device Ops Api DMA Writelist command
 */
DmaWriteListCmd::DmaWriteListCmd(
  CmdTag tagId,
  const std::tuple<device_ops_api::cmd_flags_e /*flags*/, const device_ops_api::dma_write_node* /*list*/,
                   uint32_t /*nodeCount*/, device_ops_api::dev_ops_api_dma_response_e /*expStatus*/>& args) {
  const auto& [flags, list, nodeCount, expStatus] = args;
  cmdMem_.resize(sizeof(*cmd_) + sizeof(list[0]) * nodeCount);
  cmd_ = templ::bit_cast<device_ops_api::device_ops_dma_writelist_cmd_t*>(cmdMem_.data());
  cmd_->command_info.cmd_hdr.tag_id = tagId;
  cmd_->command_info.cmd_hdr.msg_id = device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_DMA_WRITELIST_CMD;
  cmd_->command_info.cmd_hdr.size = cmdMem_.size();
  cmd_->command_info.cmd_hdr.flags = flags;
  memcpy(cmd_->list, list, sizeof(list[0]) * nodeCount);
  expStatus_ = expStatus;
}

DmaWriteListCmd::DmaWriteListCmd(CmdTag tagId, const DmaWriteListCmd* orig) {
  cmdMem_.resize(orig->cmdMem_.size());
  cmd_ = templ::bit_cast<device_ops_api::device_ops_dma_writelist_cmd_t*>(cmdMem_.data());
  memcpy(static_cast<void*>(cmd_), static_cast<const void*>(orig->cmd_), cmdMem_.size());
  cmd_->command_info.cmd_hdr.tag_id = tagId;
}

std::byte* DmaWriteListCmd::getCmdPtr() {
  return templ::bit_cast<std::byte*>(cmd_);
}

size_t DmaWriteListCmd::getCmdSize() {
  return cmd_->command_info.cmd_hdr.size;
}

CmdTag DmaWriteListCmd::getCmdTagId() {
  return cmd_->command_info.cmd_hdr.tag_id;
}

CmdType DmaWriteListCmd::whoAmI() {
  return CmdType::DMA_WRITELIST_CMD;
}

CmdStatus DmaWriteListCmd::setRsp(const std::vector<std::byte>& rsp) {
  if (cmdStatus_ != CmdStatus::CMD_RSP_NOT_RECEIVED) {
    return cmdStatus_ = CmdStatus::CMD_RSP_DUPLICATE;
  }
  if (rsp.size() < sizeof(rsp_)) {
    return cmdStatus_;
  }

  memcpy(static_cast<void*>(&rsp_), rsp.data(), sizeof(rsp_));

  if (rsp_.response_info.rsp_hdr.tag_id != cmd_->command_info.cmd_hdr.tag_id ||
      rsp_.response_info.rsp_hdr.msg_id != device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_DMA_WRITELIST_RSP) {
    return cmdStatus_;
  }

  if (rsp_.status == expStatus_) {
    cmdStatus_ = CmdStatus::CMD_SUCCESSFUL;
  } else if (rsp_.status == device_ops_api::DEV_OPS_API_DMA_RESPONSE_TIMEOUT_IDLE_CHANNEL_UNAVAILABLE) {
    cmdStatus_ = CmdStatus::CMD_TIMED_OUT;
  } else {
    cmdStatus_ = CmdStatus::CMD_FAILED;
  }

  return cmdStatus_;
}

uint32_t DmaWriteListCmd::getRspStatusCode() const {
  if (cmdStatus_ == CmdStatus::CMD_RSP_NOT_RECEIVED) {
    throw Exception("No response received!");
  }
  return rsp_.status;
}

std::string DmaWriteListCmd::printSummary() {
  std::stringstream summary;
  summary << "\n"
          << whoAmI() << " (tagId: " << getCmdTagId() << ")\n\t" << cmdStatus_ << "\n\tCmd Flags: 0b"
          << std::bitset<sizeof(cmd_->command_info.cmd_hdr.flags) * 8>(cmd_->command_info.cmd_hdr.flags) << std::endl;
  auto nodeCount = (cmd_->command_info.cmd_hdr.size - sizeof(*cmd_)) / sizeof(cmd_->list[0]);
  for (int i = 0; i < nodeCount; i++) {
    summary << "\tnode: " << i << "\n\t\tsrc_host_virt_addr: 0x" << std::hex << cmd_->list[i].src_host_virt_addr
            << "\n\t\tdst_device_phy_addr: 0x" << cmd_->list[i].dst_device_phy_addr << "\n\t\tsize: " << std::dec
            << cmd_->list[i].size << std::endl;
  }
  if (cmdStatus_ != CmdStatus::CMD_RSP_NOT_RECEIVED) {
    summary << "Response\n\tdevice_cmd_start_ts: " << rsp_.device_cmd_start_ts
            << "\n\tdevice_cmd_wait_dur: " << rsp_.device_cmd_wait_dur
            << "\n\tdevice_cmd_execute_dur: " << rsp_.device_cmd_execute_dur << "\n\tstatus code: " << rsp_.status
            << "\n\texpected status code: " << expStatus_ << std::endl;
  }

  return summary.str();
}

/*
 * Device Ops Api DMA Readlist command
 */
DmaReadListCmd::DmaReadListCmd(
  CmdTag tagId,
  const std::tuple<device_ops_api::cmd_flags_e /*flags*/, const device_ops_api::dma_read_node* /*list*/,
                   uint32_t /*nodeCount*/, device_ops_api::dev_ops_api_dma_response_e /*expStatus*/>& args) {
  const auto& [flags, list, nodeCount, expStatus] = args;
  cmdMem_.resize(sizeof(*cmd_) + sizeof(list[0]) * nodeCount);
  cmd_ = templ::bit_cast<device_ops_api::device_ops_dma_readlist_cmd_t*>(cmdMem_.data());
  cmd_->command_info.cmd_hdr.tag_id = tagId;
  cmd_->command_info.cmd_hdr.msg_id = device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_DMA_READLIST_CMD;
  cmd_->command_info.cmd_hdr.size = cmdMem_.size();
  cmd_->command_info.cmd_hdr.flags = flags;
  memcpy(cmd_->list, list, sizeof(list[0]) * nodeCount);
  expStatus_ = expStatus;
}

DmaReadListCmd::DmaReadListCmd(CmdTag tagId, const DmaReadListCmd* orig) {
  cmdMem_.resize(orig->cmdMem_.size());
  cmd_ = templ::bit_cast<device_ops_api::device_ops_dma_readlist_cmd_t*>(cmdMem_.data());
  memcpy(static_cast<void*>(cmd_), static_cast<const void*>(orig->cmd_), cmdMem_.size());
  cmd_->command_info.cmd_hdr.tag_id = tagId;
}

std::byte* DmaReadListCmd::getCmdPtr() {
  return templ::bit_cast<std::byte*>(cmd_);
}

size_t DmaReadListCmd::getCmdSize() {
  return cmd_->command_info.cmd_hdr.size;
}

CmdTag DmaReadListCmd::getCmdTagId() {
  return cmd_->command_info.cmd_hdr.tag_id;
}

CmdType DmaReadListCmd::whoAmI() {
  return CmdType::DMA_READLIST_CMD;
}

CmdStatus DmaReadListCmd::setRsp(const std::vector<std::byte>& rsp) {
  if (cmdStatus_ != CmdStatus::CMD_RSP_NOT_RECEIVED) {
    return cmdStatus_ = CmdStatus::CMD_RSP_DUPLICATE;
  }
  if (rsp.size() < sizeof(rsp_)) {
    return cmdStatus_;
  }

  memcpy(static_cast<void*>(&rsp_), rsp.data(), sizeof(rsp_));

  if (rsp_.response_info.rsp_hdr.tag_id != cmd_->command_info.cmd_hdr.tag_id ||
      rsp_.response_info.rsp_hdr.msg_id != device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_DMA_READLIST_RSP) {
    return cmdStatus_;
  }

  if (rsp_.status == expStatus_) {
    cmdStatus_ = CmdStatus::CMD_SUCCESSFUL;
  } else if (rsp_.status == device_ops_api::DEV_OPS_API_DMA_RESPONSE_TIMEOUT_IDLE_CHANNEL_UNAVAILABLE) {
    cmdStatus_ = CmdStatus::CMD_TIMED_OUT;
  } else {
    cmdStatus_ = CmdStatus::CMD_FAILED;
  }

  return cmdStatus_;
}

uint32_t DmaReadListCmd::getRspStatusCode() const {
  if (cmdStatus_ == CmdStatus::CMD_RSP_NOT_RECEIVED) {
    throw Exception("No response received!");
  }
  return rsp_.status;
}

std::string DmaReadListCmd::printSummary() {
  std::stringstream summary;
  summary << "\n"
          << whoAmI() << " (tagId: " << getCmdTagId() << ")\n\t" << cmdStatus_ << "\n\tCmd Flags: 0b"
          << std::bitset<sizeof(cmd_->command_info.cmd_hdr.flags) * 8>(cmd_->command_info.cmd_hdr.flags) << std::endl;
  auto nodeCount = (cmd_->command_info.cmd_hdr.size - sizeof(*cmd_)) / sizeof(cmd_->list[0]);
  for (int i = 0; i < nodeCount; i++) {
    summary << "\tnode: " << i << "\n\t\tdst_host_virt_addr: 0x" << std::hex << cmd_->list[i].dst_host_virt_addr
            << "\n\t\tsrc_device_phy_addr: 0x" << cmd_->list[i].src_device_phy_addr << "\n\t\tsize: " << std::dec
            << cmd_->list[i].size << std::endl;
  }
  if (cmdStatus_ != CmdStatus::CMD_RSP_NOT_RECEIVED) {
    summary << "Response\n\tdevice_cmd_start_ts: " << rsp_.device_cmd_start_ts
            << "\n\tdevice_cmd_wait_dur: " << rsp_.device_cmd_wait_dur
            << "\n\tdevice_cmd_execute_dur: " << rsp_.device_cmd_execute_dur << "\n\tstatus code: " << rsp_.status
            << "\n\texpected status code: " << expStatus_ << std::endl;
  }

  return summary.str();
}

/*
 * Device Ops Api Kernel Launch command
 */
KernelLaunchCmd::KernelLaunchCmd(
  CmdTag tagId,
  const std::tuple<device_ops_api::cmd_flags_e /*flags*/, uint64_t /*codeStartAddr*/, uint64_t /*ptrToArgs*/,
                   uint64_t /*exceptionBuffer*/, uint64_t /*shireMask*/, uint64_t /*traceBuffer*/,
                   const uint64_t* /*argumentPayload*/, uint32_t /*sizeOfArgPayload*/, std::string /*kernelName*/,
                   device_ops_api::dev_ops_api_kernel_launch_response_e /*expStatus*/>& args) {
  const auto& [flags, codeStartAddr, ptrToArgs, exceptionBuffer, shireMask, traceBuffer, argumentPayload,
               sizeOfArgPayload, kernelName, expStatus] = args;
  cmdMem_.resize(sizeof(*cmd_) + sizeOfArgPayload);
  cmd_ = templ::bit_cast<device_ops_api::device_ops_kernel_launch_cmd_t*>(cmdMem_.data());
  cmd_->command_info.cmd_hdr.tag_id = tagId;
  cmd_->command_info.cmd_hdr.msg_id = device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_KERNEL_LAUNCH_CMD;
  cmd_->command_info.cmd_hdr.size = cmdMem_.size();
  cmd_->command_info.cmd_hdr.flags = flags;
  cmd_->code_start_address = codeStartAddr;
  cmd_->pointer_to_args = ptrToArgs;
  cmd_->exception_buffer = exceptionBuffer;
  cmd_->shire_mask = shireMask;
  cmd_->kernel_trace_buffer = traceBuffer;
  if (sizeOfArgPayload > 0) {
    memcpy(cmd_->argument_payload, argumentPayload, sizeOfArgPayload);
  }
  kernelName_ = kernelName;
  expStatus_ = expStatus;
}

KernelLaunchCmd::KernelLaunchCmd(CmdTag tagId, const KernelLaunchCmd* orig) {
  cmdMem_.resize(orig->cmdMem_.size());
  cmd_ = templ::bit_cast<device_ops_api::device_ops_kernel_launch_cmd_t*>(cmdMem_.data());
  memcpy(static_cast<void*>(cmd_), static_cast<const void*>(orig->cmd_), cmdMem_.size());
  cmd_->command_info.cmd_hdr.tag_id = tagId;
}

std::byte* KernelLaunchCmd::getCmdPtr() {
  return templ::bit_cast<std::byte*>(cmd_);
}

size_t KernelLaunchCmd::getCmdSize() {
  return cmd_->command_info.cmd_hdr.size;
}

CmdTag KernelLaunchCmd::getCmdTagId() {
  return cmd_->command_info.cmd_hdr.tag_id;
}

CmdType KernelLaunchCmd::whoAmI() {
  return CmdType::KERNEL_LAUNCH_CMD;
}

CmdStatus KernelLaunchCmd::setRsp(const std::vector<std::byte>& rsp) {
  if (cmdStatus_ != CmdStatus::CMD_RSP_NOT_RECEIVED) {
    return cmdStatus_ = CmdStatus::CMD_RSP_DUPLICATE;
  }
  if (rsp.size() < sizeof(rsp_)) {
    return cmdStatus_;
  }

  memcpy(static_cast<void*>(&rsp_), rsp.data(), sizeof(rsp_));

  if (rsp_.response_info.rsp_hdr.tag_id != cmd_->command_info.cmd_hdr.tag_id ||
      rsp_.response_info.rsp_hdr.msg_id != device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_KERNEL_LAUNCH_RSP) {
    return cmdStatus_;
  }

  if (rsp_.status == expStatus_) {
    cmdStatus_ = CmdStatus::CMD_SUCCESSFUL;
  } else if (rsp_.status == device_ops_api::DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_TIMEOUT_HANG) {
    cmdStatus_ = CmdStatus::CMD_TIMED_OUT;
  } else {
    cmdStatus_ = CmdStatus::CMD_FAILED;
  }

  return cmdStatus_;
}

uint32_t KernelLaunchCmd::getRspStatusCode() const {
  if (cmdStatus_ == CmdStatus::CMD_RSP_NOT_RECEIVED) {
    throw Exception("No response received!");
  }
  return rsp_.status;
}

std::string KernelLaunchCmd::printSummary() {
  std::stringstream summary;
  auto payloadSize = cmd_->command_info.cmd_hdr.size - sizeof(*cmd_);
  summary << "\n"
          << whoAmI() << " (tagId: " << getCmdTagId() << ")\n\t" << cmdStatus_ << "\n\tCmd Flags: 0b"
          << std::bitset<sizeof(cmd_->command_info.cmd_hdr.flags) * 8>(cmd_->command_info.cmd_hdr.flags)
          << "\n\tcode_start_address: 0x" << std::hex << cmd_->code_start_address << "\n\tpointer_to_args: 0x"
          << cmd_->pointer_to_args << "\n\texception_buffer: 0x" << cmd_->exception_buffer << "\n\tshire_mask: 0x"
          << cmd_->shire_mask << "\n\tkernel_trace_buffer: 0x" << cmd_->kernel_trace_buffer
          << "\n\tpayload size: " << std::dec << payloadSize << std::endl;
  if (cmdStatus_ != CmdStatus::CMD_RSP_NOT_RECEIVED) {
    summary << "Response\n\tdevice_cmd_start_ts: " << rsp_.device_cmd_start_ts
            << "\n\tdevice_cmd_wait_dur: " << rsp_.device_cmd_wait_dur
            << "\n\tdevice_cmd_execute_dur: " << rsp_.device_cmd_execute_dur << "\n\tstatus code: " << rsp_.status
            << "\n\texpected status code: " << expStatus_ << std::endl;
  }

  return summary.str();
}

/*
 * Device Ops Api Kernel Abort command
 */
KernelAbortCmd::KernelAbortCmd(
  CmdTag tagId, const std::tuple<device_ops_api::cmd_flags_e /*flags*/, CmdTag /*kernelToAbortTagId*/,
                                 device_ops_api::dev_ops_api_kernel_abort_response_e /*expStatus*/>& args) {
  const auto& [flags, kernelToAbortTagId, expStatus] = args;
  cmd_.command_info.cmd_hdr.tag_id = tagId;
  cmd_.command_info.cmd_hdr.msg_id = device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_KERNEL_ABORT_CMD;
  cmd_.command_info.cmd_hdr.size = sizeof(cmd_);
  cmd_.command_info.cmd_hdr.flags = flags;
  cmd_.kernel_launch_tag_id = kernelToAbortTagId;
  expStatus_ = expStatus;
}

KernelAbortCmd::KernelAbortCmd(CmdTag tagId, const KernelAbortCmd* orig) {
  memcpy(static_cast<void*>(&cmd_), static_cast<const void*>(&orig->cmd_), sizeof(cmd_));
  cmd_.command_info.cmd_hdr.tag_id = tagId;
}

std::byte* KernelAbortCmd::getCmdPtr() {
  return templ::bit_cast<std::byte*>(&cmd_);
}

size_t KernelAbortCmd::getCmdSize() {
  return sizeof(cmd_);
}

CmdTag KernelAbortCmd::getCmdTagId() {
  return cmd_.command_info.cmd_hdr.tag_id;
}

CmdType KernelAbortCmd::whoAmI() {
  return CmdType::KERNEL_ABORT_CMD;
}

CmdStatus KernelAbortCmd::setRsp(const std::vector<std::byte>& rsp) {
  if (cmdStatus_ != CmdStatus::CMD_RSP_NOT_RECEIVED) {
    return cmdStatus_ = CmdStatus::CMD_RSP_DUPLICATE;
  }
  if (rsp.size() < sizeof(rsp_)) {
    return cmdStatus_;
  }

  memcpy(static_cast<void*>(&rsp_), rsp.data(), sizeof(rsp_));

  if (rsp_.response_info.rsp_hdr.tag_id != cmd_.command_info.cmd_hdr.tag_id ||
      rsp_.response_info.rsp_hdr.msg_id != device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_KERNEL_ABORT_RSP) {
    return cmdStatus_;
  }

  if (rsp_.status == expStatus_) {
    cmdStatus_ = CmdStatus::CMD_SUCCESSFUL;
  } else {
    cmdStatus_ = CmdStatus::CMD_FAILED;
  }

  return cmdStatus_;
}

uint32_t KernelAbortCmd::getRspStatusCode() const {
  if (cmdStatus_ == CmdStatus::CMD_RSP_NOT_RECEIVED) {
    throw Exception("No response received!");
  }
  return rsp_.status;
}

std::string KernelAbortCmd::printSummary() {
  std::stringstream summary;
  summary << "\n"
          << whoAmI() << " (tagId: " << getCmdTagId() << ")\n\t" << cmdStatus_ << "\n\tCmd Flags: 0b"
          << std::bitset<sizeof(cmd_.command_info.cmd_hdr.flags) * 8>(cmd_.command_info.cmd_hdr.flags)
          << "\n\tkernel_launch_tag_id: " << cmd_.kernel_launch_tag_id << std::endl;
  if (cmdStatus_ != CmdStatus::CMD_RSP_NOT_RECEIVED) {
    summary << "Response\n\tstatus code: " << rsp_.status << "\n\texpected status code: " << expStatus_ << std::endl;
  }

  return summary.str();
}

/*
 * Device Ops Api trace configuration command
 */
TraceRtConfigCmd::TraceRtConfigCmd(
  CmdTag tagId, const std::tuple<device_ops_api::cmd_flags_e /*flags*/, uint64_t /*shireMask*/, uint64_t /*threadMask*/,
                                 uint32_t /*eventMask*/, uint32_t /*filterMask*/,
                                 device_ops_api::dev_ops_trace_rt_config_response_e /*expStatus*/>& args) {
  const auto& [flags, shireMask, threadMask, eventMask, filterMask, expStatus] = args;
  cmd_.command_info.cmd_hdr.tag_id = tagId;
  cmd_.command_info.cmd_hdr.msg_id = device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_TRACE_RT_CONFIG_CMD;
  cmd_.command_info.cmd_hdr.size = sizeof(cmd_);
  cmd_.command_info.cmd_hdr.flags = flags;
  cmd_.shire_mask = shireMask;
  cmd_.thread_mask = threadMask;
  cmd_.event_mask = eventMask;
  cmd_.filter_mask = filterMask;
  expStatus_ = expStatus;
}

TraceRtConfigCmd::TraceRtConfigCmd(CmdTag tagId, const TraceRtConfigCmd* orig) {
  memcpy(static_cast<void*>(&cmd_), static_cast<const void*>(&orig->cmd_), sizeof(cmd_));
  cmd_.command_info.cmd_hdr.tag_id = tagId;
}

std::byte* TraceRtConfigCmd::getCmdPtr() {
  return templ::bit_cast<std::byte*>(&cmd_);
}

size_t TraceRtConfigCmd::getCmdSize() {
  return sizeof(cmd_);
}

CmdTag TraceRtConfigCmd::getCmdTagId() {
  return cmd_.command_info.cmd_hdr.tag_id;
}

CmdType TraceRtConfigCmd::whoAmI() {
  return CmdType::TRACE_CONFIG_CMD;
}

CmdStatus TraceRtConfigCmd::setRsp(const std::vector<std::byte>& rsp) {
  if (cmdStatus_ != CmdStatus::CMD_RSP_NOT_RECEIVED) {
    return cmdStatus_ = CmdStatus::CMD_RSP_DUPLICATE;
  }
  if (rsp.size() < sizeof(rsp_)) {
    return cmdStatus_;
  }

  memcpy(static_cast<void*>(&rsp_), rsp.data(), sizeof(rsp_));

  if (rsp_.response_info.rsp_hdr.tag_id != cmd_.command_info.cmd_hdr.tag_id ||
      rsp_.response_info.rsp_hdr.msg_id != device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_TRACE_RT_CONFIG_RSP) {
    return cmdStatus_;
  }

  if (rsp_.status == expStatus_) {
    cmdStatus_ = CmdStatus::CMD_SUCCESSFUL;
  } else {
    cmdStatus_ = CmdStatus::CMD_FAILED;
  }

  return cmdStatus_;
}

uint32_t TraceRtConfigCmd::getRspStatusCode() const {
  if (cmdStatus_ == CmdStatus::CMD_RSP_NOT_RECEIVED) {
    throw Exception("No response received!");
  }
  return rsp_.status;
}

std::string TraceRtConfigCmd::printSummary() {
  std::stringstream summary;
  summary << "\n"
          << whoAmI() << " (tagId: " << getCmdTagId() << ")\n\t" << cmdStatus_ << "\n\tCmd Flags: 0b"
          << std::bitset<sizeof(cmd_.command_info.cmd_hdr.flags) * 8>(cmd_.command_info.cmd_hdr.flags)
          << "\n\tshire_mask: 0x" << std::hex << cmd_.shire_mask << "\n\tthread_mask: 0x" << cmd_.thread_mask
          << "\n\tevent_mask: 0x" << cmd_.event_mask << "\n\tfilter_mask: 0x" << cmd_.filter_mask << std::dec
          << std::endl;
  if (cmdStatus_ != CmdStatus::CMD_RSP_NOT_RECEIVED) {
    summary << "Response\n\tstatus code: " << rsp_.status << "\n\texpected status code: " << expStatus_ << std::endl;
  }

  return summary.str();
}

/*
 * Device Ops Api trace control command
 */
TraceRtControlCmd::TraceRtControlCmd(
  CmdTag tagId,
  const std::tuple<device_ops_api::cmd_flags_e /*flags*/, device_ops_api::trace_rt_type_e /*rtType*/,
                   uint32_t /*control*/, device_ops_api::dev_ops_trace_rt_control_response_e /*expStatus*/>& args) {
  const auto& [flags, rtType, control, expStatus] = args;
  cmd_.command_info.cmd_hdr.tag_id = tagId;
  cmd_.command_info.cmd_hdr.msg_id = device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_TRACE_RT_CONTROL_CMD;
  cmd_.command_info.cmd_hdr.size = sizeof(cmd_);
  cmd_.command_info.cmd_hdr.flags = flags;
  cmd_.rt_type = rtType;
  cmd_.control = control;
  expStatus_ = expStatus;
}

TraceRtControlCmd::TraceRtControlCmd(CmdTag tagId, const TraceRtControlCmd* orig) {
  memcpy(static_cast<void*>(&cmd_), static_cast<const void*>(&orig->cmd_), sizeof(cmd_));
  cmd_.command_info.cmd_hdr.tag_id = tagId;
}

std::byte* TraceRtControlCmd::getCmdPtr() {
  return templ::bit_cast<std::byte*>(&cmd_);
}

size_t TraceRtControlCmd::getCmdSize() {
  return sizeof(cmd_);
}

CmdTag TraceRtControlCmd::getCmdTagId() {
  return cmd_.command_info.cmd_hdr.tag_id;
}

CmdType TraceRtControlCmd::whoAmI() {
  return CmdType::TRACE_CONTROL_CMD;
}

CmdStatus TraceRtControlCmd::setRsp(const std::vector<std::byte>& rsp) {
  if (cmdStatus_ != CmdStatus::CMD_RSP_NOT_RECEIVED) {
    return cmdStatus_ = CmdStatus::CMD_RSP_DUPLICATE;
  }
  if (rsp.size() < sizeof(rsp_)) {
    return cmdStatus_;
  }

  memcpy(static_cast<void*>(&rsp_), rsp.data(), sizeof(rsp_));

  if (rsp_.response_info.rsp_hdr.tag_id != cmd_.command_info.cmd_hdr.tag_id ||
      rsp_.response_info.rsp_hdr.msg_id != device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_TRACE_RT_CONTROL_RSP) {
    return cmdStatus_;
  }

  if (rsp_.status == expStatus_) {
    cmdStatus_ = CmdStatus::CMD_SUCCESSFUL;
  } else {
    cmdStatus_ = CmdStatus::CMD_FAILED;
  }

  return cmdStatus_;
}

uint32_t TraceRtControlCmd::getRspStatusCode() const {
  if (cmdStatus_ == CmdStatus::CMD_RSP_NOT_RECEIVED) {
    throw Exception("No response received!");
  }
  return rsp_.status;
}

std::string TraceRtControlCmd::printSummary() {
  std::stringstream summary;
  summary << "\n"
          << whoAmI() << " (tagId: " << getCmdTagId() << ")\n\t" << cmdStatus_ << "\n\tCmd Flags: 0b"
          << std::bitset<sizeof(cmd_.command_info.cmd_hdr.flags) * 8>(cmd_.command_info.cmd_hdr.flags)
          << "\n\trt_type: 0x" << std::hex << cmd_.rt_type << "\n\tcontrol: 0x" << cmd_.control << std::dec
          << std::endl;
  if (cmdStatus_ != CmdStatus::CMD_RSP_NOT_RECEIVED) {
    summary << "Response\n\tstatus code: " << rsp_.status << "\n\texpected status code: " << expStatus_ << std::endl;
  }

  return summary.str();
}

/*
 * Device Ops Api Custom command.
 * Custom command can be any of device Ops API commands.
 */
CustomCmd::CustomCmd(CmdTag tagId, const std::tuple<const std::byte* /*cmdPtr*/, size_t /*cmdSize*/>& args) {
  const auto& [cmdPtr, cmdSize] = args;
  cmdMem_.resize(cmdSize);
  cmd_ = cmdMem_.data();
  memcpy(cmd_, cmdPtr, cmdSize);
  auto* customCmd = templ::bit_cast<device_ops_api::cmd_header_t*>(cmd_);
  customCmd->cmd_hdr.tag_id = tagId;
  customCmd->cmd_hdr.size = cmdSize;
}

CustomCmd::CustomCmd(CmdTag tagId, const CustomCmd* orig) {
  cmdMem_.resize(orig->cmdMem_.size());
  cmd_ = cmdMem_.data();
  memcpy(cmd_, orig->cmd_, cmdMem_.size());
  auto* customCmd = templ::bit_cast<device_ops_api::cmd_header_t*>(cmd_);
  customCmd->cmd_hdr.tag_id = tagId;
}

std::byte* CustomCmd::getCmdPtr() {
  return cmd_;
}

size_t CustomCmd::getCmdSize() {
  const auto* customCmd = templ::bit_cast<device_ops_api::cmd_header_t*>(cmd_);
  return customCmd->cmd_hdr.size;
}

CmdTag CustomCmd::getCmdTagId() {
  const auto* customCmd = templ::bit_cast<device_ops_api::cmd_header_t*>(cmd_);
  return customCmd->cmd_hdr.tag_id;
}

CmdType CustomCmd::whoAmI() {
  return CmdType::CUSTOM_CMD;
}

CmdStatus CustomCmd::setRsp(const std::vector<std::byte>& rsp) {
  if (cmdStatus_ != CmdStatus::CMD_RSP_NOT_RECEIVED) {
    return cmdStatus_ = CmdStatus::CMD_RSP_DUPLICATE;
  }

  rspMem_.resize(rsp.size());
  rsp_ = rspMem_.data();
  memcpy(rsp_, rsp.data(), rsp.size());

  if (templ::bit_cast<device_ops_api::rsp_header_t*>(rsp_)->rsp_hdr.tag_id !=
        templ::bit_cast<device_ops_api::cmd_header_t*>(cmd_)->cmd_hdr.tag_id ||
      templ::bit_cast<device_ops_api::rsp_header_t*>(rsp_)->rsp_hdr.msg_id !=
        templ::bit_cast<device_ops_api::cmd_header_t*>(cmd_)->cmd_hdr.msg_id + 1) {
    return cmdStatus_;
  }

  return cmdStatus_ = CmdStatus::CMD_SUCCESSFUL;
}

std::string CustomCmd::printSummary() {
  std::stringstream summary;
  const auto* customCmd = templ::bit_cast<device_ops_api::cmd_header_t*>(cmd_);
  summary << "\n"
          << whoAmI() << " (tagId: " << getCmdTagId() << ")\n\t" << cmdStatus_ << "\n\tCmd Flags: 0b"
          << std::bitset<sizeof(customCmd->cmd_hdr.flags) * 8>(customCmd->cmd_hdr.flags) << std::endl;
  return summary.str();
}
