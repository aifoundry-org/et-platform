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
