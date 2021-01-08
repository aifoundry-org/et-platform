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
#include <glog/logging.h>
#include <iomanip>

#define ET_LOG(channel, severity) LOG(severity) << "ET [" << #channel << "]: "
#define ET_DLOG(channel, severity) DLOG(severity) << "ET [" << #channel << "]: "

#define SE_LOG(severity) ET_DLOG(SYS_EMU, severity)
#define SE_DLOG(severity) ET_DLOG(SYS_EMU, severity)