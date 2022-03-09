/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#include "emu_defines.h"
#include "emu_gio.h"
#include "insn.h"
#include "insn_func.h"
#include "insn_util.h"
#include "log.h"
#include "processor.h"
#include "utility.h"

namespace bemu {


static bool ebreak_should_break(const Hart& cpu)
{
    return cpu.dcsr & (1ull << (static_cast<int>(cpu.prv) + 12));
}


void insn_c_ebreak(Hart& cpu)
{
    DISASM_NOARG("c.ebreak");

    if (cpu.debug_mode) {
        cpu.exit_progbuf(Hart::Progbuf::ok);
        return;
    }

    if (ebreak_should_break(cpu)) {
        throw Debug_entry(Debug_entry::Cause::ebreak);
    }

    // The spec says that hardware breakpoint sets mtval/stval to the current
    // PC but ebreak is a software breakpoint; should it also set mtval/stval
    // to the current PC or set it to 0?
    throw trap_breakpoint(PC);
}


void insn_ebreak(Hart& cpu)
{
    DISASM_NOARG("ebreak");

    if (cpu.debug_mode) {
        cpu.exit_progbuf(Hart::Progbuf::ok);
        return;
    }

    if (ebreak_should_break(cpu)) {
        throw Debug_entry(Debug_entry::Cause::ebreak);
    }

    // The spec says that hardware breakpoint sets mtval/stval to the current
    // PC but ebreak is a software breakpoint; should it also set mtval/stval
    // to the current PC or set it to 0?
    throw trap_breakpoint(PC);
}


void insn_ecall(Hart& cpu)
{
    DISASM_NOARG("ecall");

    switch (PRV) {
    case Privilege::U: throw trap_user_ecall();
    case Privilege::S: throw trap_supervisor_ecall();
    case Privilege::M: throw trap_machine_ecall();
    }
}


void insn_mret(Hart& cpu)
{
    DISASM_NOARG("mret");

    if (PRV != Privilege::M)
        throw trap_illegal_instruction(cpu.inst.bits);

    // Invalidate the fetch buffer when changing VM mode or permissions
    cpu.fetch_pc = -1;

    // Take mpie and mpp
    uint64_t mstatus = cpu.mstatus;
    uint64_t mpie = (mstatus >> 7) & 0x1;
    Privilege mpp = Privilege((mstatus >> 11) & 0x3);

    // Set mie = mpie, mpie = 1, mpp = U (0)
    mstatus = (mstatus & 0xFFFFFFFFFFFFE777ULL) | (mpie << 3) | (1 << 7);
    cpu.mstatus = mstatus;
    LOG_MSTATUS("=", mstatus);

    // Set prv = mpp
    cpu.set_prv(mpp);
    LOG_PRV("=", mpp);

    // Update PC
    WRITE_PC(cpu.mepc);
}


void insn_sfence_vma(Hart& cpu)
{
    DISASM_RS1_RS2("sfence.vma");

    throw trap_mcode_instruction(cpu.inst.bits);
}


void insn_sret(Hart& cpu)
{
    DISASM_NOARG("sret");

    Privilege curprv = PRV;
    uint64_t mstatus = cpu.mstatus;
    if (curprv == Privilege::U || (curprv == Privilege::S && (((mstatus >> 22) & 1) == 1)))
        throw trap_illegal_instruction(cpu.inst.bits);

    // Invalidate the fetch buffer when changing VM mode or permissions
    cpu.fetch_pc = -1;

    // Take spie and spp
    uint64_t spie = (mstatus >> 5) & 0x1;
    Privilege spp = Privilege((mstatus >> 8) & 0x1);

    // Clean sie, spie and spp
    // Set sie = spie, spie = 1, spp = U (0)
    mstatus = (mstatus & 0xFFFFFFFFFFFFFEDDULL) | (spie << 1) | (1 << 5);
    cpu.mstatus = mstatus;
    LOG_MSTATUS("=", mstatus);

    // Set prv = spp
    cpu.set_prv(spp);
    LOG_PRV("=", spp);

    // Update PC
    WRITE_PC(cpu.sepc);
}


void insn_wfi(Hart& cpu)
{
    DISASM_NOARG("wfi");

    Privilege curprv = PRV;
    uint64_t mstatus = cpu.mstatus;

    if (curprv == Privilege::U || (curprv == Privilege::S && (((mstatus >> 21) & 1) == 1))) {
        throw trap_illegal_instruction(cpu.inst.bits);
    }

    // WFI will not go to sleep in exclusive mode or when there are pending
    // interrupts, even when they are globally disabled (but they will ignore
    // pending interrupts if they are locally disabled).
    if (!cpu.core->excl_mode) {
        if (((cpu.mip | cpu.ext_seip) & cpu.mie) == 0) {
            cpu.start_waiting(Hart::Waiting::interrupt);
        }
    }
}


} // namespace bemu
