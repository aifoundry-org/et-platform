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

// for sys_emu, to log only data from one minion
int32_t minion_only_log = -1;

void log_only_minion(int32_t m) {
    minion_only_log = (m >= 0 && m < EMU_NUM_MINIONS) ? m : -1;
}
