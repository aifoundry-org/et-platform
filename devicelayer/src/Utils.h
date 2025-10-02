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