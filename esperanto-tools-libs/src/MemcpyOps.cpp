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
#include "dma/CmaManager.h"
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

namespace {
constexpr auto kMinBytesPerEntry = 1024UL;
constexpr auto kBytesReservedForPrioritaryCommands = 256UL << 20;

size_t getFreeCmaForCommand(const rt::CmaManager& cmaManager, std::optional<rt::EventId> topPrio, rt::EventId eventId) {
  auto freeSize = cmaManager.getFreeBytes();
  if (topPrio && topPrio != eventId) {
    freeSize = (freeSize > kBytesReservedForPrioritaryCommands) ? (freeSize - kBytesReservedForPrioritaryCommands) : 0;
    RT_VLOG(HIGH) << "Current command :" << static_cast<int>(eventId)
                  << " Top prio command: " << static_cast<int>(topPrio.value())
                  << " Max allocation bytes: " << freeSize;
  }
  return freeSize;
}
} // namespace

namespace rt {

void MemcpyCommandBuilder::addOp(const std::byte* hostAddr, const std::byte* deviceAddr, size_t size) {
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

MemcpyCommandBuilder::MemcpyCommandBuilder(MemcpyType type, bool barrierEnabled) {
  data_.reserve(sizeof(dma_write_node) * DEVICE_OPS_DMA_LIST_NODES_MAX + sizeof(device_ops_dma_readlist_cmd_t));
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

EventId RuntimeImp::memcpyHostToDevice(StreamId stream, const std::byte* h_src, std::byte* d_dst, size_t size,
                                       bool barrier) {
  ScopedProfileEvent profileEvent(Class::MemcpyHostToDevice, *profiler_, stream);
  auto eventId = memcpyHostToDeviceWithoutProfiling(stream, h_src, d_dst, size, barrier);
  profileEvent.setEventId(eventId);
  return eventId;
}

EventId RuntimeImp::memcpyHostToDeviceWithoutProfiling(StreamId stream, const std::byte* h_src, const std::byte* d_dst,
                                                       size_t size, bool barrier,
                                                       const CmaCopyFunction& cmaCopyFunction) {
  std::unique_lock lock(mutex_);
  auto streamInfo = streamManager_.getStreamInfo(stream);
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
  commandSender.send(Command{{}, commandSender, evt});
  RT_VLOG(HIGH) << "H2D: Added GHOST command id: " << static_cast<int>(evt) << " to CS " << &commandSender;

  blockableThreadPool_.pushTask([this, size, barrier, d_dst, h_src, evt, stream, streamInfo, cmaCopyFunction] {
    RT_VLOG(HIGH) << "Start processing command id " << static_cast<int>(evt);
    std::vector<EventId> cmdEvents;
    auto& cs = find(commandSenders_, getCommandSenderIdx(streamInfo.device_, streamInfo.vq_))->second;
    auto dmaInfo = deviceLayer_->getDmaInfo(streamInfo.device_);
    MemcpyCommandBuilder builder(MemcpyType::H2D, barrier);
    auto offset = 0UL;
    auto pendingBytes = size;
    while (pendingBytes > 0) {
      std::unique_lock lock(mutex_);
      auto topPrio = cs.getTopPrioritaryCommand();
      auto freeSize = getFreeCmaForCommand(*cmaManager_, topPrio, evt);
      if (freeSize > kMinBytesPerEntry) {
        // calculate current memcpy command size
        auto currentSize =
          std::min(std::min(freeSize, pendingBytes), dmaInfo.maxElementSize_ * dmaInfo.maxElementCount_);
        // prepare the command
        auto cmaPtr = cmaManager_->alloc(currentSize);
        lock.unlock();
        if (!cmaPtr) {
          throw Exception("Inconsistency error in CMA Manager");
        }
        auto entrySize = std::min(currentSize, dmaInfo.maxElementSize_);
        auto cmaPtrOffset = 0UL;
        while (currentSize > 0) {
          entrySize = std::min(entrySize, currentSize);
          builder.addOp(cmaPtr + cmaPtrOffset, d_dst + offset, entrySize);
          // copy the data to the cma buffer
          cmaCopyFunction(h_src + offset, cmaPtr + cmaPtrOffset, entrySize, CmaCopyType::TO_CMA);
          offset += entrySize;
          cmaPtrOffset += entrySize;
          currentSize -= entrySize;
          pendingBytes -= entrySize;
        }

        auto cmdEvt = eventManager_.getNextId();
        streamManager_.addEvent(stream, cmdEvt);
        builder.setTagId(cmdEvt);
        cs.sendBefore(evt, {builder.build(), cs, cmdEvt, true, true});
        builder.clear();
        cmdEvents.emplace_back(cmdEvt);

        // add a thread which will free the cma memory
        eventManager_.addOnDispatchCallback({{cmdEvt}, [this, cmaPtr] { cmaManager_->free(cmaPtr); }});
      } else {
        lock.unlock();
        if (topPrio && topPrio != evt) {
          RT_VLOG(LOW) << "H2D. Waiting for top prio command: " << static_cast<int>(topPrio.value());
          waitForEventWithoutProfiling(topPrio.value());
        } else {
          RT_VLOG(LOW) << "H2D. I'm top prio command (" << static_cast<int>(evt) << "); waiting for epoll. ";
          uint64_t sq;
          bool cq;
          deviceLayer_->waitForEpollEventsMasterMinion(streamInfo.device_, sq, cq, std::chrono::milliseconds(1));
        }
      }
    }
    eventManager_.addOnDispatchCallback({cmdEvents, [this, evt] {
                                           RT_VLOG(LOW)
                                             << "MemcpyH2D done, dispatching last event: " << static_cast<int>(evt);
                                           dispatch(evt);
                                         }});

    // remove the ghost
    cs.cancel(evt);
    RT_VLOG(HIGH) << "H2D: Cancelled GHOST command: " << static_cast<int>(evt);
    RT_VLOG(HIGH) << "End processing command id " << static_cast<int>(evt);
  });
  lock.unlock();
  Sync(evt);
  return evt;
}

EventId RuntimeImp::memcpyHostToDevice(StreamId stream, const IDmaBuffer* h_src, std::byte* d_dst, size_t size,
                                       bool barrier) {
  ScopedProfileEvent profileEvent(Class::MemcpyHostToDevice, *profiler_, stream);
  auto eventId = memcpyHostToDeviceWithoutProfiling(stream, h_src, d_dst, size, barrier);
  profileEvent.setEventId(eventId);
  return eventId;
}

EventId RuntimeImp::memcpyHostToDeviceWithoutProfiling(StreamId stream, const IDmaBuffer* h_src, const std::byte* d_dst,
                                                       size_t size, bool barrier) {
  if (size > h_src->getSize()) {
    throw Exception("Trying to do a memcpy host to device using a dma buffer shorter than required size.");
  }
  std::unique_lock lock(mutex_);
  auto streamInfo = streamManager_.getStreamInfo(stream);
  if (checkMemcpyDeviceAddress_) {
    auto& mm = memoryManagers_.at(DeviceId{streamInfo.device_});
    mm.checkOperation(d_dst, size);
  }
  MemcpyCommandBuilder cmdBuilder{MemcpyType::H2D, barrier};
  cmdBuilder.addOp(h_src->getPtr(), d_dst, size);
  auto evt = eventManager_.getNextId();
  cmdBuilder.setTagId(evt);
  RT_VLOG(LOW) << "MemcpyHostToDevice (DMABuffer) stream: " << static_cast<int>(stream)
               << " EventId: " << static_cast<int>(evt) << std::hex << " Host address: " << h_src->getPtr()
               << " Device address: " << d_dst << " Size: " << size;
  auto& commandSender = find(commandSenders_, getCommandSenderIdx(streamInfo.device_, streamInfo.vq_))->second;
  streamManager_.addEvent(stream, evt);
  commandSender.send(Command{cmdBuilder.build(), commandSender, evt, true, true});
  return evt;
}

EventId RuntimeImp::memcpyDeviceToHost(StreamId stream, const std::byte* d_src, IDmaBuffer* h_dst, size_t size,
                                       bool barrier) {
  ScopedProfileEvent profileEvent(Class::MemcpyDeviceToHost, *profiler_, stream);
  auto eventId = memcpyDeviceToHostWithoutProfiling(stream, d_src, h_dst, size, barrier);
  profileEvent.setEventId(eventId);
  return eventId;
}

EventId RuntimeImp::memcpyDeviceToHostWithoutProfiling(StreamId stream, const std::byte* d_src, const IDmaBuffer* h_dst,
                                                       size_t size, bool barrier) {
  if (size > h_dst->getSize()) {
    throw Exception("Trying to do a memcpy device to host using a dma buffer shorter than required size.");
  }
  std::unique_lock lock(mutex_);
  auto streamInfo = streamManager_.getStreamInfo(stream);
  if (checkMemcpyDeviceAddress_) {
    auto& mm = memoryManagers_.at(DeviceId{streamInfo.device_});
    mm.checkOperation(d_src, size);
  }
  MemcpyCommandBuilder cmdBuilder{MemcpyType::D2H, barrier};
  cmdBuilder.addOp(h_dst->getPtr(), d_src, size);
  auto evt = eventManager_.getNextId();
  cmdBuilder.setTagId(evt);
  RT_VLOG(LOW) << "MemcpyDeviceToHost (DMABuffer) stream: " << static_cast<int>(stream)
               << " EventId: " << static_cast<int>(evt) << std::hex << " Host address: " << d_src
               << " Device address: " << h_dst->getPtr() << std::dec << " Size: " << size;
  auto& commandSender = find(commandSenders_, getCommandSenderIdx(streamInfo.device_, streamInfo.vq_))->second;
  streamManager_.addEvent(stream, evt);
  commandSender.send(Command{cmdBuilder.build(), commandSender, evt, true, true});
  return evt;
}

EventId RuntimeImp::memcpyDeviceToHost(StreamId stream, const std::byte* d_src, std::byte* h_dst, size_t size,
                                       bool barrier) {
  ScopedProfileEvent profileEvent(Class::MemcpyDeviceToHost, *profiler_, stream);
  auto eventId = memcpyDeviceToHostWithoutProfiling(stream, d_src, h_dst, size, barrier);
  profileEvent.setEventId(eventId);
  return eventId;
}

EventId RuntimeImp::memcpyDeviceToHostWithoutProfiling(StreamId stream, const std::byte* d_src, std::byte* h_dst,
                                                       size_t size, bool barrier,
                                                       const CmaCopyFunction& cmaCopyFunction) {
  std::unique_lock lock(mutex_);
  auto streamInfo = streamManager_.getStreamInfo(stream);
  auto& commandSender = find(commandSenders_, getCommandSenderIdx(streamInfo.device_, streamInfo.vq_))->second;

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
  // addresses and sizes in the queue or we will have to chunk them

  commandSender.send(Command{{}, commandSender, evt});
  RT_VLOG(HIGH) << "D2H: Added GHOST command id: " << static_cast<int>(evt) << " to CS " << &commandSender;

  blockableThreadPool_.pushTask([this, stream, size, barrier, d_src, h_dst, evt, streamInfo, cmaCopyFunction] {
    RT_VLOG(HIGH) << "Start processing command id " << static_cast<int>(evt);
    std::vector<EventId> cmdEvents;
    auto& cs = find(commandSenders_, getCommandSenderIdx(streamInfo.device_, streamInfo.vq_))->second;
    auto dmaInfo = deviceLayer_->getDmaInfo(streamInfo.device_);
    MemcpyCommandBuilder builder(MemcpyType::D2H, barrier);
    auto offset = 0UL;
    auto pendingBytes = size;
    while (pendingBytes > 0) {
      std::unique_lock lock(mutex_);
      auto topPrio = cs.getTopPrioritaryCommand();
      auto freeSize = getFreeCmaForCommand(*cmaManager_, topPrio, evt);
      if (freeSize > kMinBytesPerEntry) {
        struct FinalCopy {
          std::byte* src;
          std::byte* dst;
          size_t size;
        };
        // this is needed to make the final copy from the cma to the user memory; after all commands finish
        std::vector<FinalCopy> cmdFinalCopies;
        // calculate current memcpy command size
        auto currentSize =
          std::min(std::min(freeSize, pendingBytes), dmaInfo.maxElementSize_ * dmaInfo.maxElementCount_);
        // prepare the command
        auto cmaPtr = cmaManager_->alloc(currentSize);
        lock.unlock();
        if (!cmaPtr) {
          throw Exception("Inconsistency error in CMA Manager");
        }
        auto entrySize = std::min(currentSize, dmaInfo.maxElementSize_);
        auto cmaPtrOffset = 0UL;
        while (currentSize > 0) {
          entrySize = std::min(entrySize, currentSize);
          builder.addOp(cmaPtr + cmaPtrOffset, d_src + offset, entrySize);
          cmdFinalCopies.emplace_back(FinalCopy{cmaPtr + cmaPtrOffset, h_dst + offset, entrySize});
          offset += entrySize;
          cmaPtrOffset += entrySize;
          currentSize -= entrySize;
          pendingBytes -= entrySize;
        }

        auto cmdEvt = eventManager_.getNextId();
        streamManager_.addEvent(stream, cmdEvt);
        builder.setTagId(cmdEvt);
        cs.sendBefore(evt, {builder.build(), cs, cmdEvt, true, true});
        builder.clear();

        // this extra event is needed to make sure we wait till the final copy between cma and user memory
        auto syncCopyEvt = eventManager_.getNextId();
        streamManager_.addEvent(stream, syncCopyEvt);
        cmdEvents.emplace_back(syncCopyEvt);

        // add a thread which will free the cma memory
        eventManager_.addOnDispatchCallback(
          {{cmdEvt}, [this, cmaPtr, cmdFinalCopies = std::move(cmdFinalCopies), syncCopyEvt, cmaCopyFunction] {
             for (auto& copy : cmdFinalCopies) {
               cmaCopyFunction(copy.src, copy.dst, copy.size, CmaCopyType::FROM_CMA);
             }
             cmaManager_->free(cmaPtr);
             dispatch(syncCopyEvt);
           }});
      } else {
        lock.unlock();
        if (topPrio && topPrio != evt) {
          RT_VLOG(LOW) << "D2H. Waiting for top prio command: " << static_cast<int>(topPrio.value());
          waitForEventWithoutProfiling(topPrio.value());
        } else {
          RT_VLOG(LOW) << "D2H. Current command is top prio command (" << static_cast<int>(evt)
                       << "); waiting for epoll. ";
          uint64_t sq;
          bool cq;
          deviceLayer_->waitForEpollEventsMasterMinion(streamInfo.device_, sq, cq, std::chrono::milliseconds(1));
        }
      }
    }
    eventManager_.addOnDispatchCallback({cmdEvents, [this, evt] {
                                           RT_VLOG(LOW)
                                             << "MemcpyD2H done, dispatching last event: " << static_cast<int>(evt);
                                           dispatch(evt);
                                         }});
    // remove the ghost
    cs.cancel(evt);
    RT_VLOG(HIGH) << "D2H: Cancelled GHOST command: " << static_cast<int>(evt);
    RT_VLOG(HIGH) << "End processing command id " << static_cast<int>(evt);
  });
  lock.unlock();
  Sync(evt);
  return evt;
}

EventId RuntimeImp::memcpyHostToDevice(StreamId stream, MemcpyList memcpyList, bool barrier) {
  ScopedProfileEvent profileEvent(Class::MemcpyHostToDevice, *profiler_, stream);
  auto eventId = memcpyHostToDeviceWithoutProfiling(stream, memcpyList, barrier);
  profileEvent.setEventId(eventId);
  return eventId;
}
EventId RuntimeImp::memcpyDeviceToHost(StreamId stream, MemcpyList memcpyList, bool barrier) {
  ScopedProfileEvent profileEvent(Class::MemcpyDeviceToHost, *profiler_, stream);
  auto eventId = memcpyDeviceToHostWithoutProfiling(stream, memcpyList, barrier);
  profileEvent.setEventId(eventId);
  return eventId;
}

EventId RuntimeImp::memcpyHostToDeviceWithoutProfiling(StreamId stream, MemcpyList memcpyList, bool barrier,
                                                       const CmaCopyFunction& cmaCopyFunction) {
  std::unique_lock lock(mutex_);
  auto streamInfo = streamManager_.getStreamInfo(stream);
  checkList(streamInfo.device_, memcpyList);

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

  commandSender.send(Command{{}, commandSender, evt, true});
  RT_VLOG(HIGH) << "H2D: Added command id: " << static_cast<int>(evt) << " to CS " << &commandSender;

  blockableThreadPool_.pushTask([this, barrier, memcpyList, evt, streamInfo, cmaCopyFunction] {
    RT_VLOG(HIGH) << "Start processing command id " << static_cast<int>(evt);
    auto& cs = find(commandSenders_, getCommandSenderIdx(streamInfo.device_, streamInfo.vq_))->second;
    auto totalSize = 0UL;
    for (auto& op : memcpyList.operations_) {
      totalSize += op.size_;
    }
    MemcpyCommandBuilder builder(MemcpyType::H2D, barrier);
    builder.setTagId(evt);
    std::unique_lock lock(mutex_);
    auto topPrio = cs.getTopPrioritaryCommand();
    auto freeSize = getFreeCmaForCommand(*cmaManager_, topPrio, evt);
    while (freeSize <= totalSize) {
      lock.unlock();
      if (topPrio != evt) {
        throw Exception("Inconsistency in runtime: If current command is topPrio command then it should have enough "
                        "CMA memory available.");
      }
      RT_VLOG(LOW) << "H2D. Waiting for top prio command: " << static_cast<int>(topPrio.value());
      waitForEventWithoutProfiling(topPrio.value());
      lock.lock();
      topPrio = cs.getTopPrioritaryCommand();
      freeSize = getFreeCmaForCommand(*cmaManager_, topPrio, evt);
    }
    auto cmaPtr = cmaManager_->alloc(totalSize);
    if (!cmaPtr) {
      throw Exception("Inconsistency error in CMA Manager");
    }
    auto cmaPtrOffset = 0UL;
    for (auto& op : memcpyList.operations_) {
      builder.addOp(cmaPtr + cmaPtrOffset, op.dst_, op.size_);
      cmaCopyFunction(op.src_, cmaPtr + cmaPtrOffset, op.size_, CmaCopyType::TO_CMA);
      cmaPtrOffset += op.size_;
    }
    cs.setCommandData(evt, builder.build());
    cs.enable(evt);
    eventManager_.addOnDispatchCallback({{evt}, [this, cmaPtr] { cmaManager_->free(cmaPtr); }});
    RT_VLOG(HIGH) << "End processing command id " << static_cast<int>(evt);
  });
  lock.unlock();
  Sync(evt);
  return evt;
}
EventId RuntimeImp::memcpyDeviceToHostWithoutProfiling(StreamId stream, MemcpyList memcpyList, bool barrier,
                                                       const CmaCopyFunction& cmaCopyFunction) {
  std::unique_lock lock(mutex_);
  auto streamInfo = streamManager_.getStreamInfo(stream);
  checkList(streamInfo.device_, memcpyList);
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

  commandSender.send(Command{{}, commandSender, evt});
  RT_VLOG(HIGH) << "D2H: Added GHOST command id: " << static_cast<int>(evt) << " to CS " << &commandSender;

  blockableThreadPool_.pushTask([this, barrier, memcpyList, evt, streamInfo, stream, cmaCopyFunction] {
    RT_VLOG(HIGH) << "Start processing command id " << static_cast<int>(evt);
    auto& cs = find(commandSenders_, getCommandSenderIdx(streamInfo.device_, streamInfo.vq_))->second;
    auto totalSize = 0UL;
    for (auto& op : memcpyList.operations_) {
      totalSize += op.size_;
    }
    std::unique_lock lock(mutex_);
    auto topPrio = cs.getTopPrioritaryCommand();
    auto freeSize = getFreeCmaForCommand(*cmaManager_, topPrio, evt);
    while (freeSize <= totalSize) {
      lock.unlock();
      if (topPrio != evt) {
        throw Exception("Inconsistency in runtime: If current command is topPrio command then it should have enough "
                        "CMA memory available.");
      }
      RT_VLOG(LOW) << "D2H. Waiting for top prio command: " << static_cast<int>(topPrio.value());
      waitForEventWithoutProfiling(topPrio.value());
      lock.lock();
      topPrio = cs.getTopPrioritaryCommand();
      freeSize = getFreeCmaForCommand(*cmaManager_, topPrio, evt);
    }
    auto cmaPtr = cmaManager_->alloc(totalSize);
    if (!cmaPtr) {
      throw Exception("Inconsistency error in CMA Manager");
    }
    auto cmaPtrOffset = 0UL;
    MemcpyCommandBuilder builder(MemcpyType::D2H, barrier);
    auto cmdEvt = eventManager_.getNextId();
    streamManager_.addEvent(stream, cmdEvt);
    builder.setTagId(cmdEvt);
    for (auto& op : memcpyList.operations_) {
      builder.addOp(cmaPtr + cmaPtrOffset, op.src_, op.size_);
      cmaPtrOffset += op.size_;
    }
    cs.sendBefore(evt, {builder.build(), cs, cmdEvt, true, true});
    cs.cancel(evt);
    RT_VLOG(HIGH) << "D2H: Cancelled GHOST command: " << static_cast<int>(evt);

    eventManager_.addOnDispatchCallback({{cmdEvt}, [this, evt, cmaPtr, cmaCopyFunction, ops = memcpyList.operations_] {
                                           auto offset = 0UL;
                                           for (auto& op : ops) {
                                             cmaCopyFunction(cmaPtr + offset, op.dst_, op.size_, CmaCopyType::FROM_CMA);
                                             offset += op.size_;
                                           }
                                           cmaManager_->free(cmaPtr);
                                           dispatch(evt);
                                         }});
    RT_VLOG(HIGH) << "End processing command id " << static_cast<int>(evt);
  });
  lock.unlock();
  Sync(evt);
  return evt;
}
} // namespace rt
