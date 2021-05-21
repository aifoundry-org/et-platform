/*-------------------------------------------------------------------------
 * Copyright (C) 2020, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/

#ifndef _EMU_GIO_H
#define _EMU_GIO_H

#include <bitset>
#include <cinttypes>

#include "emu_defines.h"
#include "processor.h"
#include "system.h"
#include "testLog.h"

//! Log for a given hart, if enabled.
#define LOG_HART(severity, hart, format, ...)                                  \
    do {                                                                       \
        assert((hart).chip);                                                   \
        if (LOG_##severity >= (hart).chip->log.getLogLevel()                   \
            && (hart).chip->log_thread[bemu::hart_index(hart)])                \
            bemu::lprintf(LOG_##severity, (hart), format, __VA_ARGS__);         \
    } while (0)

//! Log for a given agent.
#define LOG_AGENT(severity, agent, format, ...)                                \
    do {                                                                       \
        assert((agent).chip);                                                  \
        if (LOG_##severity >= (agent).chip->log.getLogLevel())                 \
            bemu::lprintf(LOG_##severity, (agent), format, __VA_ARGS__);       \
    } while (0)

namespace bemu
{

//! Format and print log message for a given agent.
void lprintf(logLevel level, const Agent& agent, const char* fmt, ...)
    __attribute__((format(printf, 3, 4)));

} // namespace bemu

#endif // _EMU_GIO_H
