/*-------------------------------------------------------------------------
 * Copyright (c) 2025 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------*/
#pragma once
#include <g3log/loglevels.hpp>
#include <hostUtils/logging/Logger.h>
#include <hostUtils/logging/Logging.h>

#define AL_LOG(severity) ET_LOG(THREADPOOL, severity)
#define AL_DLOG(severity) ET_DLOG(THREADPOOL, severity)
#define AL_VLOG(severity) ET_VLOG(THREADPOOL, severity)
#define AL_LOG_IF(severity, condition) ET_LOG_IF(THREADPOOL, severity, condition)