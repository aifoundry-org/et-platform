/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

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
