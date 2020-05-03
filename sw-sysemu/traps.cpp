/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#include "decode.h"
#include "emu_defines.h"
#include "emu_gio.h"
#include "log.h"
#include "processor.h"
#ifdef SYS_EMU
#include "sys_emu.h"
#endif
#include "traps.h"
#include "utility.h"

namespace bemu {


extern std::array<Processor,EMU_NUM_THREADS> cpu;


#define set_mip_bit(thread, cause) do {  \
    if (~cpu[thread].mip & 1<<(cause)) \
        LOG(DEBUG, "Raising interrupt number %d", cause); \
    cpu[thread].mip |= 1<<(cause); \
} while (0)


#define clear_mip_bit(thread, cause) do {  \
    if (cpu[thread].mip & 1<<(cause)) \
        LOG(DEBUG, "Clearing interrupt number %d", cause); \
    cpu[thread].mip &= ~(1<<(cause)); \
} while (0)


#define set_ext_seip(thread) do { \
    if (cpu[thread].ext_seip & (1<<SUPERVISOR_EXTERNAL_INTERRUPT)) \
        LOG(DEBUG, "Raising external interrupt number %d", SUPERVISOR_EXTERNAL_INTERRUPT); \
    cpu[thread].ext_seip |= (1<<SUPERVISOR_EXTERNAL_INTERRUPT); \
} while (0)


#define clear_ext_seip(thread) do { \
    if (~cpu[thread].ext_seip & (1<<SUPERVISOR_EXTERNAL_INTERRUPT)) \
        LOG(DEBUG, "Clearing external interrupt number %d", SUPERVISOR_EXTERNAL_INTERRUPT); \
    cpu[thread].ext_seip &= ~(1<<SUPERVISOR_EXTERNAL_INTERRUPT); \
} while (0)


static void trap_to_smode(uint64_t cause, uint64_t val)
{
    // Get current privilege mode
    uint64_t curprv = PRV;
    bool interrupt = (cause & 0x8000000000000000ULL);
    int code = (cause & 63);
    assert(curprv <= PRV_S);

#ifdef SYS_EMU
    if (sys_emu::get_display_trap_info())
        LOG(INFO, "\tTrapping to S-mode with cause 0x%" PRIx64 " and tval 0x%" PRIx64, cause, val);
#else
    LOG(DEBUG, "\tTrapping to S-mode with cause 0x%" PRIx64 " and tval 0x%" PRIx64, cause, val);
#endif

    // if checking against RTL, clear the correspoding MIP bit it will be set
    // to 1 again if the pending bit was not really cleared just before
    // entering the interrupt again
    // TODO: you won't be able to read the MIP CSR from code or you'll get
    // checker errors (you would have errors if the interrupt is cleared by a
    // memory mapped store)
#ifndef SYS_EMU
    if (interrupt) {
        // Clear external supervisor interrupt
        if (code == 0x9 && !(cpu[current_thread].mip & 0x200)) {
            clear_external_supervisor_interrupt(current_thread);
            LOG(DEBUG, "%s", "\tClearing external supervisor interrupt");
        }
        cpu[current_thread].mip &= ~(1<<code);
    }
#endif

    // Take sie
    uint64_t mstatus = cpu[current_thread].mstatus;
    uint64_t sie = (mstatus >> 1) & 0x1;
    // Set spie = sie, sie = 0, spp = prv
    cpu[current_thread].mstatus = (mstatus & 0xFFFFFFFFFFFFFEDDULL) | (curprv << 8) | (sie << 5);
    // Set scause, stval and sepc
    cpu[current_thread].scause = cause & 0x800000000000001FULL;
    cpu[current_thread].stval = sextVA(val);
    cpu[current_thread].sepc = sextVA(PC & ~1ULL);
    // Jump to stvec
    set_prv(cpu[current_thread], PRV_S);

    // Throw an error if no one ever set stvec otherwise we'll enter an
    // infinite loop of illegal instruction exceptions
    if (cpu[current_thread].stvec_is_set == false)
        LOG(WARN, "%s", "Trap vector has never been set. Can't take exception properly");

    // compute address where to jump to
    uint64_t tvec = cpu[current_thread].stvec;
    if ((tvec & 1) && interrupt) {
        tvec += code * 4;
    }
    tvec &= ~0x1ULL;
    WRITE_PC(tvec);

    log_trap(cpu[current_thread].mstatus, cpu[current_thread].scause,
             cpu[current_thread].stval, cpu[current_thread].sepc);
}


