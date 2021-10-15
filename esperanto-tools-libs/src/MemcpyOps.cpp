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
#include "runtime/Types.h"
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
  RT_LOG_IF(FATAL, numEntries_ == DEVICE_OPS_DMA_LIST_NODES_MAX) << "Can't add more entries. Max number of entries is " << DEVICE_OPS_DMA_LIST_NODES_MAX;
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

CommandsSentResult prepareAndSendCommands(MemcpyType memcpyType, bool barrierEnabled, StreamId stream,
                                          StreamManager* streamManager, EventManager* eventManager,
                                          const std::vector<DmaBufferInfo>& stageBuffers, CommandSender* commandSender,
                                          const std::byte* devicePtr, bool enableCommands, size_t maxSize) {
  CommandsSentResult res;
  auto setupTagId = [stream, streamManager, eventManager, commandEvents = &res.events_](auto& builder) {
    auto evt = eventManager->getNextId();
    streamManager->addEvent(stream, evt);
    builder.setTagId(evt);
    commandEvents->emplace_back(evt);
  };
  MemcpyCommandBuilder cmdBuilder{memcpyType, barrierEnabled};
  auto totalSize = 0UL;

  for (auto i = 0UL, offset = 0UL, count = stageBuffers.size(); i < count; ++i) {
    if (cmdBuilder.numEntries_ == DEVICE_OPS_DMA_LIST_NODES_MAX) {
      setupTagId(cmdBuilder);
      auto cmd = commandSender->send(Command{cmdBuilder.build(), *commandSender, true, enableCommands});
      res.commands_.emplace_back(cmd);
      cmdBuilder.clear();
    }
    auto ptr = stageBuffers[i].ptr_;
    auto size = stageBuffers[i].size_;
    totalSize += size;
    if (totalSize > maxSize) {
      size -= (totalSize - maxSize);
    }
    if (size > 0) {
      cmdBuilder.addOp(ptr, devicePtr + offset, size);
      offset += size;
    }
    if (totalSize > maxSize)
      break;
  }

  // add the last command
  if (cmdBuilder.numEntries_ > 0) {
    setupTagId(cmdBuilder);
    auto cmd = commandSender->send(Command{cmdBuilder.build(), *commandSender, true, enableCommands});
    res.commands_.emplace_back(cmd);
  }
  return res;
}

