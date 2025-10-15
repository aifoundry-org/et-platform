/*-------------------------------------------------------------------------
 * Copyright (c) 2025 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------*/
#pragma once
#include "CommandSender.h"
#include "runtime/Types.h"
#include <cstddef>
#include <stdint.h>
#include <vector>

namespace rt {

enum class MemcpyType { H2D, D2H };
struct MemcpyCommandBuilder {

  void addOp(const std::byte* hostAddr, const std::byte* deviceAddr, size_t size);
  void setTagId(rt::EventId eventId);

  explicit MemcpyCommandBuilder(MemcpyType type, bool barrierEnabled, uint32_t maxEntries);

  std::vector<std::byte> build();

  void clear();

  uint32_t numEntries_ = 0;
  uint32_t maxEntries_ = 0;
  std::vector<std::byte> data_;
};

} // namespace rt
