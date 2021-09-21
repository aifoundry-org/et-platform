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
#include <esperanto/device-apis/device_apis_message_types.h>
#include <esperanto/device-apis/operations-api/device_ops_api_cxx.h>
#include <esperanto/device-apis/operations-api/device_ops_api_rpc_types.h>
#include <iterator>
#include <optional>
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
  RT_LOG_IF(FATAL, numEntries_ == kMaxEntries) << "Can't add more entries. Max number of entries is " << kMaxEntries;
  ++numEntries_;
  dma_write_node newNode;
  newNode.dst_device_phy_addr = reinterpret_cast<uint64_t>(deviceAddr);
  newNode.src_host_virt_addr = newNode.src_host_phy_addr = reinterpret_cast<uint64_t>(hostAddr);
  newNode.size = static_cast<uint32_t>(size);
  auto ptr = reinterpret_cast<std::byte*>(&newNode);
  RT_VLOG(MID) << "Adding copy host_addr: " << std::hex << hostAddr << " device_addr: " << deviceAddr
               << " size: " << size;
  std::copy(ptr, ptr + sizeof(newNode), std::back_insert_iterator(data_));
}

void MemcpyCommandBuilder::setTagId(rt::EventId eventId) {
  static_assert(sizeof(std::underlying_type_t<EventId>) == sizeof(tag_id_t));
  auto cmdPtr = reinterpret_cast<device_ops_dma_readlist_cmd_t*>(data_.data());
  cmdPtr->command_info.cmd_hdr.tag_id = static_cast<tag_id_t>(eventId);
}

