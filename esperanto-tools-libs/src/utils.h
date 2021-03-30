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
#include <chrono>
#include <glog/logging.h>
#include <iomanip>
#include <string>
namespace rt {
template <typename Container, typename Key> auto find(Container&& c, Key&& k, std::string error = "Not found") {
  auto it = c.find(k);
  if (it == end(c)) {
    throw Exception(std::move(error));
  }
  return it;
}

inline auto getTime() {
  auto now = std::chrono::system_clock::now();
  auto t_c = std::chrono::system_clock::to_time_t(now);
  return std::put_time(std::localtime(&t_c), "%F %T");
}
enum VERBOSITY { LOW = 0, MID = 5, HIGH = 10 };
} // namespace rt

#define ET_LOG(channel, severity) LOG(severity) << "ET [" << #channel << "]: "
#define ET_DLOG(channel, severity) DLOG(severity) << "ET [" << #channel << "]: "
#define ET_VLOG(channel, severity) VLOG(severity) << "ET [" << #channel << "]: "

#define RT_LOG(severity) ET_DLOG(RUNTIME, severity)
#define RT_DLOG(severity) ET_DLOG(RUNTIME, severity)
#define RT_VLOG(severity) ET_VLOG(RUNTIME, severity)
