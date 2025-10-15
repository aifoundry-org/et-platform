/*-------------------------------------------------------------------------
 * Copyright (c) 2025 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------*/
#pragma once
#include "StackException.h"
namespace dbg {
template <typename Condition, typename Message, typename TException = StackException>
void Check(Condition condition, Message&& msg) {
  if (!condition) {
    throw TException(std::forward<Message>(msg));
  }
}
} // namespace dbg