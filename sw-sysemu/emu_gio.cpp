/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------*/

#include <cstdarg>
#include <cstdio>

#include "emu_defines.h"
#include "emu_gio.h"
#include "system.h"
#include "testLog.h"

namespace bemu {

void lprintf(logLevel level, const Agent& agent, const char* fmt, ...)
{
    assert(agent.chip);

    static thread_local char lbuf[4096] = { '\0' };
    va_list ap;
    va_start(ap, fmt);
    (void)vsnprintf(lbuf, 4096, fmt, ap);
    va_end(ap);

    auto& logger = agent.chip->log;
    logger << level << "[" << agent.name() << "] " << lbuf << endm;

    logger.dumpTraceBufferIfFatal(agent);
}

} // namespace bemu
