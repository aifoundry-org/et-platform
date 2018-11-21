// g[s]printf functions for emu shared library used by checker in RTL tests

#include "emu_gio.h"

// from emu.h
extern uint32_t current_thread;

// for sys_emu, to log only data from one minion
int32_t minion_only_log = -1;

// print into this buffer before logging
thread_local char emu_log_buffer[4096] = {'\0'};


testLog& emu_log()
{
    static testLog l("EMU", LOG_INFO);
    static testLog l_filtered("EMU",LOG_ERR);
    if (minion_only_log >= 0 && int32_t(current_thread / EMU_THREADS_PER_MINION) != minion_only_log)
    {
        return l_filtered;
    }
    return l;
}
