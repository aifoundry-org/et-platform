/*-------------------------------------------------------------------------
 * Copyright (c) 2025 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------*/

#ifndef _EMU_GIO_H
#define _EMU_GIO_H

#include <bitset>
#include <cinttypes>

#include "emu_defines.h"
#include "processor.h"
#include "system.h"
#include "testLog.h"

#ifdef SDK_RELEASE
// Only log in U-Mode if building for SDK release
#define HART_LOG_EN(hart) \
    (hart).chip->log_thread[bemu::hart_index(hart)] && ((hart).prv == bemu::Privilege::U)
#else
#define HART_LOG_EN(hart) \
    (hart).chip->log_thread[bemu::hart_index(hart)]
#endif

//! Log for a given hart, if enabled.
#define LOG_HART(severity, hart, format, ...)                           \
    do {                                                                \
        assert((hart).chip);                                            \
        if (LOG_##severity >= (hart).chip->log.getLogLevel()            \
            && HART_LOG_EN(hart))                                       \
            bemu::lprintf(LOG_##severity, (hart), format, __VA_ARGS__); \
    } while (0)

//! Log for a given agent.
#define LOG_AGENT(severity, agent, format, ...)                          \
    do {                                                                 \
        assert((agent).chip);                                            \
        if (LOG_##severity >= (agent).chip->log.getLogLevel())           \
            bemu::lprintf(LOG_##severity, (agent), format, __VA_ARGS__); \
    } while (0)

//! Log a warning/error for a given hart, depending on settings.
#define WARN_HART(category, hart, format, ...)                        \
    do {                                                              \
        assert((hart).chip);                                          \
        const logLevel severity                                       \
            = (hart).chip->warning.severity(bemu::Warning::category); \
        if (severity >= (hart).chip->log.getLogLevel()                \
            && HART_LOG_EN(hart))                                     \
            bemu::lprintf(severity, (hart), format, __VA_ARGS__);     \
    } while (0)

//! Log a warning/error for a given agent, depending on settings.
#define WARN_AGENT(category, agent, format, ...)                       \
    do {                                                               \
        assert((agent).chip);                                          \
        const logLevel severity                                        \
            = (agent).chip->warning.severity(bemu::Warning::category); \
        if (severity >= (agent).chip->log.getLogLevel())               \
            bemu::lprintf(severity, (agent), format, __VA_ARGS__);     \
    } while (0)

namespace bemu {

//! Format and print log message for a given agent.
void lprintf(logLevel level, const Agent& agent, const char* fmt, ...)
    __attribute__((format(printf, 3, 4)));

} // namespace bemu

#endif // _EMU_GIO_H
