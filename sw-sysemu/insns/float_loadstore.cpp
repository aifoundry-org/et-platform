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
#include "mmu.h"
#include "processor.h"
#include "utility.h"
#include "fpu/fpu_casts.h"

// FIXME: Replace with "processor.h"
#include "emu_defines.h"
extern std::array<Processor,EMU_NUM_THREADS> cpu;

//namespace bemu {


void insn_flw(insn_t inst)
{
    require_fp_active();
    DISASM_LOAD_FD_RS1_IIMM("flw");
    LOAD_FD(mmu_load<uint32_t>(RS1 + IIMM, Mem_Access_Load));

}


void insn_fsw(insn_t inst)
{
    require_fp_active();
    DISASM_STORE_FS2_RS1_SIMM("fsw");
    mmu_store<uint32_t>(RS1 + SIMM, FS2.u32[0], Mem_Access_Store);
}


//} // namespace bemu
