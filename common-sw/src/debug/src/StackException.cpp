/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------*/
#include "hostUtils/debug/StackException.h"
#include <g3log/crashhandler.hpp>
using namespace dbg;

StackException::StackException(const std::string& message) 
  : decoratedMessage_("Exception message:" + message + "\nStackTrace:\n" + g3::internal::stackdump()) {
}

const char* StackException::what() const noexcept {
  return decoratedMessage_.c_str();
}

