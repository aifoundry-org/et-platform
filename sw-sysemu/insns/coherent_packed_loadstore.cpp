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
#include "system.h"
#include "utility.h"

namespace bemu {


void insn_fgbg_ps(Hart& cpu)
{
    require_fp_active();
    DISASM_GATHER_FD_FS1_RS2("fgbg.ps");
    GATHER(sext<8>(mmu_load8(cpu, RS2 + FS1.i32[e], Mem_Access_LoadG)));
}


void insn_fgbl_ps(Hart& cpu)
{
    require_fp_active();
    DISASM_GATHER_FD_FS1_RS2("fgbl.ps");
    GATHER(sext<8>(mmu_load8(cpu, RS2 + FS1.i32[e], Mem_Access_LoadL)));
}


void insn_fghg_ps(Hart& cpu)
{
    require_fp_active();
    DISASM_GATHER_FD_FS1_RS2("fghg.ps");
    GATHER(sext<16>(mmu_aligned_load16(cpu, RS2 + FS1.i32[e], Mem_Access_LoadG)));
}


void insn_fghl_ps(Hart& cpu)
{
    require_fp_active();
    DISASM_GATHER_FD_FS1_RS2("fghl.ps");
    GATHER(sext<16>(mmu_aligned_load16(cpu, RS2 + FS1.i32[e], Mem_Access_LoadL)));
}


void insn_fgwg_ps(Hart& cpu)
{
    require_fp_active();
    DISASM_GATHER_FD_FS1_RS2("fgwg.ps");
    GATHER(mmu_aligned_load32(cpu, RS2 + FS1.i32[e], Mem_Access_LoadG));
}


void insn_fgwl_ps(Hart& cpu)
{
    require_fp_active();
    DISASM_GATHER_FD_FS1_RS2("fgwl.ps");
    GATHER(mmu_aligned_load32(cpu, RS2 + FS1.i32[e], Mem_Access_LoadL));
}


void insn_flwg_ps(Hart& cpu)
{
    require_fp_active();
    DISASM_LOAD_FD_RS1("flwg.ps");
    LOG_MREG(":", 0);

    mmu_aligned_loadVLEN(cpu, RS1, FD, M0, Mem_Access_LoadG);
    LOAD_VD_NODATA(M0);
}


void insn_flwl_ps(Hart& cpu)
{
    require_fp_active();
    DISASM_LOAD_FD_RS1("flwl.ps");
    LOG_MREG(":", 0);

    mmu_aligned_loadVLEN(cpu, RS1, FD, M0, Mem_Access_LoadL);
    LOAD_VD_NODATA(M0);
}


void insn_fscbg_ps(Hart& cpu)
{
    require_fp_active();
    DISASM_SCATTER_FD_FS1_RS2("fscbg.ps");
    SCATTER(mmu_store8(cpu, RS2 + FS1.i32[e], uint8_t(FD.u32[e]), Mem_Access_StoreG));
}


void insn_fscbl_ps(Hart& cpu)
{
    require_fp_active();
    DISASM_SCATTER_FD_FS1_RS2("fscbl.ps");
    SCATTER(mmu_store8(cpu, RS2 + FS1.i32[e], uint8_t(FD.u32[e]), Mem_Access_StoreL));
}


void insn_fschg_ps(Hart& cpu)
{
    require_fp_active();
    DISASM_SCATTER_FD_FS1_RS2("fschg.ps");
    SCATTER(mmu_aligned_store16(cpu, RS2 + FS1.i32[e], uint16_t(FD.u32[e]), Mem_Access_StoreG));
}


void insn_fschl_ps(Hart& cpu)
{
    require_fp_active();
    DISASM_SCATTER_FD_FS1_RS2("fschl.ps");
    SCATTER(mmu_aligned_store16(cpu, RS2 + FS1.i32[e], uint16_t(FD.u32[e]), Mem_Access_StoreL));
}


void insn_fscwg_ps(Hart& cpu)
{
    require_fp_active();
    DISASM_SCATTER_FD_FS1_RS2("fscwg.ps");
    SCATTER(mmu_aligned_store32(cpu, RS2 + FS1.i32[e], FD.u32[e], Mem_Access_StoreG));
}


void insn_fscwl_ps(Hart& cpu)
{
    require_fp_active();
    DISASM_SCATTER_FD_FS1_RS2("fscwl.ps");
    SCATTER(mmu_aligned_store32(cpu, RS2 + FS1.i32[e], FD.u32[e], Mem_Access_StoreL));
}


void insn_fswg_ps(Hart& cpu)
{
    require_fp_active();
    DISASM_STORE_FD_RS1("fswg.ps");
    LOG_MREG(":", 0);
    mreg_t msk;
    if (cpu.chip->stepping == System::Stepping::A0) {
        msk = mreg_t(-1);
        if (msk != M0) {
            WARN_HART(memory, cpu, "%s", "fswg.ps with m0 not all 1s is UNDEFINED behavior");
        }
    } else {
        msk = M0;
    }
    mmu_aligned_storeVLEN(cpu, RS1, FD, msk, Mem_Access_StoreG);
}


void insn_fswl_ps(Hart& cpu)
{
    require_fp_active();
    DISASM_STORE_FD_RS1("fswl.ps");
    LOG_MREG(":", 0);
    mreg_t msk;
    if (cpu.chip->stepping == System::Stepping::A0) {
        msk = mreg_t(-1);
        if (msk != M0) {
            WARN_HART(memory, cpu, "%s", "fswl.ps with m0 not all 1s is UNDEFINED behavior");
        }
    } else {
        msk = M0;
    }
    mmu_aligned_storeVLEN(cpu, RS1, FD, msk, Mem_Access_StoreL);
}


} // namespace bemu
