/*-------------------------------------------------------------------------
 * Copyright (C) 2021, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
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