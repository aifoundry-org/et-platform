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


void insn_c_ld(insn_t inst)
{
    C_DISASM_LOAD_RS2P_RS1P_IMMLSD("c.ld");
    uint64_t tmp = mmu_load<uint64_t>(C_RS1P + C_IMMLSD, Mem_Access_Load);
    LOAD_WRITE_C_RS2P(tmp);
}


void insn_c_ldsp(insn_t inst)
{
    C_DISASM_LOAD_LDSP("c.ldsp");
    uint64_t tmp = mmu_load<uint64_t>(X2 + C_IMMLDSP, Mem_Access_Load);
    LOAD_WRITE_C_RS1(tmp);
}


void insn_c_lw(insn_t inst)
{
    C_DISASM_LOAD_RS2P_RS1P_IMMLSW("c.lw");
    uint64_t tmp = sext<32>(mmu_load<uint32_t>(C_RS1P + C_IMMLSW, Mem_Access_Load));
    LOAD_WRITE_C_RS2P(tmp);
}


void insn_c_lwsp(insn_t inst)
{
    C_DISASM_LOAD_LWSP("c.lwsp");
    uint64_t tmp = sext<32>(mmu_load<uint32_t>(X2 + C_IMMLWSP, Mem_Access_Load));
    LOAD_WRITE_C_RS1(tmp);
}


void insn_c_sd(insn_t inst)
{
    C_DISASM_STORE_RS2P_RS1P_IMMLSD("c.sd");
    mmu_store<uint64_t>(C_RS1P + C_IMMLSD, C_RS2P, Mem_Access_Store);
}


void insn_c_sdsp(insn_t inst)
{
    C_DISASM_STORE_SWSP("c.sdsp");
    mmu_store<uint64_t>(X2 + C_IMMSDSP, C_RS2, Mem_Access_Store);
}


void insn_c_sw(insn_t inst)
{
    C_DISASM_STORE_RS2P_RS1P_IMMLSW("c.sd");
    mmu_store<uint32_t>(C_RS1P + C_IMMLSW, uint32_t(C_RS2P), Mem_Access_Store);
}


void insn_c_swsp(insn_t inst)
{
    C_DISASM_STORE_SWSP("c.swsp");
    mmu_store<uint32_t>(X2 + C_IMMSWSP, uint32_t(C_RS2), Mem_Access_Store);
}


} // namespace bemu
