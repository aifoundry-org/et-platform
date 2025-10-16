/*-------------------------------------------------------------------------
 * Copyright (c) 2025 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------*/

#include "MemcpyOps.h"
#include "CommandSender.h"
#include "RuntimeImp.h"
#include "ScopedProfileEvent.h"
#include "Utils.h"
#include "dma/CmaManager.h"
#include "dma/MemcpyContext.h"
#include "dma/MemcpyD2HAction.h"
#include "dma/MemcpyH2DAction.h"
#include "dma/MemcpyListD2HAction.h"
#include "dma/MemcpyListH2DAction.h"
#include "runtime/Types.h"
#include <algorithm>
#include <chrono>
#include <esperanto/device-apis/device_apis_message_types.h>
#include <esperanto/device-apis/operations-api/device_ops_api_cxx.h>
#include <esperanto/device-apis/operations-api/device_ops_api_rpc_types.h>
#include <g3log/loglevels.hpp>
#include <iterator>
#include <mutex>
#include <numeric>
#include <optional>
#include <thread>
#include <type_traits>
using namespace rt::profiling;
using namespace device_ops_api;
static_assert(sizeof(device_ops_dma_readlist_cmd_t) == sizeof(device_ops_dma_writelist_cmd_t));
static_assert(offsetof(device_ops_dma_readlist_cmd_t, list) == offsetof(device_ops_dma_writelist_cmd_t, list));
static_assert(sizeof(dma_write_node) == sizeof(dma_read_node));
static_assert(offsetof(dma_write_node, src_host_virt_addr) == offsetof(dma_read_node, dst_host_virt_addr));
static_assert(offsetof(dma_write_node, dst_device_phy_addr) == offsetof(dma_read_node, src_device_phy_addr));
static_assert(offsetof(dma_write_node, size) == offsetof(dma_read_node, size));

