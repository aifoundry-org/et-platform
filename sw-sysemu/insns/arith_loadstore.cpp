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
#include "insn.h"
#include "insn_func.h"
#include "log.h"
#include "mmu.h"
#include "processor.h"
#include "utility.h"

namespace bemu {


extern std::array<Hart,EMU_NUM_THREADS> cpu;


void insn_fence(insn_t inst __attribute__((unused)))
{
    DISASM_NOARG("fence");
}


void insn_lb(insn_t inst)
{
    DISASM_LOAD_RD_RS1_IIMM("lb");
    uint64_t tmp = sext<8>(mmu_load<uint8_t>(RS1 + IIMM, Mem_Access_Load));
    LOAD_WRITE_RD(tmp);
}


void insn_lbu(insn_t inst)
{
    DISASM_LOAD_RD_RS1_IIMM("lbu");
    uint64_t tmp = mmu_load<uint8_t>(RS1 + IIMM, Mem_Access_Load);
    LOAD_WRITE_RD(tmp);
}


void insn_ld(insn_t inst)
{
    DISASM_LOAD_RD_RS1_IIMM("ld");
    uint64_t tmp = mmu_load<uint64_t>(RS1 + IIMM, Mem_Access_Load);
    LOAD_WRITE_RD(tmp);
}


void insn_lh(insn_t inst)
{
    DISASM_LOAD_RD_RS1_IIMM("lh");
    uint64_t tmp = sext<16>(mmu_load<uint16_t>(RS1 + IIMM, Mem_Access_Load));
    LOAD_WRITE_RD(tmp);
}


void insn_lhu(insn_t inst)
{
    DISASM_LOAD_RD_RS1_IIMM("lhu");
    uint64_t tmp = mmu_load<uint16_t>(RS1 + IIMM, Mem_Access_Load);
    LOAD_WRITE_RD(tmp);
}


void insn_lw(insn_t inst)
{
    DISASM_LOAD_RD_RS1_IIMM("lw");
    uint64_t tmp = sext<32>(mmu_load<uint32_t>(RS1 + IIMM, Mem_Access_Load));
    LOAD_WRITE_RD(tmp);
}


void insn_lwu(insn_t inst)
{
    DISASM_LOAD_RD_RS1_IIMM("lw");
    uint64_t tmp = mmu_load<uint32_t>(RS1 + IIMM, Mem_Access_Load);
    LOAD_WRITE_RD(tmp);
}


void insn_sb(insn_t inst)
{
    DISASM_STORE_RS2_RS1_SIMM("sb");
    mmu_store<uint8_t>(RS1 + SIMM, uint8_t(RS2), Mem_Access_Store);
}


void insn_sd(insn_t inst)
{
    DISASM_STORE_RS2_RS1_SIMM("sd");
    mmu_store<uint64_t>(RS1 + SIMM, RS2, Mem_Access_Store);
}


void insn_sh(insn_t inst)
{
    DISASM_STORE_RS2_RS1_SIMM("sh");
    mmu_store<uint16_t>(RS1 + SIMM, uint16_t(RS2), Mem_Access_Store);
}


void insn_sw(insn_t inst)
{
    DISASM_STORE_RS2_RS1_SIMM("sw");
    mmu_store<uint32_t>(RS1 + SIMM, uint32_t(RS2), Mem_Access_Store);
}


} // namespace bemu
