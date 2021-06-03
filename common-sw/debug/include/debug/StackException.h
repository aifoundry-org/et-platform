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
#include <exception>
#include <string>
namespace dbg {
class StackException : public std::exception {
public:
  explicit StackException(const std::string& message);
  const char* what() const noexcept override;

private:
  std::string decoratedMessage_;
};
}