/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------*/
#include "hostUtils/logging/Instance.h"
#include "hostUtils/logging/Logger.h"

#include <g3log/loglevels.hpp>

#include <optional>


using namespace logging;

#define INSTANCE_LEVEL(LVL)                                                                                     \
  case Instance::LEVEL::LVL: {                                                                                  \
    return LVL;                                                                                                     \
  }

std::optional<LEVELS> translateLogLevel(const Instance::LEVEL lvl) {
  switch (lvl) {
  INSTANCE_LEVEL(FATAL)
  INSTANCE_LEVEL(WARNING)
  INSTANCE_LEVEL(INFO)
  INSTANCE_LEVEL(DEBUG)
  default:
    return std::nullopt;
  }
}

Instance::Instance() {
  logger_ = std::make_unique<LoggerDefault>();
}
Instance::~Instance() = default;

void Instance::setEnable(LEVEL lvl, bool enable) {
  auto level = translateLogLevel(lvl);
  if (level.has_value()) {
    if (enable) {
      g3::log_levels::enable(level.value());
    } else {
      g3::log_levels::disable(level.value());
    }
  }
}