static void trap_to_mmode(uint64_t cause, uint64_t val)
{
    // Get current privilege mode
    uint64_t curprv = PRV;
    bool interrupt = (cause & 0x8000000000000000ULL);
    int code = (cause & 63);

    // Check if we should deletegate the trap to S-mode
    if ((curprv < PRV_M) && ((interrupt ? cpu[current_thread].mideleg : cpu[current_thread].medeleg) & (1ull<<code))) {
        trap_to_smode(cause, val);
        return;
    }

    // if checking against RTL, clear the correspoding MIP bit it will be set
    // to 1 again if the pending bit was not really cleared just before
    // entering the interrupt again
    // TODO: you won't be able to read the MIP CSR from code or you'll get
    // checker errors (you would have errors if the interrupt is cleared by a
    // memory mapped store)
#ifndef SYS_EMU
    if (interrupt) {
        cpu[current_thread].mip &= ~(1<<code);
        // Clear external supervisor interrupt
        if (cause == 9 && !(cpu[current_thread].mip & 0x200)) {
            clear_external_supervisor_interrupt(current_thread);
        }
    }
#endif

#ifdef SYS_EMU
    if (sys_emu::get_display_trap_info())
        LOG(INFO, "\tTrapping to M-mode with cause 0x%" PRIx64 " and tval 0x%" PRIx64, cause, val);
#else
    LOG(DEBUG, "\tTrapping to M-mode with cause 0x%" PRIx64 " and tval 0x%" PRIx64, cause, val);
#endif

    // Take mie
    uint64_t mstatus = cpu[current_thread].mstatus;
    uint64_t mie = (mstatus >> 3) & 0x1;
    // Set mpie = mie, mie = 0, mpp = prv
    cpu[current_thread].mstatus = (mstatus & 0xFFFFFFFFFFFFE777ULL) | (curprv << 11) | (mie << 7);
    // Set mcause, mtval and mepc
    cpu[current_thread].mcause = cause & 0x800000000000001FULL;
    cpu[current_thread].mtval = sextVA(val);
    cpu[current_thread].mepc = sextVA(PC & ~1ULL);
    // Jump to mtvec
    set_prv(cpu[current_thread], PRV_M);

    // Throw an error if no one ever set mtvec otherwise we'll enter an
    // infinite loop of illegal instruction exceptions
    if (cpu[current_thread].mtvec_is_set == false)
        LOG(WARN, "%s", "Trap vector has never been set. Doesn't smell good...");

    // compute address where to jump to
    uint64_t tvec = cpu[current_thread].mtvec;
    if ((tvec & 1) && interrupt) {
        tvec += code * 4;
    }
    tvec &= ~0x1ULL;
    WRITE_PC(tvec);

    log_trap(cpu[current_thread].mstatus, cpu[current_thread].mcause,
             cpu[current_thread].mtval, cpu[current_thread].mepc);
}


void take_trap(const trap_t& t)
{
    trap_to_mmode(t.cause(), t.tval());
}


void raise_interrupt(int thread, int cause, uint64_t mip, uint64_t mbusaddr)
{
    if (cause == SUPERVISOR_EXTERNAL_INTERRUPT && !(mip & (1<<SUPERVISOR_EXTERNAL_INTERRUPT))) {
        set_ext_seip(thread);
    } else {
        set_mip_bit(thread, cause);
        if (cause == BUS_ERROR_INTERRUPT)
            cpu[thread].mbusaddr = zextPA(mbusaddr);
    }
}


void raise_software_interrupt(int thread)
{
    set_mip_bit(thread, MACHINE_SOFTWARE_INTERRUPT);
}


void clear_software_interrupt(int thread)
{
    clear_mip_bit(thread, MACHINE_SOFTWARE_INTERRUPT);
}


void raise_timer_interrupt(int thread)
{
    set_mip_bit(thread, MACHINE_TIMER_INTERRUPT);
}


void clear_timer_interrupt(int thread)
{
    clear_mip_bit(thread, MACHINE_TIMER_INTERRUPT);
}


void raise_external_machine_interrupt(int thread)
{
    set_mip_bit(thread, MACHINE_EXTERNAL_INTERRUPT);
}


void clear_external_machine_interrupt(int thread)
{
    clear_mip_bit(thread, MACHINE_EXTERNAL_INTERRUPT);
}


void raise_external_supervisor_interrupt(int thread)
{
    set_ext_seip(thread);
}


void clear_external_supervisor_interrupt(int thread)
{
    clear_ext_seip(thread);
}


void raise_bus_error_interrupt(int thread, uint64_t busaddr)
{
    set_mip_bit(thread, BUS_ERROR_INTERRUPT);
    cpu[thread].mbusaddr = zextPA(busaddr);
}


void clear_bus_error_interrupt(int thread)
{
    clear_mip_bit(thread, BUS_ERROR_INTERRUPT);
}


} // namespace bemu
