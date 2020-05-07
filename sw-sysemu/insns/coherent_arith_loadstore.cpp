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


void insn_sbg(insn_t inst)
{
    DISASM_STORE_RS2_RS1("sbg");
    mmu_store<uint8_t>(RS1, uint8_t(RS2), Mem_Access_StoreG);
}


void insn_sbl(insn_t inst)
{
    DISASM_STORE_RS2_RS1("sbl");
    mmu_store<uint8_t>(RS1, uint8_t(RS2), Mem_Access_StoreL);
}


void insn_shg(insn_t inst)
{
    DISASM_STORE_RS2_RS1("shg");
    mmu_aligned_store16(RS1, uint16_t(RS2), Mem_Access_StoreG);
}


void insn_shl(insn_t inst)
{
    DISASM_STORE_RS2_RS1("shl");
    mmu_aligned_store16(RS1, uint16_t(RS2), Mem_Access_StoreL);
}


} // namespace bemu
