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
#include <assert.h>
#include <hostUtils/logging/Logging.h>
#include <runtime/Types.h>
#include <string>
namespace rt {
template <typename Container, typename Key> auto find(Container& c, Key&& k, const std::string& error = "Not found") {
  auto it = c.find(std::forward<Key>(k));
  if (it == end(c)) {
    throw Exception(error);
  }
  return it;
}
} // namespace rt

template <typename T, typename U> T align(T address, U alignment) {
  auto one = static_cast<T>(1);
  auto align = static_cast<T>(alignment);
  assert((align & one) == 0);
  return (address + (align - one)) & ~(align - one);
}

template <typename T> void unused(T t) {
  (void)t;
}

template <typename T, typename... Args> void unused(T t, Args... args) {
  unused(t);
  unused(args...);
}

#define RT_LOG(severity) ET_LOG(RUNTIME, severity)
#define RT_DLOG(severity) ET_DLOG(RUNTIME, severity)
#define RT_VLOG(severity) ET_VLOG(RUNTIME, severity)
#define RT_LOG_IF(severity, condition) ET_LOG_IF(RUNTIME, severity, condition)