MemcpyCommandBuilder::MemcpyCommandBuilder(MemcpyType type, bool barrierEnabled) {
  data_.reserve(sizeof(dma_write_node) * kMaxEntries + sizeof(device_ops_dma_readlist_cmd_t));
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

CommandsSentResult prepareAndSendCommands(MemcpyType memcpyType, bool barrierEnabled, StreamId stream,
                                          StreamManager* streamManager, EventManager* eventManager,
                                          const std::vector<DmaBufferInfo>& stageBuffers, CommandSender* commandSender,
                                          const std::byte* devicePtr, bool enableCommands) {
  CommandsSentResult res;
  auto setupTagId = [stream, streamManager, eventManager, commandEvents = &res.events_](auto& builder) {
    auto evt = eventManager->getNextId();
    streamManager->addEvent(stream, evt);
    builder.setTagId(evt);
    commandEvents->emplace_back(evt);
  };
  MemcpyCommandBuilder cmdBuilder{memcpyType, barrierEnabled};
  setupTagId(cmdBuilder);

  for (auto i = 0UL, offset = 0UL, count = stageBuffers.size(); i < count; ++i) {
    if (cmdBuilder.numEntries_ == MemcpyCommandBuilder::kMaxEntries) {
      auto cmd = commandSender->send(Command{cmdBuilder.build(), *commandSender, true, enableCommands});
      res.commands_.emplace_back(cmd);
      cmdBuilder.clear();
      setupTagId(cmdBuilder);
    }
    auto& hostBuffer = stageBuffers[i];
    cmdBuilder.addOp(hostBuffer.ptr_, devicePtr + offset, hostBuffer.size_);
    offset += hostBuffer.size_;
  }

  // add the last command
  auto cmd = commandSender->send(Command{cmdBuilder.build(), *commandSender, true, enableCommands});
  res.commands_.emplace_back(cmd);
  return res;
}

// perform a staged copy, enable all commands after the copy in the passed vector and finally trigger the given event
void doStagedCopyAndEnableCommands(threadPool::ThreadPool* threadPool, RuntimeImp* runtime, std::byte* hostPtr,
                                   std::vector<DmaBufferInfo> stageBuffers, std::vector<Command*> commands,
                                   std::optional<EventId> syncEvent, MemcpyType type) {
  auto task = [runtime, hostPtr, stageBuffers = std::move(stageBuffers), commands = std::move(commands), syncEvent,
               type] {
    for (auto i = 0UL, offset = 0UL, count = stageBuffers.size(); i < count; ++i) {
      auto sbPtr = stageBuffers[i].ptr_;
      auto size = stageBuffers[i].size_;
      if (type == MemcpyType::H2D) { // then the staged buffer is the dst
        memcpy(sbPtr, hostPtr + offset, size);
        RT_VLOG(MID) << "Copied stage buffer from " << std::hex << hostPtr + offset << " to " << sbPtr
                     << " size: " << size;
      } else { // then the hostPtr is the dst
        memcpy(hostPtr + offset, sbPtr, size);
        RT_VLOG(MID) << "Copied stage buffer from " << std::hex << sbPtr << " to " << hostPtr + offset
                     << " size: " << size;
      }
      offset += size;
    }
    for (auto c : commands) {
      c->enable();
    }
    // finally trigger that the task is complete
    if (syncEvent) {
      RT_VLOG(MID) << "After all copies have been done, dispatching sync event: " << int(syncEvent.value());
      runtime->dispatch(syncEvent.value());
    }
  };
  threadPool->pushTask(task);
}

EventId RuntimeImp::memcpyHostToDevice(StreamId stream, const std::byte* h_src, std::byte* d_dst, size_t size,
                                       bool barrier) {
  ScopedProfileEvent profileEvent(Class::MemcpyHostToDevice, profiler_, stream);

  auto streamInfo = streamManager_.getStreamInfo(stream);

  // this lock must be mantained until all commands have been sent to the commandSender, to gaurantee the ordering.
  std::unique_lock lock(mutex_);
  auto& hbm = find(hostBufferManagers_, DeviceId{streamInfo.device_})->second;
  auto& commandSender = find(commandSenders_, getCommandSenderIdx(streamInfo.device_, streamInfo.vq_))->second;
  auto hostBuffers = hbm.alloc(size);

  auto stageBuffers = getDmaBufferInfo(hostBuffers);

  auto [commandEvents, commands] = prepareAndSendCommands(MemcpyType::H2D, barrier, stream, &streamManager_,
                                                          &eventManager_, stageBuffers, &commandSender, d_dst, false);
  auto evt = eventManager_.getNextId();
  RT_VLOG(LOW) << "MemcpyHostToDevice stream: " << static_cast<int>(stream) << "EventId: " << static_cast<int>(evt)
               << std::hex << " Host address: " << h_src << " Device address: " << d_dst << " Size: " << size;
  // now we can free the lock since the commands are already serialized into the commandSender queue
  lock.unlock();
  streamManager_.addEvent(stream, evt);

  doStagedCopyAndEnableCommands(&nonblockableThreadPool_, this, const_cast<std::byte*>(h_src), std::move(stageBuffers),
                                std::move(commands), std::nullopt, MemcpyType::H2D);

  // now, add a new task to wait for all the commands and trigger the sync event
  blockableThreadPool_.pushTask([this, evt, eventsToWait = std::move(commandEvents),
                                 hostBuffers = std::move(hostBuffers)] {
    RT_VLOG(MID) << "Waiting for events (" << eventsToWait.size() << ") to complete before dispatching event "
                 << static_cast<int>(evt);
    for (auto e : eventsToWait) {
      waitForEvent(e);
    }
    dispatch(evt);
  });
  profileEvent.setEventId(evt);
  Sync(evt);
  return evt;
}

EventId RuntimeImp::memcpyDeviceToHost(StreamId stream, const std::byte* d_src, std::byte* h_dst, size_t size,
                                       bool barrier) {
  ScopedProfileEvent profileEvent(Class::MemcpyDeviceToHost, profiler_, stream);

  auto streamInfo = streamManager_.getStreamInfo(stream);
  std::unique_lock lock(mutex_);
  auto& hbm = find(hostBufferManagers_, DeviceId{streamInfo.device_})->second;
  auto& commandSender = find(commandSenders_, getCommandSenderIdx(streamInfo.device_, streamInfo.vq_))->second;
  auto hostBuffers = hbm.alloc(size);

  auto stageBuffers = getDmaBufferInfo(hostBuffers);

  // first stage the copy into the stageBuffers
  auto [commandEvents, commands] = prepareAndSendCommands(MemcpyType::D2H, barrier, stream, &streamManager_,
                                                          &eventManager_, stageBuffers, &commandSender, d_src, true);
  auto evt = eventManager_.getNextId();
  RT_VLOG(LOW) << "MemcpyDeviceToHost stream: " << static_cast<int>(stream) << "EventId: " << static_cast<int>(evt)
               << std::hex << " Host address: " << h_dst << " Device address: " << d_src << " Size: " << size;
  // now we can free the lock since the commands are already serialized into the commandSender queue
  lock.unlock();
  streamManager_.addEvent(stream, evt);

  // now, add a new task to wait for all the commands, make the final copy and trigger the sync event
  blockableThreadPool_.pushTask([this, h_dst, evt, eventsToWait = std::move(commandEvents),
                                 hostBuffers = std::move(hostBuffers),
                                 stageBuffers = std::move(stageBuffers)]() mutable {
    RT_VLOG(MID) << "Memcpy D2H waiting till copy is done from device to stage buffers.";
    for (auto e : eventsToWait) {
      waitForEvent(e);
    }
    RT_VLOG(MID) << "Memcpy D2H do the copy from stage buffers to final (user) buffers.";
    doStagedCopyAndEnableCommands(&nonblockableThreadPool_, this, h_dst, stageBuffers, {}, evt, MemcpyType::D2H);
    // this wait for event is only to hold stageBuffers enough time to finish the staged copy
    waitForEvent(evt);
    RT_VLOG(MID) << "Memcpy D2H completely finished, releasing resources.";
    stageBuffers.clear();
    RT_VLOG(MID) << "Resources released";
  });
  profileEvent.setEventId(evt);
  Sync(evt);
  return evt;
}
} // namespace rt