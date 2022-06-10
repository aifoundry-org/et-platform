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
