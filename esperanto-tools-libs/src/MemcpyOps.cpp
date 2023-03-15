/*-------------------------------------------------------------------------
 * Copyright (C) 2021,2020, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/

#include "MemcpyOps.h"
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
  if (checkMemcpyDeviceAddress_) {
    SpinLock lock(mutex_);
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

  commandSender.send(Command{{}, commandSender, evt, evt, true});
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

  if (checkMemcpyDeviceAddress_) {
    SpinLock lock(mutex_);
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

  commandSender.send(Command{{}, commandSender, evt, evt, true});
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

  if (checkMemcpyDeviceAddress_) {
    SpinLock lock(mutex_);
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

  commandSender.send(Command{{}, commandSender, evt, evt, true});
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
  if (checkMemcpyDeviceAddress_) {
    SpinLock lock(mutex_);
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

  commandSender.send(Command{{}, commandSender, evt, evt, true});
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
} // namespace rt
