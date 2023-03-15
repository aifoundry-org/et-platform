/*-------------------------------------------------------------------------
 * Copyright (C) 2021, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/
#pragma once
#include <g3log/loglevels.hpp>
#include <logging/Logger.h>
#include <logging/Logging.h>

#define AL_LOG(severity) ET_LOG(THREADPOOL, severity)
#define AL_DLOG(severity) ET_DLOG(THREADPOOL, severity)
#define AL_VLOG(severity) ET_VLOG(THREADPOOL, severity)
#define AL_LOG_IF(severity, condition) ET_LOG_IF(THREADPOOL, severity, condition)