namespace rt {

void MemcpyCommandBuilder::addOp(const std::byte* hostAddr, const std::byte* deviceAddr, size_t size) {
  if (numEntries_ >= maxEntries_) {
    throw Exception("Can't add new op. Maximum number of operations is: " + std::to_string(maxEntries_));
  }
  ++numEntries_;
  dma_write_node newNode;
  newNode.dst_device_phy_addr = reinterpret_cast<uint64_t>(deviceAddr);
  newNode.src_host_virt_addr = newNode.src_host_phy_addr = reinterpret_cast<uint64_t>(hostAddr);
  newNode.size = static_cast<uint32_t>(size);
  auto ptr = reinterpret_cast<std::byte*>(&newNode);
  RT_VLOG(MID) << "Adding copy host_addr: " << std::hex << hostAddr << " device_addr: " << deviceAddr << std::dec
               << " size: " << size;
  std::copy(ptr, ptr + sizeof(newNode), std::back_insert_iterator(data_));
}

void MemcpyCommandBuilder::setTagId(rt::EventId eventId) {
  static_assert(sizeof(std::underlying_type_t<EventId>) == sizeof(tag_id_t));
  auto cmdPtr = reinterpret_cast<device_ops_dma_readlist_cmd_t*>(data_.data());
  cmdPtr->command_info.cmd_hdr.tag_id = static_cast<tag_id_t>(eventId);
}

MemcpyCommandBuilder::MemcpyCommandBuilder(MemcpyType type, bool barrierEnabled, uint32_t maxEntries)
  : maxEntries_(maxEntries) {
  data_.reserve(sizeof(dma_write_node) * maxEntries_ + sizeof(device_ops_dma_readlist_cmd_t));
  data_.resize(sizeof(device_ops_dma_readlist_cmd_t));
  auto cmd = reinterpret_cast<device_ops_dma_readlist_cmd_t*>(data_.data());
  memset(cmd, 0, sizeof(*cmd));
  cmd->command_info.cmd_hdr.msg_id = type == MemcpyType::H2D ? DEV_OPS_API_MID_DEVICE_OPS_DMA_WRITELIST_CMD
                                                             : DEV_OPS_API_MID_DEVICE_OPS_DMA_READLIST_CMD;
  if (barrierEnabled)
    cmd->command_info.cmd_hdr.flags |= device_ops_api::CMD_FLAGS_BARRIER_ENABLE;
}

std::vector<std::byte> MemcpyCommandBuilder::build() {
  auto cmdPtr = reinterpret_cast<device_ops_dma_readlist_cmd_t*>(data_.data());
  cmdPtr->command_info.cmd_hdr.size = static_cast<unsigned short>(data_.size());
  return data_;
}

void MemcpyCommandBuilder::clear() {
  numEntries_ = 0;
  data_.resize(offsetof(device_ops_dma_readlist_cmd_t, list));
}

EventId RuntimeImp::doMemcpyHostToDevice(StreamId stream, const std::byte* h_src, std::byte* d_dst, size_t size,
                                         bool barrier, const CmaCopyFunction& cmaCopyFunction) {
  auto streamInfo = streamManager_.getStreamInfo(stream);
  SpinLock lock(mutex_);
  if (checkMemcpyDeviceAddress_) {
    auto& mm = memoryManagers_.at(DeviceId{streamInfo.device_});
    mm.checkOperation(d_dst, size);
  }
  auto& commandSender = find(commandSenders_, getCommandSenderIdx(streamInfo.device_, streamInfo.vq_))->second;

  auto evt = eventManager_.getNextId();
  RT_VLOG(LOW) << "MemcpyHostToDevice stream: " << static_cast<int>(stream) << " EventId: " << static_cast<int>(evt)
               << std::hex << " Host address: " << h_src << " Device address: " << d_dst << " Size: " << size;
  streamManager_.addEvent(stream, evt);

  // start sending a "ghost" command which will be create the needed barrier in command sender until we have sent all
  // commands. This is needed because we don't know if we will have enough CMA memory to hold all commands with their
  // addresses and sizes in the queue or we will have to chunk them

  commandSender.send(Command{{}, commandSender, evt, evt, stream, true});
  RT_VLOG(MID) << "H2D: Added GHOST command id: " << static_cast<int>(evt) << " to CS " << &commandSender;

  auto device = DeviceId{streamInfo.device_};

  auto& cmaManager = cmaManagers_.at(device);

  MemcpyContext mc{cmaCopyFunction, deviceLayer_->getDmaInfo(streamInfo.device_),
                   *this,           *cmaManager,
                   streamManager_,  eventManager_,
                   commandSender,   *threadPools_.at(device),
                   stream,          evt};
  auto action = std::make_unique<MemcpyH2DAction>(h_src, d_dst, size, barrier, std::move(mc));
  cmaManager->addMemcpyAction(std::move(action));
  Sync(evt);
  return evt;
}

EventId RuntimeImp::doMemcpyDeviceToHost(StreamId stream, const std::byte* d_src, std::byte* h_dst, size_t size,
                                         bool barrier, const CmaCopyFunction& cmaCopyFunction) {
  auto streamInfo = streamManager_.getStreamInfo(stream);
  auto& commandSender = find(commandSenders_, getCommandSenderIdx(streamInfo.device_, streamInfo.vq_))->second;
  SpinLock lock(mutex_);
  if (checkMemcpyDeviceAddress_) {
    auto& mm = memoryManagers_.at(DeviceId{streamInfo.device_});
    mm.checkOperation(d_src, size);
  }
  auto evt = eventManager_.getNextId();
  RT_VLOG(LOW) << "MemcpyDeviceToHost stream: " << static_cast<int>(stream) << " EventId: " << static_cast<int>(evt)
               << std::hex << " Host address: " << h_dst << " Device address: " << d_src << " Size: " << size;
  streamManager_.addEvent(stream, evt);

  // start sending a "ghost" command which will be create the needed barrier in command sender until we have sent all
  // commands. This is needed because we don't know if we will have enough CMA memory to hold all commands with their
  // addresses and sizes in the queue or we will have to chunk them.

  commandSender.send(Command{{}, commandSender, evt, evt, stream, true});
  RT_VLOG(MID) << "D2H: Added GHOST command id: " << static_cast<int>(evt) << " to CS " << &commandSender;

  auto device = DeviceId{streamInfo.device_};

  auto& cmaManager = cmaManagers_.at(device);

  MemcpyContext mc{cmaCopyFunction, deviceLayer_->getDmaInfo(streamInfo.device_),
                   *this,           *cmaManager,
                   streamManager_,  eventManager_,
                   commandSender,   *threadPools_.at(device),
                   stream,          evt};
  auto action = std::make_unique<MemcpyD2HAction>(d_src, h_dst, size, barrier, std::move(mc));
  cmaManager->addMemcpyAction(std::move(action));
  Sync(evt);
  return evt;
}

EventId RuntimeImp::doMemcpyHostToDevice(StreamId stream, MemcpyList memcpyList, bool barrier,
                                         const CmaCopyFunction& cmaCopyFunction) {
  auto streamInfo = streamManager_.getStreamInfo(stream);
  checkList(streamInfo.device_, memcpyList);

  SpinLock lock(mutex_);
  if (checkMemcpyDeviceAddress_) {
    auto& mm = memoryManagers_.at(DeviceId{streamInfo.device_});
    for (auto& elem : memcpyList.operations_) {
      mm.checkOperation(elem.dst_, elem.size_);
    }
  }

  auto& commandSender = find(commandSenders_, getCommandSenderIdx(streamInfo.device_, streamInfo.vq_))->second;

  auto evt = eventManager_.getNextId();
  RT_VLOG(LOW) << "MemcpyHostToDevice (list) stream: " << static_cast<int>(stream)
               << " EventId: " << static_cast<int>(evt);
  streamManager_.addEvent(stream, evt);

  commandSender.send(Command{{}, commandSender, evt, evt, stream, true});
  RT_VLOG(MID) << "H2D: Added command id: " << static_cast<int>(evt) << " to CS " << &commandSender;

  auto device = DeviceId{streamInfo.device_};

  auto& cmaManager = cmaManagers_.at(device);

  MemcpyContext mc{cmaCopyFunction, deviceLayer_->getDmaInfo(streamInfo.device_),
                   *this,           *cmaManager,
                   streamManager_,  eventManager_,
                   commandSender,   *threadPools_.at(device),
                   stream,          evt};
  auto action = std::make_unique<MemcpyListH2DAction>(memcpyList, barrier, std::move(mc));
  cmaManager->addMemcpyAction(std::move(action));
  Sync(evt);
  return evt;
}

EventId RuntimeImp::doMemcpyDeviceToHost(StreamId stream, MemcpyList memcpyList, bool barrier,
                                         const CmaCopyFunction& cmaCopyFunction) {
  auto streamInfo = streamManager_.getStreamInfo(stream);
  checkList(streamInfo.device_, memcpyList);
  SpinLock lock(mutex_);
  if (checkMemcpyDeviceAddress_) {
    auto& mm = memoryManagers_.at(DeviceId{streamInfo.device_});
    for (auto& elem : memcpyList.operations_) {
      mm.checkOperation(elem.src_, elem.size_);
    }
  }
  auto& commandSender = find(commandSenders_, getCommandSenderIdx(streamInfo.device_, streamInfo.vq_))->second;

  auto evt = eventManager_.getNextId();
  RT_VLOG(LOW) << "MemcpyDeviceToHost (list) stream: " << static_cast<int>(stream)
               << " EventId: " << static_cast<int>(evt);
  streamManager_.addEvent(stream, evt);

  commandSender.send(Command{{}, commandSender, evt, evt, stream, true});
  RT_VLOG(MID) << "D2H: Added GHOST command id: " << static_cast<int>(evt) << " to CS " << &commandSender;

  auto device = DeviceId{streamInfo.device_};
  auto& cmaManager = cmaManagers_.at(device);

  MemcpyContext mc{cmaCopyFunction, deviceLayer_->getDmaInfo(streamInfo.device_),
                   *this,           *cmaManager,
                   streamManager_,  eventManager_,
                   commandSender,   *threadPools_.at(device),
                   stream,          evt};
  auto action = std::make_unique<MemcpyListD2HAction>(memcpyList, barrier, std::move(mc));
  cmaManager->addMemcpyAction(std::move(action));

  Sync(evt);
  return evt;
}

EventId RuntimeImp::doMemcpyDeviceToDevice(StreamId streamSrc, DeviceId deviceDst, const std::byte* d_src,
                                           std::byte* d_dst, size_t size, bool barrier) {
  auto streamInfo = streamManager_.getStreamInfo(streamSrc);
  auto& commandSender = find(commandSenders_, getCommandSenderIdx(streamInfo.device_, streamInfo.vq_))->second;
  if (!doIsP2PEnabled(DeviceId{streamInfo.device_}, deviceDst)) {
    RT_LOG(WARNING) << "Devices " << streamInfo.device_ << " and " << static_cast<int>(deviceDst)
                    << " do not support p2p memcpy operation.";
    throw Exception("P2P unsupported for these devices");
  }
  if (auto dmaInfo = deviceLayer_->getDmaInfo(streamInfo.device_); size > dmaInfo.maxElementSize_) {
    std::stringstream ss;
    ss << "Max supported memcpyDeviceToDevice size is: " << dmaInfo.maxElementSize_;
    throw Exception(ss.str());
  }
  auto dc = deviceLayer_->getDeviceConfig(static_cast<int>(deviceDst));
  SpinLock lock(mutex_);
  if (checkMemcpyDeviceAddress_) {
    const auto& mmSrc = memoryManagers_.at(DeviceId{streamInfo.device_});
    mmSrc.checkOperation(d_src, size);

    const auto& mmDst = memoryManagers_.at(deviceDst);
    mmDst.checkOperation(d_dst, size);
  }
  auto evt = eventManager_.getNextId();
  RT_VLOG(LOW) << "MemcpyDeviceToDevice streamSrc: " << static_cast<int>(streamSrc)
               << " Device destination: " << static_cast<int>(deviceDst) << " EventId: " << static_cast<int>(evt)
               << std::hex << " DeviceSrc address: " << d_src << " DeviceDst address: " << d_dst << " Size: " << size;
  streamManager_.addEvent(streamSrc, evt);

  auto data = std::vector<std::byte>(sizeof(device_ops_p2pdma_readlist_cmd_t) + sizeof(p2pdma_read_node));
  auto dataPtr = reinterpret_cast<device_ops_p2pdma_readlist_cmd_t*>(data.data());

  dataPtr->command_info.cmd_hdr.size = static_cast<msg_size_t>(data.size());
  dataPtr->command_info.cmd_hdr.tag_id = static_cast<tag_id_t>(evt);
  dataPtr->command_info.cmd_hdr.msg_id = DEV_OPS_API_MID_DEVICE_OPS_P2PDMA_READLIST_CMD;
  if (barrier) {
    dataPtr->command_info.cmd_hdr.flags |= device_ops_api::CMD_FLAGS_BARRIER_ENABLE;
  }
  dataPtr->list[0].src_device_phy_addr = reinterpret_cast<uint64_t>(d_src);
  dataPtr->list[0].dst_device_phy_addr = reinterpret_cast<uint64_t>(d_dst);
  dataPtr->list[0].peer_devnum = dc.physDeviceId_;
  dataPtr->list[0].size = static_cast<uint32_t>(size);

  commandSender.send(Command{std::move(data), commandSender, evt, evt, streamSrc, true, true, true});

  Sync(evt);
  return evt;
}

EventId RuntimeImp::doMemcpyDeviceToDevice(DeviceId deviceSrc, StreamId streamDst, const std::byte* d_src,
                                           std::byte* d_dst, size_t size, bool barrier) {
  auto streamInfo = streamManager_.getStreamInfo(streamDst);
  auto& commandSender = find(commandSenders_, getCommandSenderIdx(streamInfo.device_, streamInfo.vq_))->second;
  if (!doIsP2PEnabled(DeviceId{streamInfo.device_}, deviceSrc)) {
    RT_LOG(WARNING) << "Devices " << streamInfo.device_ << " and " << static_cast<int>(deviceSrc)
                    << " do not support p2p memcpy operation.";
    throw Exception("P2P unsupported for these devices");
  }

  if (auto dmaInfo = deviceLayer_->getDmaInfo(streamInfo.device_); size > dmaInfo.maxElementSize_) {
    std::stringstream ss;
    ss << "Max supported memcpyDeviceToDevice size is: " << dmaInfo.maxElementSize_;
    throw Exception(ss.str());
  }
  auto dc = deviceLayer_->getDeviceConfig(static_cast<int>(deviceSrc));
  SpinLock lock(mutex_);
  if (checkMemcpyDeviceAddress_) {
    const auto& mmSrc = memoryManagers_.at(deviceSrc);
    mmSrc.checkOperation(d_src, size);

    const auto& mmDst = memoryManagers_.at(DeviceId{streamInfo.device_});
    mmDst.checkOperation(d_dst, size);
  }
  auto evt = eventManager_.getNextId();
  RT_VLOG(LOW) << "MemcpyDeviceToDevice streamDst: " << static_cast<int>(streamDst)
               << " Device source: " << static_cast<int>(deviceSrc) << " EventId: " << static_cast<int>(evt) << std::hex
               << " DeviceSrc address: " << d_src << " DeviceDst address: " << d_dst << " Size: " << size;
  streamManager_.addEvent(streamDst, evt);

  auto data = std::vector<std::byte>(sizeof(device_ops_p2pdma_writelist_cmd_t) + sizeof(p2pdma_write_node));
  auto dataPtr = reinterpret_cast<device_ops_p2pdma_writelist_cmd_t*>(data.data());

  dataPtr->command_info.cmd_hdr.size = static_cast<msg_size_t>(data.size());
  dataPtr->command_info.cmd_hdr.tag_id = static_cast<tag_id_t>(evt);
  dataPtr->command_info.cmd_hdr.msg_id = DEV_OPS_API_MID_DEVICE_OPS_P2PDMA_WRITELIST_CMD;
  if (barrier) {
    dataPtr->command_info.cmd_hdr.flags |= device_ops_api::CMD_FLAGS_BARRIER_ENABLE;
  }
  dataPtr->list[0].src_device_phy_addr = reinterpret_cast<uint64_t>(d_src);
  dataPtr->list[0].dst_device_phy_addr = reinterpret_cast<uint64_t>(d_dst);
  dataPtr->list[0].peer_devnum = dc.physDeviceId_;
  dataPtr->list[0].size = static_cast<uint32_t>(size);

  commandSender.send(Command{std::move(data), commandSender, evt, evt, streamDst, true, true, true});

  Sync(evt);
  return evt;
}
} // namespace rt
