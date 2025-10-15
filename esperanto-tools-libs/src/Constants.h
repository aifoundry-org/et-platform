/*-------------------------------------------------------------------------
 * Copyright (c) 2025 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------*/

#pragma once
#include "runtime/Types.h"

namespace rt {
// these constants are based on documentation:
// https://esperantotech.atlassian.net/wiki/spaces/SW/pages/1289355429/Device+Ops+Interface+-+Command+Response+Events+Bindings#Trace-setup-and-execution-flow
constexpr auto kNumErrorContexts = 2080;
constexpr auto kExceptionBufferSize = sizeof(ErrorContext) * kNumErrorContexts;
constexpr auto kNumExecutionCacheBuffers = 5;  // initial number of execution cache buffers
constexpr auto kAllocFactorTotalMaxMemory = 2; // this will affect the size of memory we allocate for CMA

constexpr auto kCmPrevExecutionPath = "./fw_trace_cm_last_execution";
constexpr auto kMmPrevExecutionPath = "./fw_trace_mm_last_execution";
} // namespace rt