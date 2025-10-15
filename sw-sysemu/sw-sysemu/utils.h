/*-------------------------------------------------------------------------
 * Copyright (c) 2025 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------*/

#pragma once
#include <glog/logging.h>
#include <iomanip>

#define ET_LOG(channel, severity) LOG(severity) << "ET [" << #channel << "]: "
#define ET_DLOG(channel, severity) DLOG(severity) << "ET [" << #channel << "]: "

#define SE_LOG(severity) ET_DLOG(SYS_EMU, severity)
#define SE_DLOG(severity) ET_DLOG(SYS_EMU, severity)