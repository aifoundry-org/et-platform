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
#include "emu_gio.h"
#include "insn.h"
#include "insn_func.h"
#include "log.h"
#include "processor.h"
#include "utility.h"

// FIXME: Replace with "processor.h"
#include "emu_defines.h"
extern std::array<Processor,EMU_NUM_THREADS> cpu;
extern uint64_t current_pc;

//namespace bemu {


#if 0
void insn_csrrc      (insn_t);
void insn_csrrci     (insn_t);
void insn_csrrs      (insn_t);
void insn_csrrsi     (insn_t);
void insn_csrrw      (insn_t);
void insn_csrrwi     (insn_t);
#endif

void insn_c_ebreak(insn_t /*inst*/)
{
    DISASM_NOARG("c.ebreak");

    // The spec says that hardware breakpoint sets mtval/stval to the current
    // PC but ebreak is a software breakpoint; should it also set mtval/stval
    // to the current PC or set it to 0?
    throw trap_breakpoint(current_pc);
}


void insn_ebreak(insn_t /*inst*/)
{
    DISASM_NOARG("ebreak");

    // The spec says that hardware breakpoint sets mtval/stval to the current
    // PC but ebreak is a software breakpoint; should it also set mtval/stval
    // to the current PC or set it to 0?
    throw trap_breakpoint(current_pc);
}


void insn_ecall(insn_t /*inst*/)
{
    DISASM_NOARG("ecall");

    switch (PRV) {
    case PRV_U: throw trap_user_ecall(); break;
    case PRV_S: throw trap_supervisor_ecall(); break;
    case PRV_M: throw trap_machine_ecall(); break;
    }
}


void insn_mret(insn_t inst)
{
    DISASM_NOARG("mret");

    if (PRV != PRV_M)
        throw trap_illegal_instruction(inst.bits);

    // Take mpie and mpp
    uint64_t mstatus = cpu[current_thread].mstatus;
    uint64_t mpie = (mstatus >> 7) & 0x1;
    prv_t    mpp = prv_t((mstatus >> 11) & 0x3);

    // Set mie = mpie, mpie = 1, mpp = U (0)
    mstatus = (mstatus & 0xFFFFFFFFFFFFE777ULL) | (mpie << 3) | (1 << 7);
    cpu[current_thread].mstatus = mstatus;
    LOG_MSTATUS("=", mstatus);

    // Set prv = mpp
    set_prv(cpu[current_thread], mpp);
    LOG_PRV("=", mpp);

    // Update PC
    log_pc_update(cpu[current_thread].mepc);
}


void insn_sfence_vma(insn_t inst)
{
    DISASM_RS1_RS2("sfence.vma");

    throw trap_mcode_instruction(inst.bits);
}


void insn_sret(insn_t inst)
{
    DISASM_NOARG("sret");

    uint64_t curprv = PRV;
    uint64_t mstatus = cpu[current_thread].mstatus;
    if (curprv == PRV_U || (curprv == PRV_S && (((mstatus >> 22) & 1) == 1)))
        throw trap_illegal_instruction(inst.bits);

    // Take spie and spp
    uint64_t spie = (mstatus >> 5) & 0x1;
    prv_t    spp = prv_t((mstatus >> 8) & 0x1);

    // Clean sie, spie and spp
    // Set sie = spie, spie = 1, spp = U (0)
    mstatus = (mstatus & 0xFFFFFFFFFFFFFEDDULL) | (spie << 1) | (1 << 5);
    cpu[current_thread].mstatus = mstatus;
    LOG_MSTATUS("=", mstatus);

    // Set prv = spp
    set_prv(cpu[current_thread], spp);
    LOG_PRV("=", spp);

    // Update PC
    log_pc_update(cpu[current_thread].sepc);
}


void insn_wfi(insn_t inst)
{
    DISASM_NOARG("wfi");

    uint64_t curprv = PRV;
    uint64_t mstatus = cpu[current_thread].mstatus;
    if (curprv == PRV_U || (curprv == PRV_S && (((mstatus >> 21) & 1) == 1)))
        throw trap_illegal_instruction(inst.bits);
}


//} // namespace bemu
