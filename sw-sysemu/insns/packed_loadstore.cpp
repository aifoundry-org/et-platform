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


void insn_fbc_ps(Hart& cpu)
{
    require_fp_active();
    DISASM_LOAD_FD_RS1_IIMM("fbc.ps");
    LOG_MREG(":", 0);
    uint32_t tmp = M0.any() ? mmu_load32(cpu, RS1 + IIMM, Mem_Access_Load) : 0;
    LOAD_VD(tmp);
}


void insn_fg32b_ps(Hart& cpu)
{
    require_fp_active();
    DISASM_GATHER_FD_RS1_RS2("fg32b.ps");
    GATHER32(sext<8>(mmu_load8(cpu, (RS2 & ~31ull) + ((RS2 + (RS1>>(5*e))) & 31), Mem_Access_Load)));
}


void insn_fg32h_ps(Hart& cpu)
{
    require_fp_active();
    DISASM_GATHER_FD_RS1_RS2("fg32h.ps");
    GATHER32(sext<16>(mmu_load16(cpu, (RS2 & ~31ull) + ((RS2 + ((RS1>>(4*e))<<1)) & 30), Mem_Access_Load)));
}


void insn_fg32w_ps(Hart& cpu)
{
    require_fp_active();
    DISASM_GATHER_FD_RS1_RS2("fg32w.ps");
    GATHER32(mmu_load32(cpu, (RS2 & ~31ull) + ((RS2 + ((RS1>>(3*e))<<2)) & 28), Mem_Access_Load));
}


void insn_fgb_ps(Hart& cpu)
{
    require_fp_active();
    DISASM_GATHER_FD_FS1_RS2("fgb.ps");
    GATHER(sext<8>(mmu_load8(cpu, RS2 + FS1.i32[e], Mem_Access_Load)));
}


void insn_fgh_ps(Hart& cpu)
{
    require_fp_active();
    DISASM_GATHER_FD_FS1_RS2("fgh.ps");
    GATHER(sext<16>(mmu_load16(cpu, RS2 + FS1.i32[e], Mem_Access_Load)));
}


void insn_fgw_ps(Hart& cpu)
{
    require_fp_active();
    DISASM_GATHER_FD_FS1_RS2("fgw.ps");
    GATHER(mmu_load32(cpu, RS2 + FS1.i32[e], Mem_Access_Load));
}


void insn_flq2(Hart& cpu)
{
    require_fp_active();
    DISASM_LOAD_FD_RS1_IIMM("flq2");
    mmu_loadVLEN(cpu, RS1 + IIMM, FD, mreg_t(-1), Mem_Access_Load);
    LOAD_VD_NODATA(mreg_t(-1));
}


void insn_flw_ps(Hart& cpu)
{
    require_fp_active();
    DISASM_LOAD_FD_RS1_IIMM("flw.ps");
    LOG_MREG(":", 0);
    mmu_loadVLEN(cpu, RS1 + IIMM, FD, M0, Mem_Access_Load);
    LOAD_VD_NODATA(M0);
}

void insn_fsc32b_ps(Hart& cpu)
{
    require_fp_active();
    DISASM_SCATTER_FD_RS1_RS2("fsc32b.ps");
    SCATTER32(mmu_store8(cpu, (RS2 & ~31ull) + ((RS2 + (RS1>>(5*e))) & 31), uint8_t(FD.u32[e]), Mem_Access_Store));
}


void insn_fsc32h_ps(Hart& cpu)
{
    require_fp_active();
    DISASM_SCATTER_FD_RS1_RS2("fsc32h.ps");
    SCATTER32(mmu_store16(cpu, (RS2 & ~31ull) + ((RS2 + ((RS1>>(4*e))<<1)) & 30), uint16_t(FD.u32[e]), Mem_Access_Store));
}


void insn_fsc32w_ps(Hart& cpu)
{
    require_fp_active();
    DISASM_SCATTER_FD_RS1_RS2("fsc32w.ps");
    SCATTER32(mmu_store32(cpu, (RS2 & ~31ull) + ((RS2 + ((RS1>>(3*e))<<2)) & 28), FD.u32[e], Mem_Access_Store));
}


void insn_fscb_ps(Hart& cpu)
{
    require_fp_active();
    DISASM_SCATTER_FD_FS1_RS2("fscb.ps");
    SCATTER(mmu_store8(cpu, RS2 + FS1.i32[e], uint8_t(FD.u32[e]), Mem_Access_Store));
}


void insn_fsch_ps(Hart& cpu)
{
    require_fp_active();
    DISASM_SCATTER_FD_FS1_RS2("fsch.ps");
    SCATTER(mmu_store16(cpu, RS2 + FS1.i32[e], uint16_t(FD.u32[e]), Mem_Access_Store));
}


void insn_fscw_ps(Hart& cpu)
{
    require_fp_active();
    DISASM_SCATTER_FD_FS1_RS2("fscw.ps");
    SCATTER(mmu_store32(cpu, RS2 + FS1.i32[e], FD.u32[e], Mem_Access_Store));
}


void insn_fsq2(Hart& cpu)
{
    require_fp_active();
    DISASM_STORE_FS2_RS1_SIMM("fsq2");
    mmu_storeVLEN(cpu, RS1 + SIMM, FS2, mreg_t(-1), Mem_Access_Store);
}


void insn_fsw_ps(Hart& cpu)
{
    require_fp_active();
    DISASM_STORE_FS2_RS1_SIMM("fsw.ps");
    LOG_MREG(":", 0);
    mmu_storeVLEN(cpu, RS1 + SIMM, FS2, M0, Mem_Access_Store);
}


} // namespace bemu
