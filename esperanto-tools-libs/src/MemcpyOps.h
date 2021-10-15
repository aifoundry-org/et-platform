/*-------------------------------------------------------------------------
 * Copyright (C) 2021,2020, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/
#pragma once
#include "CommandSender.h"
#include "dma/HostBufferManager.h"
#include "runtime/Types.h"
#include <cstddef>
#include <hostUtils/threadPool/ThreadPool.h>
#include <stdint.h>
#include <vector>

namespace threadPool {
class ThreadPool;
}
namespace rt {
class StreamManager;
class EventManager;

enum class MemcpyType { H2D, D2H };
struct MemcpyCommandBuilder {

  void addOp(const std::byte* hostAddr, const std::byte* deviceAddr, size_t size);
  void setTagId(rt::EventId eventId);

  explicit MemcpyCommandBuilder(MemcpyType type, bool barrierEnabled);

  std::vector<std::byte> build();

  void clear();

  uint32_t numEntries_ = 0;
  std::vector<std::byte> data_;
};
struct CommandsSentResult {
  std::vector<EventId> events_;
  std::vector<Command*> commands_;
};
struct DmaBufferInfo {
  DmaBufferInfo(std::byte* ptr, size_t size)
    : ptr_(ptr)
    , size_(size) {
  }
  std::byte* ptr_;
  size_t size_;
};

inline std::vector<DmaBufferInfo> getDmaBufferInfo(const std::vector<HostAllocation>& hostAllocations) {
  std::vector<DmaBufferInfo> res;
  for (const auto& ha : hostAllocations) {
    res.emplace_back(ha.getPtr(), ha.getSize());
  }
  return res;
}
CommandsSentResult prepareAndSendCommands(MemcpyType memcpyType, bool barrierEnabled, StreamId stream,
                                          StreamManager* streamManager, EventManager* eventManager,
                                          const std::vector<DmaBufferInfo>& stageBuffers, CommandSender* commandSender,
                                          const std::byte* devicePtr, bool enableCommands, size_t maxSize);

void doStagedCopyAndEnableCommands(threadPool::ThreadPool* threadPool, EventManager* eventManager, std::byte* hostPtr,
                                   std::vector<DmaBufferInfo> stageBuffers, std::vector<Command*> commands,
                                   std::optional<EventId> syncEvent, MemcpyType type, size_t maxSize);
} // namespace rt
