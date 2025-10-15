/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/
/*! \file log_internal.h
    \brief A C header that defines the defines used for enabling logging
    in et-common-libs.
*/
/***********************************************************************/
#ifndef _LOG_INTERNAL_H_
#define _LOG_INTERNAL_H_

#if defined(SP_RT)

#include "etsoc/common/log_common.h"
extern int32_t Log_Write(log_level_t level, const char *const fmt, ...)
    __attribute__((format(printf, 2, 3)));

#define LOG_FROM "**SP**"

#endif /* defined(SP_RT) */

#if defined(MM_RT)

#include "etsoc/common/log_common.h"
extern int32_t __Log_Write(log_level_t level, const char *const fmt, ...)
    __attribute__((format(printf, 2, 3)));

extern log_level_t Log_Get_Level(void);

#define Log_Write(level, fmt, ...)                  \
    do                                              \
    {                                               \
        if (level <= Log_Get_Level())               \
            __Log_Write(level, fmt, ##__VA_ARGS__); \
    } while (0)

#define LOG_FROM "**MM**"

#endif /* defined(MM_RT) */

#endif /* _LOG_INTERNAL_H_ */
