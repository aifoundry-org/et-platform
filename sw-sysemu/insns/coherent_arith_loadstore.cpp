/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
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


void insn_sbg(Hart& cpu)
{
    DISASM_STORE_RS2_RS1("sbg");
    mmu_store8(cpu, RS1, uint8_t(RS2), Mem_Access_StoreG);
}


void insn_sbl(Hart& cpu)
{
    DISASM_STORE_RS2_RS1("sbl");
    mmu_store8(cpu, RS1, uint8_t(RS2), Mem_Access_StoreL);
}


void insn_shg(Hart& cpu)
{
    DISASM_STORE_RS2_RS1("shg");
    mmu_aligned_store16(cpu, RS1, uint16_t(RS2), Mem_Access_StoreG);
}


void insn_shl(Hart& cpu)
{
    DISASM_STORE_RS2_RS1("shl");
    mmu_aligned_store16(cpu, RS1, uint16_t(RS2), Mem_Access_StoreL);
}


} // namespace bemu
