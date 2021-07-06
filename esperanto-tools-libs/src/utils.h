/*-------------------------------------------------------------------------
 * Copyright (C) 2020, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/

#pragma once
#include "runtime/IRuntime.h"
#include <hostUtils/logging/Logging.h>
#include <string>
namespace rt {
template <typename Container, typename Key> auto find(Container&& c, Key&& k, std::string error = "Not found") {
  auto it = c.find(std::forward<Key>(k));
  if (it == end(c)) {
    throw Exception(std::move(error));
  }
  return it;
}
} // namespace rt

#define RT_LOG(severity) ET_LOG(RUNTIME, severity)
#define RT_DLOG(severity) ET_DLOG(RUNTIME, severity)
#define RT_VLOG(severity) ET_VLOG(RUNTIME, severity)
