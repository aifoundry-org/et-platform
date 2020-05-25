/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#include "emu_gio.h"
#include "esrs.h"
#include "processor.h"
#ifdef SYS_EMU
#include "sys_emu.h"
#endif

namespace bemu {


// Fast local barriers can be accessed through UC to do stores and loads,
// and also through the CSR that implement the fast local barrier function.
uint64_t write_flb(const Hart& cpu, uint64_t value)
{
    unsigned barrier = value & 0x1F;
    unsigned limit   = (value >> 5) & 0xFF;

    unsigned shire  = shire_index(cpu);
    unsigned oldval = shire_other_esrs[shire].fast_local_barrier[barrier];

    LOG_AGENT(DEBUG, cpu, "S%u:fast_local_barrier%u : 0x%x", unsigned(cpu.shireid()), barrier, oldval);

#ifdef SYS_EMU
    if (sys_emu::get_flb_check())
        sys_emu::get_flb_checker().access(oldval, limit, barrier, hart_index(cpu));
#endif

    unsigned newval = (oldval == limit) ? 0 : (oldval + 1);
    LOG_AGENT(DEBUG, cpu, "S%u:fast_local_barrier%u = 0x%x", unsigned(cpu.shireid()), barrier, newval);
    shire_other_esrs[shire].fast_local_barrier[barrier] = newval;
    return (newval == 0);
}


} // namespace bemu
