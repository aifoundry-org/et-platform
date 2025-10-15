/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------*/

#include "emu_defines.h"
#include "emu_gio.h"
#include "fpu/fpu_casts.h"
#include "insn.h"
#include "insn_func.h"
#include "insn_util.h"
#include "log.h"
#include "mmu.h"
#include "processor.h"
#include "utility.h"

namespace bemu {


void insn_flw(Hart& cpu)
{
    require_fp_active();
    DISASM_LOAD_FD_RS1_IIMM("flw");
    LOAD_FD(mmu_load32(cpu, RS1 + IIMM, Mem_Access_Load));

}


void insn_fsw(Hart& cpu)
{
    require_fp_active();
    DISASM_STORE_FS2_RS1_SIMM("fsw");
    mmu_store32(cpu, RS1 + SIMM, FS2.u32[0], Mem_Access_Store);
}


} // namespace bemu
