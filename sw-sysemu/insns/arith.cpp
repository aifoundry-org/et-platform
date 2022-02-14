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

#ifdef SYS_EMU
#include "sys_emu.h"
#endif

namespace bemu {


void insn_reserved(Hart& cpu)
{
    DISASM_NOARG("illegal opcode");
    throw trap_illegal_instruction(cpu.inst.bits);
}


void insn_add(Hart& cpu)
{
    DISASM_RD_RS1_RS2("add");
    WRITE_RD(RS1 + RS2);
}


void insn_addi(Hart& cpu)
{
    DISASM_RD_RS1_IIMM("addi");
    WRITE_RD(RS1 + IIMM);
}


void insn_addiw(Hart& cpu)
{
    DISASM_RD_RS1_IIMM("addiw");
    WRITE_RD(sext<32>(RS1 + IIMM));
}


void insn_addw(Hart& cpu)
{
    DISASM_RD_RS1_RS2("addw");
    WRITE_RD(sext<32>(RS1 + RS2));
}


void insn_and(Hart& cpu)
{
    DISASM_RD_RS1_RS2("and");
    WRITE_RD(RS1 & RS2);
}


void insn_andi(Hart& cpu)
{
    DISASM_RD_RS1_IIMM("andi");
    WRITE_RD(RS1 & IIMM);
}


void insn_auipc(Hart& cpu)
{
    DISASM_RD_UIMM("auipc");
    LOG_PC(":");
    WRITE_RD(PC + UIMM);
}


void insn_lui(Hart& cpu)
{
    DISASM_RD_UIMM("lui");
    WRITE_RD(UIMM);
}


void insn_or(Hart& cpu)
{
    DISASM_RD_RS1_RS2("or");
    WRITE_RD(RS1 | RS2);
}


void insn_ori(Hart& cpu)
{
    DISASM_RD_RS1_IIMM("ori");
    WRITE_RD(RS1 | IIMM);
}


void insn_sll(Hart& cpu)
{
    DISASM_RD_RS1_RS2("sll");
    WRITE_RD(RS1 << (RS2 % 64));
}


void insn_slli(Hart& cpu)
{
    DISASM_RD_RS1_SHAMT6("slli");
    WRITE_RD(RS1 << SHAMT6);
}


void insn_slliw(Hart& cpu)
{
    DISASM_RD_RS1_SHAMT5("slliw");
    WRITE_RD(sext<32>(RS1 << SHAMT5));
}

void insn_sllw(Hart& cpu)
{
    DISASM_RD_RS1_RS2("sllw");
    WRITE_RD(sext<32>(RS1 << (RS2 % 32)));
}


void insn_slt(Hart& cpu)
{
    DISASM_RD_RS1_RS2("slt");
    WRITE_RD(int64_t(RS1) < int64_t(RS2));
}


void insn_slti(Hart& cpu)
{
    DISASM_RD_RS1_IIMM("slti");
    WRITE_RD(int64_t(RS1) < IIMM);
#ifdef SYS_EMU
    if (cpu.inst.rs1() || cpu.inst.rd()) return;
    // SW hints are encoded as `slti x0,x0,hint`.
    // For a reference of available hints, see [Interface with DV environment][1].
    // Note: This is a late addition, so most of these hints are not actually implemented.
    //
    // [1]: https://esperantotech.atlassian.net/wiki/spaces/VE/pages/221216827/Interface+between+assembly+test+and+EVL+DV+environment
    switch (IIMM) {
    case 0x602: { // L1 eviction for the given minion (sysemu only).
        if (cpu.chip->emu()->get_mem_check()) {
            cpu.chip->emu()->get_mem_checker().l1_evict_all(shire_index(cpu), core_index(cpu) % EMU_MINIONS_PER_SHIRE);
        }
        break;
    }
    default:;
    }
#endif
}


void insn_sltiu(Hart& cpu)
{
    DISASM_RD_RS1_IIMM("sltiu");
    WRITE_RD(RS1 < uint64_t(IIMM));
}

void insn_sltu(Hart& cpu)
{
    DISASM_RD_RS1_RS2("sltu");
    WRITE_RD(RS1 < RS2);
}


void insn_sra(Hart& cpu)
{
    DISASM_RD_RS1_RS2("sra");
    WRITE_RD(int64_t(RS1) >> (RS2 % 64));
}


void insn_srai(Hart& cpu)
{
    DISASM_RD_RS1_SHAMT6("srai");
    WRITE_RD(int64_t(RS1) >> SHAMT6);
}


void insn_sraiw(Hart& cpu)
{
    DISASM_RD_RS1_SHAMT5("sraiw");
    WRITE_RD(sext<32>(int32_t(RS1) >> SHAMT5));
}


void insn_sraw(Hart& cpu)
{
    DISASM_RD_RS1_RS2("sraw");
    WRITE_RD(sext<32>(int32_t(RS1) >> (RS2 % 32)));
}


void insn_srl(Hart& cpu)
{
    DISASM_RD_RS1_RS2("srl");
    WRITE_RD(RS1 >> (RS2 % 64));
}


void insn_srli(Hart& cpu)
{
    DISASM_RD_RS1_SHAMT6("srli");
    WRITE_RD(RS1 >> SHAMT6);
}


void insn_srliw(Hart& cpu)
{
    DISASM_RD_RS1_SHAMT5("srliw");
    WRITE_RD(sext<32>(uint32_t(RS1) >> SHAMT5));
}


void insn_srlw(Hart& cpu)
{
    DISASM_RD_RS1_RS2("srlw");
    WRITE_RD(sext<32>(uint32_t(RS1) >> (RS2 % 32)));
}


void insn_sub(Hart& cpu)
{
    DISASM_RD_RS1_RS2("sub");
    WRITE_RD(RS1 - RS2);
}


void insn_subw(Hart& cpu)
{
    DISASM_RD_RS1_RS2("subw");
    WRITE_RD(sext<32>(RS1 - RS2));
}


void insn_xor(Hart& cpu)
{
    DISASM_RD_RS1_RS2("xor");
    WRITE_RD(RS1 ^ RS2);
}


void insn_xori(Hart& cpu)
{
    DISASM_RD_RS1_IIMM("xori");
    WRITE_RD(RS1 ^ IIMM);
}


} // namespace bemu
