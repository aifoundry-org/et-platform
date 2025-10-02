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

#include "DefaultSinks.h"
#include "Logger.h"
#include <thread>

#define ET_MSG(channel) "ET [" << #channel << "][TH:" << std::this_thread::get_id() << "]: "

#define ET_LOG(channel, severity) LOG(severity) << ET_MSG(channel)
#define ET_DLOG(channel, severity) LOG(DEBUG) << ET_MSG(channel)
#define CAT_NX(A, B) A##B
#define ET_VLOG(channel, level) LOG(CAT_NX(logging::VLOG_, level)) << ET_MSG(channel)
#define ET_LOG_IF(channel, severity, condition) LOG_IF(severity, condition) << ET_MSG(channel)