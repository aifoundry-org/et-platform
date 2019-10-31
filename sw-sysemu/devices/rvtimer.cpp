/* vim: set sw=4 et sta cin cino=\:0s,l1,g0,N-s,E-s,i0,+2s,(0,W2s : */

#include "devices/rvtimer.h"
#ifdef SYS_EMU
#include "sys_emu.h"
#endif

void RVTimer::update(uint64_t cycle)
{
    if ((cycle % tick_freq) != 0)
        return;

    if (++mtime >= mtimecmp) {
        if (!interrupt) {
#ifdef SYS_EMU
            sys_emu::raise_timer_interrupt((1ULL << EMU_NUM_SHIRES) - 1);
#endif
            interrupt = true;
        }
    }
}

void RVTimer::write_mtimecmp(uint64_t val)
{
    bool had_interrupt = interrupt;
    mtimecmp = val;
    interrupt = (mtime >= mtimecmp);
    if (had_interrupt && !interrupt) {
#ifdef SYS_EMU
        sys_emu::clear_timer_interrupt((1ULL << EMU_NUM_SHIRES) - 1);
#endif
    }
}
