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

//namespace bemu {


void insn_fbc_ps(insn_t inst)
{
    require_fp_active();
    DISASM_LOAD_FD_RS1_IIMM("fbc.ps");
    LOG_MREG(":", 0);
    uint32_t tmp = M0.any() ? mmu_load32(RS1 + IIMM) : 0;
    WRITE_VD(tmp);
}


void insn_fg32b_ps(insn_t inst)
{
    require_fp_active();
    DISASM_GATHER_FD_RS1_RS2("fg32b.ps");
    GATHER(sext<8>(mmu_load8((RS2 & ~31ull) + ((RS2 + (RS1>>(5*e))) & 31))));
}


void insn_fg32h_ps(insn_t inst)
{
    require_fp_active();
    DISASM_GATHER_FD_RS1_RS2("fg32h.ps");
    GATHER(sext<16>(mmu_load16((RS2 & ~31ull) + ((RS2 + ((RS1>>(4*e))<<1)) & 30))));
}


void insn_fg32w_ps(insn_t inst)
{
    require_fp_active();
    DISASM_GATHER_FD_RS1_RS2("fg32w.ps");
    GATHER(mmu_load32((RS2 & ~31ull) + ((RS2 + ((RS1>>(3*e))<<2)) & 28)));
}


void insn_fgb_ps(insn_t inst)
{
    require_fp_active();
    DISASM_GATHER_FD_FS1_RS2("fgb.ps");
    GATHER(sext<8>(mmu_load8(RS2 + FS1.i32[e])));
}


void insn_fgh_ps(insn_t inst)
{
    require_fp_active();
    DISASM_GATHER_FD_FS1_RS2("fgh.ps");
    GATHER(sext<16>(mmu_load16(RS2 + FS1.i32[e])));
}


void insn_fgw_ps(insn_t inst)
{
    require_fp_active();
    DISASM_GATHER_FD_FS1_RS2("fgw.ps");
    GATHER(mmu_load32(RS2 + FS1.i32[e]));
}


void insn_flq2(insn_t inst)
{
    require_fp_active();
    DISASM_LOAD_FD_RS1_IIMM("flq2");
    mmu_loadVLEN(RS1 + IIMM, FD, mreg_t(-1));
    WRITE_VD_NODATA(mreg_t(-1));
}


void insn_flw_ps(insn_t inst)
{
    require_fp_active();
    DISASM_LOAD_FD_RS1_IIMM("flw.ps");
    LOG_MREG(":", 0);
    mmu_loadVLEN(RS1 + IIMM, FD, M0);
    WRITE_VD_NODATA(M0);
}

void insn_fsc32b_ps(insn_t inst)
{
    require_fp_active();
    DISASM_SCATTER_FD_RS1_RS2("fsc32b.ps");
    SCATTER(mmu_store8((RS2 & ~31ull) + ((RS2 + (RS1>>(5*e))) & 31), uint8_t(FD.u32[e])));
}


void insn_fsc32h_ps(insn_t inst)
{
    require_fp_active();
    DISASM_SCATTER_FD_RS1_RS2("fsc32h.ps");
    SCATTER(mmu_store16((RS2 & ~31ull) + ((RS2 + ((RS1>>(4*e))<<1)) & 30), uint16_t(FD.u32[e])));
}


void insn_fsc32w_ps(insn_t inst)
{
    require_fp_active();
    DISASM_SCATTER_FD_RS1_RS2("fsc32w.ps");
    SCATTER(mmu_store32((RS2 & ~31ull) + ((RS2 + ((RS1>>(3*e))<<2)) & 28), FD.u32[e]));
}


void insn_fscb_ps(insn_t inst)
{
    require_fp_active();
    DISASM_SCATTER_FD_FS1_RS2("fscb.ps");
    SCATTER(mmu_store8(RS2 + FS1.i32[e], uint8_t(FD.u32[e])));
}


void insn_fsch_ps(insn_t inst)
{
    require_fp_active();
    DISASM_SCATTER_FD_FS1_RS2("fsch.ps");
    SCATTER(mmu_store16(RS2 + FS1.i32[e], uint16_t(FD.u32[e])));
}


void insn_fscw_ps(insn_t inst)
{
    require_fp_active();
    DISASM_SCATTER_FD_FS1_RS2("fscw.ps");
    SCATTER(mmu_store32(RS2 + FS1.i32[e], FD.u32[e]));
}


void insn_fsq2(insn_t inst)
{
    require_fp_active();
    DISASM_STORE_FS2_RS1_SIMM("fsq2");
    mmu_storeVLEN(RS1 + SIMM, FS2, mreg_t(-1));
}


void insn_fsw_ps(insn_t inst)
{
    require_fp_active();
    DISASM_STORE_FS2_RS1_SIMM("fsw.ps");
    LOG_MREG(":", 0);
    mmu_storeVLEN(RS1 + SIMM, FS2, M0);
}


//} // namespace bemu
