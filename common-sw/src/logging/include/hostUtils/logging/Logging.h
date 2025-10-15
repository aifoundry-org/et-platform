/*-------------------------------------------------------------------------
 * Copyright (c) 2025 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------*/

 #pragma once

#include "DefaultSinks.h"
#include "Logger.h"
#include <thread>

#define ET_MSG(channel) "ET [" << #channel << "][TH:" << std::this_thread::get_id() << "]: "

#define ET_LOG(channel, severity) LOG(severity) << ET_MSG(channel)
#define ET_DLOG(channel, severity) LOG(DEBUG) << ET_MSG(channel)
#define CAT_NX(A, B) A##B
#define ET_VLOG(channel, level) LOG(CAT_NX(logging::VLOG_, level)) << ET_MSG(channel)
#define ET_LOG_IF(channel, severity, condition) LOG_IF(severity, condition) << ET_MSG(channel)