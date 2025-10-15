/*-------------------------------------------------------------------------
 * Copyright (c) 2025 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------*/

#pragma once
#include "MemoryManager.h"
#include "runtime/Types.h"
#include <set>
#include <unordered_map>

namespace rt {
class RuntimeImp;
class CoreDumper {
public:
  using AllocationInfo = MemoryManager::AllocationInfo;
  void addKernelExecution(const std::string& coreDumpPath, KernelId kernelId, EventId eventId);
  void removeKernelExecution(EventId eventId);
  void addCodeAddress(DeviceId device, std::byte* address);
  void removeCodeAddress(DeviceId device, std::byte* address);
  void dump(EventId eventId, const std::vector<AllocationInfo>& allocations, const rt::StreamError& error,
            RuntimeImp& runtime);

private:
  struct KernelExecution {
    KernelId kernelId_;
    std::string coreDumpPath_;
  };

  std::unordered_map<DeviceId, std::set<std::byte*>> codeAddresses_; // store all code addresses
  std::unordered_map<EventId, KernelExecution> kernelExecutions_;
};
} // namespace rt