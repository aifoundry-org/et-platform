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


void insn_jal(insn_t inst)
{
    DISASM_RD_JIMM("jal");
    WRITE_RD(NPC);
    WRITE_PC(PC + JIMM);
}


void insn_jalr(insn_t inst)
{
    DISASM_RD_RS1_IIMM("jalr");
    uint64_t tmp = (RS1 + IIMM) & ~1ull;
    WRITE_RD(NPC);
    WRITE_PC(tmp);
}


void insn_beq(insn_t inst)
{
    DISASM_RS1_RS2_BIMM("beq");
    if (RS1 == RS2)
        WRITE_PC(PC + BIMM);
}


void insn_bge(insn_t inst)
{
    DISASM_RS1_RS2_BIMM("bge");
    if (int64_t(RS1) >= int64_t(RS2))
        WRITE_PC(PC + BIMM);
}


void insn_bgeu(insn_t inst)
{
    DISASM_RS1_RS2_BIMM("bgeu");
    if (RS1 >= RS2)
        WRITE_PC(PC + BIMM);
}


void insn_blt(insn_t inst)
{
    DISASM_RS1_RS2_BIMM("blt");
    if (int64_t(RS1) < int64_t(RS2))
        WRITE_PC(PC + BIMM);
}


void insn_bltu(insn_t inst)
{
    DISASM_RS1_RS2_BIMM("bltu");
    if (RS1 < RS2)
        WRITE_PC(PC + BIMM);
}


void insn_bne(insn_t inst)
{
    DISASM_RS1_RS2_BIMM("bne");
    if (RS1 != RS2)
        WRITE_PC(PC + BIMM);
}


//} // namespace bemu
