/*-------------------------------------------------------------------------
* Copyright (C) 2021, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/
#include "debug/StackException.h"
#include <g3log/crashhandler.hpp>
using namespace dbg;

StackException::StackException(const std::string& message) 
  : decoratedMessage_("Exception message:" + message + "\nStackTrace:\n" + g3::internal::stackdump()) {
}

const char* StackException::what() const noexcept {
  return decoratedMessage_.c_str();
}

