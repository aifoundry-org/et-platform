/*-------------------------------------------------------------------------
 * Copyright (C) 2022, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/
#pragma once
#define BM_LOG(severity) ET_LOG(BENCHMARK, severity)
#define BM_DLOG(severity) ET_DLOG(BENCHMARK, severity)
#define BM_VLOG(severity) ET_VLOG(BENCHMARK, severity)
#define BM_LOG_IF(severity, condition) ET_LOG_IF(BENCHMARK, severity, condition)