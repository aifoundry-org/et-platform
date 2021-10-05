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
#include "processor.h"
#include "traps.h"
#include "utility.h"

namespace bemu {


static inline void set_mip_bit(Hart& cpu, int cause)
{
    if ((cpu.mip & (1ULL << cause)) == 0) {
        LOG_HART(DEBUG, cpu, "Raising interrupt number %d", cause);
    }
    cpu.mip |= (1ULL << cause);
}


static inline void clear_mip_bit(Hart& cpu, int cause)
{
    if ((cpu.mip & (1ULL << cause)) != 0) {
        LOG_HART(DEBUG, cpu, "Clearing interrupt number %d", cause);
    }
    cpu.mip &= ~(1ULL << cause);
}


static inline void set_ext_seip(Hart& cpu)
{
    if ((cpu.ext_seip & (1ULL << SUPERVISOR_EXTERNAL_INTERRUPT)) == 0) {
        LOG_HART(DEBUG, cpu, "Raising external interrupt number %d", SUPERVISOR_EXTERNAL_INTERRUPT);
    }
    cpu.ext_seip |= (1ULL << SUPERVISOR_EXTERNAL_INTERRUPT);
}


static inline void clear_ext_seip(Hart& cpu)
{
    if ((cpu.ext_seip & (1ULL << SUPERVISOR_EXTERNAL_INTERRUPT)) != 0) {
        LOG_HART(DEBUG, cpu, "Clearing external interrupt number %d", SUPERVISOR_EXTERNAL_INTERRUPT);
    }
    cpu.ext_seip &= ~(1ULL << SUPERVISOR_EXTERNAL_INTERRUPT);
}


void raise_interrupt(Hart& cpu, int cause, uint64_t mip, uint64_t mbusaddr)
{
    if ((cause == SUPERVISOR_EXTERNAL_INTERRUPT) && ((mip & (1ULL << SUPERVISOR_EXTERNAL_INTERRUPT)) == 0)) {
        set_ext_seip(cpu);
    } else {
        set_mip_bit(cpu, cause);
        if (cause == BUS_ERROR_INTERRUPT) {
            cpu.mbusaddr = zextPA(mbusaddr);
        }
    }
}


void raise_software_interrupt(Hart& cpu)
{
    set_mip_bit(cpu, MACHINE_SOFTWARE_INTERRUPT);
}


void clear_software_interrupt(Hart& cpu)
{
    clear_mip_bit(cpu, MACHINE_SOFTWARE_INTERRUPT);
}


void raise_timer_interrupt(Hart& cpu)
{
    set_mip_bit(cpu, MACHINE_TIMER_INTERRUPT);
}


void clear_timer_interrupt(Hart& cpu)
{
    clear_mip_bit(cpu, MACHINE_TIMER_INTERRUPT);
}


void raise_external_machine_interrupt(Hart& cpu)
{
    set_mip_bit(cpu, MACHINE_EXTERNAL_INTERRUPT);
}


void clear_external_machine_interrupt(Hart& cpu)
{
    clear_mip_bit(cpu, MACHINE_EXTERNAL_INTERRUPT);
}


void raise_external_supervisor_interrupt(Hart& cpu)
{
    set_ext_seip(cpu);
}


void clear_external_supervisor_interrupt(Hart& cpu)
{
    clear_ext_seip(cpu);
}


void raise_bus_error_interrupt(Hart& cpu, uint64_t busaddr)
{
    set_mip_bit(cpu, BUS_ERROR_INTERRUPT);
    cpu.mbusaddr = zextPA(busaddr);
}


void clear_bus_error_interrupt(Hart& cpu)
{
    clear_mip_bit(cpu, BUS_ERROR_INTERRUPT);
}


} // namespace bemu
