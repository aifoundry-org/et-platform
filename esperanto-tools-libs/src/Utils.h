/*-------------------------------------------------------------------------
 * Copyright (c) 2025 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------*/

#pragma once
#include <assert.h>
#include <chrono>
#include <hostUtils/logging/Logger.h>
#include <hostUtils/logging/Logging.h>
#include <runtime/Types.h>
#include <string>
namespace rt {
inline std::string stringizeEvents(const std::vector<EventId>& events) {
  std::stringstream ss;
  ss << "Events: ";
  for (auto e : events) {
    ss << int(e) << " ";
  }
  return ss.str();
}

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
#define RT_LOG(severity) ET_LOG(RUNTIME, severity)                             /*NOSONAR*/
#define RT_DLOG(severity) ET_DLOG(RUNTIME, severity)                           /*NOSONAR*/
#define RT_VLOG(severity) ET_VLOG(RUNTIME, severity)                           /*NOSONAR*/
#define RT_LOG_IF(severity, condition) ET_LOG_IF(RUNTIME, severity, condition) /*NOSONAR*/

rt::DeviceErrorCode convert(int responseType, uint32_t responseCode);

template <typename Mutex> struct SpinLock : public std::unique_lock<Mutex> {
  explicit SpinLock(Mutex& mutex, std::chrono::milliseconds spinTime = std::chrono::milliseconds(10))
    : std::unique_lock<Mutex>{mutex, std::defer_lock} {
    auto start = std::chrono::high_resolution_clock::now();
    while (!this->try_lock()) {
      if (std::chrono::high_resolution_clock::now() - start > spinTime) {
        RT_VLOG(HIGH) << "Couldn't lock in " << spinTime.count() << "ms. Now go to regular lock.";
        this->lock();
        break;
      }
    }
  }
};