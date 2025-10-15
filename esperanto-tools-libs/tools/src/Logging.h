/*-------------------------------------------------------------------------
 * Copyright (c) 2025 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------*/
#pragma once
#define BM_LOG(severity) ET_LOG(BENCHMARK, severity)
#define BM_DLOG(severity) ET_DLOG(BENCHMARK, severity)
#define BM_VLOG(severity) ET_VLOG(BENCHMARK, severity)
#define BM_LOG_IF(severity, condition) ET_LOG_IF(BENCHMARK, severity, condition)