// perform a staged copy, enable all commands after the copy in the passed vector and finally trigger the given event
void doStagedCopyAndEnableCommands(threadPool::ThreadPool* threadPool, RuntimeImp* runtime, std::byte* hostPtr,
                                   const std::vector<DmaBufferInfo>& stageBuffers, std::vector<Command*> commands,
                                   std::optional<EventId> syncEvent, MemcpyType type, size_t maxSize) {
  auto task = [runtime, hostPtr, stageBuffers = std::move(stageBuffers), commands = std::move(commands), syncEvent,
               type, maxSize] {
    for (auto i = 0UL, offset = 0UL, count = stageBuffers.size(), currentSize = 0UL; i < count && currentSize < maxSize;
         ++i) {
      auto sbPtr = stageBuffers[i].ptr_;
      auto size = stageBuffers[i].size_;

      currentSize += size;
      if (currentSize > maxSize) {
        size -= currentSize - maxSize;
      }
      if (size > 0) {
        RT_VLOG(HIGH) << ">>> " << std::hex << (type == MemcpyType::H2D ? "H2D" : "D2H") << " offset: " << offset
                      << " hostPtr: " << hostPtr << " size: " << size;
        if (type == MemcpyType::H2D) {           // then the staged buffer is the dst
          memcpy(sbPtr, hostPtr + offset, size); // aqui esta llegando un valor negativo
          RT_VLOG(MID) << "Copied stage buffer from " << std::hex << hostPtr + offset << " to " << sbPtr
                       << " size: " << size;
        } else { // then the hostPtr is the dst
          memcpy(hostPtr + offset, sbPtr, size);
          RT_VLOG(MID) << "Copied stage buffer from " << std::hex << sbPtr << " to " << hostPtr + offset
                       << " size: " << size;
        }
        offset += size;
      }
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

  auto streamInfo = streamManager_.getStreamInfo(stream); // NOSONAR

  // this lock must be mantained until all commands have been sent to the commandSender, to gaurantee the ordering.
  std::unique_lock lock(mutex_); // NOSONAR
  auto& commandSender =
    find(commandSenders_, getCommandSenderIdx(streamInfo.device_, streamInfo.vq_))->second; // NOSONAR
  // here check the max allocatable size. If it doesnt fit, do it in several commands stages

  auto maxSize = hostBufferManager_->getMaxAllocSize() + deviceLayer_->getFreeCmaMemory() / 2; // NOSONAR
  auto numIters = (size + maxSize - 1) / maxSize;                                              // NOSONAR

  auto cmaSize = std::min(size, maxSize);                // NOSONAR
  auto hostBuffers = hostBufferManager_->alloc(cmaSize); // NOSONAR
  auto stageBuffers = getDmaBufferInfo(hostBuffers);     // NOSONAR
  std::vector<CommandsSentResult> commandsSendResult;    // NOSONAR

  for (auto i = 0U; i < numIters; ++i) { // NOSONAR
    auto offset = i * cmaSize;           // NOSONAR
    auto hostBufferSize = cmaSize;       // NOSONAR
    // last iter need to adapt the size
    if (i == numIters - 1) {          // NOSONAR
      hostBufferSize = size - offset; // NOSONAR
    }                                 // NOSONAR
    commandsSendResult.emplace_back(prepareAndSendCommands(MemcpyType::H2D, barrier, stream, &streamManager_,
                                                           &eventManager_, stageBuffers, &commandSender, d_dst + offset,
                                                           false, hostBufferSize));
  }
  auto evt = eventManager_.getNextId();
  RT_VLOG(LOW) << "MemcpyHostToDevice stream: " << static_cast<int>(stream) << "EventId: " << static_cast<int>(evt)
               << std::hex << " Host address: " << h_src << " Device address: " << d_dst << " Size: " << size;
  // now we can free the lock since the commands are already serialized into the commandSender queue
  lock.unlock();
  streamManager_.addEvent(stream, evt);

  std::vector<EventId> waitEvents;
  for (auto i = 0UL, count = commandsSendResult.size(); i < count; ++i) {
    blockableThreadPool_.pushTask([this, waitEvents, cmaSize, totalSize = size, numIter = i, h_src, stageBuffers,
                                   commands = commandsSendResult[i].commands_] {
      auto hSrc = h_src + numIter * cmaSize;
      auto currentSize = cmaSize;
      if (((numIter + 1) * cmaSize) > totalSize) {
        currentSize -= ((numIter + 1) * cmaSize) - totalSize;
      }

      // wait for all previous events to finish
      for (auto e : waitEvents) {
        waitForEvent(e);
      }
      if (currentSize > 0) {
        // when all finished, then do the copy and enable commands
        doStagedCopyAndEnableCommands(&nonblockableThreadPool_, this, const_cast<std::byte*>(hSrc), stageBuffers,
                                      commands, std::nullopt, MemcpyType::H2D, currentSize);
      }
    });
    waitEvents = commandsSendResult[i].events_;
  }

  // now, add a new task to wait for all the commands and trigger the sync event
  blockableThreadPool_.pushTask(
    [this, evt, eventsToWait = std::move(waitEvents), hostBuffers = std::move(hostBuffers)] {
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

EventId RuntimeImp::memcpyHostToDevice(StreamId stream, const IDmaBuffer* h_src, std::byte* d_dst, size_t size,
                                       bool barrier) {
  ScopedProfileEvent profileEvent(Class::MemcpyHostToDevice, profiler_, stream);
  if (size > h_src->getSize()) {
    throw Exception("Trying to do a memcpy host to device using a dma buffer shorter than required size.");
  }
  auto streamInfo = streamManager_.getStreamInfo(stream);
  MemcpyCommandBuilder cmdBuilder{MemcpyType::H2D, barrier};
  cmdBuilder.addOp(h_src->getPtr(), d_dst, size);
  auto evt = eventManager_.getNextId();
  cmdBuilder.setTagId(evt);
  RT_VLOG(LOW) << "MemcpyHostToDevice (DMABuffer) stream: " << static_cast<int>(stream)
               << "EventId: " << static_cast<int>(evt) << std::hex << " Host address: " << h_src->getPtr()
               << " Device address: " << d_dst << " Size: " << size;
  auto& commandSender = find(commandSenders_, getCommandSenderIdx(streamInfo.device_, streamInfo.vq_))->second;
  streamManager_.addEvent(stream, evt);
  commandSender.send(Command{cmdBuilder.build(), commandSender, true, true});
  return evt;
}

EventId RuntimeImp::memcpyDeviceToHost(StreamId stream, const std::byte* d_src, IDmaBuffer* h_dst, size_t size,
                                       bool barrier) {
  ScopedProfileEvent profileEvent(Class::MemcpyDeviceToHost, profiler_, stream);
  if (size > h_dst->getSize()) {
    throw Exception("Trying to do a memcpy device to host using a dma buffer shorter than required size.");
  }
  auto streamInfo = streamManager_.getStreamInfo(stream);
  MemcpyCommandBuilder cmdBuilder{MemcpyType::D2H, barrier};
  cmdBuilder.addOp(h_dst->getPtr(), d_src, size);
  auto evt = eventManager_.getNextId();
  cmdBuilder.setTagId(evt);
  RT_VLOG(LOW) << "MemcpyDeviceToHost (DMABuffer) stream: " << static_cast<int>(stream)
               << "EventId: " << static_cast<int>(evt) << std::hex << " Host address: " << d_src
               << " Device address: " << h_dst->getPtr() << std::dec << " Size: " << size;
  auto& commandSender = find(commandSenders_, getCommandSenderIdx(streamInfo.device_, streamInfo.vq_))->second;
  streamManager_.addEvent(stream, evt);
  commandSender.send(Command{cmdBuilder.build(), commandSender, true, true});
  return evt;
}

EventId RuntimeImp::memcpyDeviceToHost(StreamId stream, const std::byte* d_src, std::byte* h_dst, size_t size,
                                       bool barrier) {
  ScopedProfileEvent profileEvent(Class::MemcpyDeviceToHost, profiler_, stream);

  auto streamInfo = streamManager_.getStreamInfo(stream);
  std::unique_lock lock(mutex_);
  auto& commandSender = find(commandSenders_, getCommandSenderIdx(streamInfo.device_, streamInfo.vq_))->second;

  auto maxSize = hostBufferManager_->getMaxAllocSize() + deviceLayer_->getFreeCmaMemory() / 2;
  auto numIters = (size + maxSize - 1) / maxSize;

  auto cmaSize = std::min(size, maxSize);
  auto hostBuffers = hostBufferManager_->alloc(cmaSize);
  auto stageBuffers = getDmaBufferInfo(hostBuffers);
  std::vector<CommandsSentResult> commandsSendResult;

  for (auto i = 0U; i < numIters; ++i) {
    auto offset = i * cmaSize;
    auto hostBufferSize = cmaSize;
    // last iter need to adapt the size
    if (i == numIters - 1) {
      hostBufferSize = size - offset;
    }
    commandsSendResult.emplace_back(prepareAndSendCommands(
      MemcpyType::D2H, barrier, stream, &streamManager_, &eventManager_, stageBuffers, &commandSender, d_src + offset,
      i == 0, hostBufferSize)); // enable by default only on first iteration
  }
  auto evt = eventManager_.getNextId();
  RT_VLOG(LOW) << "MemcpyDeviceToHost stream: " << static_cast<int>(stream) << "EventId: " << static_cast<int>(evt)
               << std::hex << " Host address: " << h_dst << " Device address: " << d_src << " Size: " << size;
  // now we can free the lock since the commands are already serialized into the commandSender queue
  lock.unlock();
  streamManager_.addEvent(stream, evt);

  for (auto i = 0UL, count = commandsSendResult.size(); i < count; ++i) {
    blockableThreadPool_.pushTask(
      [this, hDst = h_dst + i * cmaSize, stageBuffers, i, cmaSize, totalSize = size, commandsSendResult, count, evt] {
        // first wait for all previous events to finish
        auto& eventsToWait = commandsSendResult[i].events_;
        for (auto e : eventsToWait) {
          waitForEvent(e);
        }

        if (running_) {
          auto currentSize = cmaSize;
          RT_VLOG(MID) << "Memcpy D2H do the copy from stage buffers to final (user) buffers.";
          // on all iterations but last, we need to enable the next commands
          // when all finished, then do the copy and enable commands for the next iteration
          std::vector<Command*> commands{};
          std::optional<EventId> evtToSync;
          if (i != count - 1) {
            commands = commandsSendResult[i + 1].commands_;
          } else {
            evtToSync = evt;
            // adjust final copy size buffer
            currentSize -= (cmaSize * count) - totalSize;
          }
          if (currentSize > 0) {
            doStagedCopyAndEnableCommands(&nonblockableThreadPool_, this, hDst, stageBuffers, std::move(commands),
                                          evtToSync, MemcpyType::D2H, currentSize);
          } else {
            assert(evtToSync);
            dispatch(*evtToSync);
          }
        } else {
          RT_LOG(WARNING) << "Memcpy D2H didn't end because runtime was stopped before.";
        }
      });
  }

  // now, add a new task to wait for all the commands and trigger the sync event
  blockableThreadPool_.pushTask([this, evt, hostBuffers = std::move(hostBuffers)] {
    RT_VLOG(MID) << "Waiting for event (" << static_cast<int>(evt) << ") to complete and free the stage buffers.";
    waitForEvent(evt);
    RT_VLOG(MID) << "Memcpy D2H completely finished, releasing resources.";
  });
  profileEvent.setEventId(evt);
  Sync(evt);
  return evt;
}
} // namespace rt
