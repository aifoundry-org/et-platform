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
#define ET_LOG(channel, severity)                                                                                      \
  LOG(severity) << "ET [" << #channel << "]: [TH: " << std::hex << std::this_thread::get_id() << "]: "
#define ET_DLOG(channel, severity)                                                                                     \
  LOG(DEBUG) << "ET [" << #channel << "]: [TH: " << std::hex << std::this_thread::get_id() << "]: "
#define CAT_NX(A, B) A ## B
#define ET_VLOG(channel, level)                                                                                        \
  LOG(CAT_NX(logging::VLOG_, level)) << "ET [" << #channel << "][TH: " << std::hex << std::this_thread::get_id()       \
                                     << "]: "