/*-------------------------------------------------------------------------
 * Copyright (c) 2025 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------*/

#pragma once
#include <hostUtils/logging/Logging.h>

#define DV_LOG(severity) ET_LOG(DEVICE_LAYER, severity)
#define DV_DLOG(severity) ET_DLOG(DEVICE_LAYER, severity)
#define DV_VLOG(level) ET_VLOG(DEVICE_LAYER, level)

template <typename T> void unused(T t) {
  (void)t;
}

template <typename T, typename... Args> void unused(T t, Args... args) {
  unused(t);
  unused(args...);
}