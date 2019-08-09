/* vim: set ts=8 sw=4 et sta cin cino=\:0s,l1,g0,N-s,E-s,i0,+2s,(0,W2s : */

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
extern system_version_t sysver;

//namespace bemu {


void insn_fgbg_ps(insn_t inst)
{
    require_fp_active();
    DISASM_GATHER_FD_FS1_RS2("fgbg.ps");
    GATHER(sext<8>(mmu_load8(RS2 + FS1.i32[e])));
}


void insn_fgbl_ps(insn_t inst)
{
    require_fp_active();
    DISASM_GATHER_FD_FS1_RS2("fgbl.ps");
    GATHER(sext<8>(mmu_load8(RS2 + FS1.i32[e])));
}


void insn_fghg_ps(insn_t inst)
{
    require_fp_active();
    DISASM_GATHER_FD_FS1_RS2("fghg.ps");
    GATHER(sext<16>(mmu_aligned_load16(RS2 + FS1.i32[e])));
}


void insn_fghl_ps(insn_t inst)
{
    require_fp_active();
    DISASM_GATHER_FD_FS1_RS2("fghl.ps");
    GATHER(sext<16>(mmu_aligned_load16(RS2 + FS1.i32[e])));
}


void insn_fgwg_ps(insn_t inst)
{
    require_fp_active();
    DISASM_GATHER_FD_FS1_RS2("fgwg.ps");
    GATHER(mmu_aligned_load32(RS2 + FS1.i32[e]));
}


void insn_fgwl_ps(insn_t inst)
{
    require_fp_active();
    DISASM_GATHER_FD_FS1_RS2("fgwl.ps");
    GATHER(mmu_aligned_load32(RS2 + FS1.i32[e]));
}


void insn_flwg_ps(insn_t inst)
{
    require_fp_active();
    DISASM_LOAD_FD_RS1("flwg.ps");
    LOG_MREG(":", 0);
    mmu_aligned_loadVLEN(RS1, FD, M0);
    LOAD_VD_NODATA(M0);
}


void insn_flwl_ps(insn_t inst)
{
    require_fp_active();
    DISASM_LOAD_FD_RS1("flwl.ps");
    LOG_MREG(":", 0);
    mmu_aligned_loadVLEN(RS1, FD, M0);
    LOAD_VD_NODATA(M0);
}


void insn_fscbg_ps(insn_t inst)
{
    require_fp_active();
    DISASM_SCATTER_FD_FS1_RS2("fscbg.ps");
    SCATTER(mmu_store8(RS2 + FS1.i32[e], uint8_t(FD.u32[e])));
}


void insn_fscbl_ps(insn_t inst)
{
    require_fp_active();
    DISASM_SCATTER_FD_FS1_RS2("fscbl.ps");
    SCATTER(mmu_store8(RS2 + FS1.i32[e], uint8_t(FD.u32[e])));
}


void insn_fschg_ps(insn_t inst)
{
    require_fp_active();
    DISASM_SCATTER_FD_FS1_RS2("fschg.ps");
    SCATTER(mmu_aligned_store16(RS2 + FS1.i32[e], uint16_t(FD.u32[e])));
}


void insn_fschl_ps(insn_t inst)
{
    require_fp_active();
    DISASM_SCATTER_FD_FS1_RS2("fschl.ps");
    SCATTER(mmu_aligned_store16(RS2 + FS1.i32[e], uint16_t(FD.u32[e])));
}


void insn_fscwg_ps(insn_t inst)
{
    require_fp_active();
    DISASM_SCATTER_FD_FS1_RS2("fscwg.ps");
    SCATTER(mmu_aligned_store32(RS2 + FS1.i32[e], FD.u32[e]));
}


void insn_fscwl_ps(insn_t inst)
{
    require_fp_active();
    DISASM_SCATTER_FD_FS1_RS2("fscwl.ps");
    SCATTER(mmu_aligned_store32(RS2 + FS1.i32[e], FD.u32[e]));
}


void insn_fswg_ps(insn_t inst)
{
    require_fp_active();
    DISASM_STORE_FD_RS1("fswg.ps");
    LOG_MREG(":", 0);
    mreg_t msk(sysver == system_version_t::ETSOC1_A0 ? mreg_t(-1) : M0);
    mmu_aligned_storeVLEN(RS1, FD, msk);
}


void insn_fswl_ps(insn_t inst)
{
    require_fp_active();
    DISASM_STORE_FD_RS1("fswl.ps");
    LOG_MREG(":", 0);
    mreg_t msk(sysver == system_version_t::ETSOC1_A0 ? mreg_t(-1) : M0);
    mmu_aligned_storeVLEN(RS1, FD, msk);
}


//} // namespace bemu
