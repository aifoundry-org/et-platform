/*-------------------------------------------------------------------------
 * Copyright (c) 2025 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------*/
#pragma once

#include <exception>
#include <hostUtils/debug/StackException.h>
#include <runtime/IRuntimeExport.h>

namespace rt {
class ETRT_API NetworkException : public dbg::StackException {
  using dbg::StackException::StackException;
};
} // namespace rt