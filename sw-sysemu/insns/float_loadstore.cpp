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
