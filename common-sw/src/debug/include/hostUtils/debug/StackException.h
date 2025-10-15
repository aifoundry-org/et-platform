/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------*/
#pragma once
#include <hostUtils/debug/DebugExport.h>

#include <exception>
#include <string>

namespace dbg {

class DEBUG_API StackException : public std::exception {
public:
  explicit StackException(const std::string& message);
  const char* what() const noexcept override;

private:
  std::string decoratedMessage_;
};

}