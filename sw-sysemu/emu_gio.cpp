// g[s]printf functions for emu shared library used by checker in RTL tests

#include "emu_gio.h"

// for sys_emu, to log only data from one minion
int32_t minion_only_log = -1;

// print into this buffer before logging
thread_local char emu_log_buffer[4096] = {'\0'};


testLog& emu_log()
{
    static testLog l("EMU", LOG_INFO);
    return l;
}


void log_only_minion(int32_t m) {
    minion_only_log = (m >= 0 && m < EMU_NUM_MINIONS) ? m : -1;
}
