/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#include <cstdio>
#include <cstdarg>

#include "emu_gio.h"

namespace emu {

    testLog log("EMU", LOG_INFO);

    void lprintf(logLevel level, const char* fmt, ...)
    {
        static thread_local char lbuf[4096] = {'\0'};
        va_list ap;
        va_start(ap, fmt);
        (void) vsnprintf(lbuf, 4096, fmt, ap);
        va_end(ap);
        emu::log << level << lbuf << endm;
    }

}

std::bitset<EMU_NUM_THREADS> log_thread;

void log_set_threads(const std::bitset<EMU_NUM_THREADS> &threads)
{
    if (threads.count() == 0)
        log_thread.set();
    else
        log_thread = threads;
}
