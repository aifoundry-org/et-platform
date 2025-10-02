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


void insn_c_ld(Hart& cpu)
{
    C_DISASM_LOAD_RS2P_RS1P_IMMLSD("c.ld");
    uint64_t tmp = mmu_load64(cpu, C_RS1P + C_IMMLSD, Mem_Access_Load);
    LOAD_WRITE_C_RS2P(tmp);
}


void insn_c_ldsp(Hart& cpu)
{
    C_DISASM_LOAD_LDSP("c.ldsp");
    uint64_t tmp = mmu_load64(cpu, X2 + C_IMMLDSP, Mem_Access_Load);
    LOAD_WRITE_C_RS1(tmp);
}


void insn_c_lw(Hart& cpu)
{
    C_DISASM_LOAD_RS2P_RS1P_IMMLSW("c.lw");
    uint64_t tmp = sext<32>(mmu_load32(cpu, C_RS1P + C_IMMLSW, Mem_Access_Load));
    LOAD_WRITE_C_RS2P(tmp);
}


void insn_c_lwsp(Hart& cpu)
{
    C_DISASM_LOAD_LWSP("c.lwsp");
    uint64_t tmp = sext<32>(mmu_load32(cpu, X2 + C_IMMLWSP, Mem_Access_Load));
    LOAD_WRITE_C_RS1(tmp);
}


void insn_c_sd(Hart& cpu)
{
    C_DISASM_STORE_RS2P_RS1P_IMMLSD("c.sd");
    mmu_store64(cpu, C_RS1P + C_IMMLSD, C_RS2P, Mem_Access_Store);
}


void insn_c_sdsp(Hart& cpu)
{
    C_DISASM_STORE_SDSP("c.sdsp");
    mmu_store64(cpu, X2 + C_IMMSDSP, C_RS2, Mem_Access_Store);
}


void insn_c_sw(Hart& cpu)
{
    C_DISASM_STORE_RS2P_RS1P_IMMLSW("c.sd");
    mmu_store32(cpu, C_RS1P + C_IMMLSW, uint32_t(C_RS2P), Mem_Access_Store);
}


void insn_c_swsp(Hart& cpu)
{
    C_DISASM_STORE_SWSP("c.swsp");
    mmu_store32(cpu, X2 + C_IMMSWSP, uint32_t(C_RS2), Mem_Access_Store);
}


} // namespace bemu
