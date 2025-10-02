/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
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
