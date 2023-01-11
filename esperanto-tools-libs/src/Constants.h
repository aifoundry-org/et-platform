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
#include "runtime/Types.h"

namespace rt {
// these constants are based on documentation:
// https://esperantotech.atlassian.net/wiki/spaces/SW/pages/1289355429/Device+Ops+Interface+-+Command+Response+Events+Bindings#Trace-setup-and-execution-flow
constexpr auto kNumErrorContexts = 2080;
constexpr auto kExceptionBufferSize = sizeof(ErrorContext) * kNumErrorContexts;
constexpr auto kNumExecutionCacheBuffers = 5;  // initial number of execution cache buffers
constexpr auto kAllocFactorTotalMaxMemory = 4; // this will affect the size of memory we allocate for CMA

constexpr auto kCmPrevExecutionPath = "./fw_trace_cm_last_execution";
constexpr auto kMmPrevExecutionPath = "./fw_trace_mm_last_execution";
} // namespace rt