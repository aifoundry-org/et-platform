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
#include "mmu.h"
#include "processor.h"
#include "utility.h"

namespace bemu {


void insn_fence(Hart& cpu)
{
    DISASM_NOARG("fence");
}


void insn_lb(Hart& cpu)
{
    DISASM_LOAD_RD_RS1_IIMM("lb");
    uint64_t tmp = sext<8>(mmu_load8(cpu, RS1 + IIMM, Mem_Access_Load));
    LOAD_WRITE_RD(tmp);
}


void insn_lbu(Hart& cpu)
{
    DISASM_LOAD_RD_RS1_IIMM("lbu");
    uint64_t tmp = mmu_load8(cpu, RS1 + IIMM, Mem_Access_Load);
    LOAD_WRITE_RD(tmp);
}


void insn_ld(Hart& cpu)
{
    DISASM_LOAD_RD_RS1_IIMM("ld");
    uint64_t tmp = mmu_load64(cpu, RS1 + IIMM, Mem_Access_Load);
    LOAD_WRITE_RD(tmp);
}


void insn_lh(Hart& cpu)
{
    DISASM_LOAD_RD_RS1_IIMM("lh");
    uint64_t tmp = sext<16>(mmu_load16(cpu, RS1 + IIMM, Mem_Access_Load));
    LOAD_WRITE_RD(tmp);
}


void insn_lhu(Hart& cpu)
{
    DISASM_LOAD_RD_RS1_IIMM("lhu");
    uint64_t tmp = mmu_load16(cpu, RS1 + IIMM, Mem_Access_Load);
    LOAD_WRITE_RD(tmp);
}


void insn_lw(Hart& cpu)
{
    DISASM_LOAD_RD_RS1_IIMM("lw");
    uint64_t tmp = sext<32>(mmu_load32(cpu, RS1 + IIMM, Mem_Access_Load));
    LOAD_WRITE_RD(tmp);
}


void insn_lwu(Hart& cpu)
{
    DISASM_LOAD_RD_RS1_IIMM("lw");
    uint64_t tmp = mmu_load32(cpu, RS1 + IIMM, Mem_Access_Load);
    LOAD_WRITE_RD(tmp);
}


void insn_sb(Hart& cpu)
{
    DISASM_STORE_RS2_RS1_SIMM("sb");
    mmu_store8(cpu, RS1 + SIMM, uint8_t(RS2), Mem_Access_Store);
}


void insn_sd(Hart& cpu)
{
    DISASM_STORE_RS2_RS1_SIMM("sd");
    mmu_store64(cpu, RS1 + SIMM, RS2, Mem_Access_Store);
}


void insn_sh(Hart& cpu)
{
    DISASM_STORE_RS2_RS1_SIMM("sh");
    mmu_store16(cpu, RS1 + SIMM, uint16_t(RS2), Mem_Access_Store);
}


void insn_sw(Hart& cpu)
{
    DISASM_STORE_RS2_RS1_SIMM("sw");
    mmu_store32(cpu, RS1 + SIMM, uint32_t(RS2), Mem_Access_Store);
}


} // namespace bemu
