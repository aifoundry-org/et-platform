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
#include "esrs.h"
#include "fpu/fpu.h"
#include "fpu/fpu_casts.h"
#include "gold.h"
#include "insn.h"
#include "insn_func.h"
#include "log.h"
#include "processor.h"
#include "traps.h"
#include "utility.h"

namespace bemu {


extern std::array<Processor,EMU_NUM_THREADS> cpu;


static inline int32_t frcp_fix_rast_vs_gold(int32_t x, int32_t y)
{
    int32_t fpuval = fpu::fxp1714_rcpStep(x, y);
    int32_t gldval = gld::fxp1714_rcpStep(x, y);
    if (abs(fpuval - gldval) > 1)
    {
        LOG(WARN, "FRCP_FIX_RAST mismatch with input: 0x%08x,0x%08x golden: 0x%08x libfpu: 0x%08x. This might happen, report to jordi.sola@esperantotech.com if needed.",
            x, y, gldval, fpuval);
    }
    return fpuval;
}


void insn_cubeface_ps(insn_t inst)
{
    require_feature_gfx();
    require_fp_active();
    DISASM_FDS0_FS1_FS2("cubeface.ps");
    WRITE_VD( (FD.u32[e] & 1)
              ? ((FS2.u32[e] & 1) ? 0 : 1)
              : ((FS1.u32[e] & 1) ? 0 : 2) );
}


void insn_cubefaceidx_ps(insn_t inst)
{
    require_feature_gfx();
    require_fp_active();
    DISASM_FD_FS1_FS2("cubefaceidx.ps");
    WRITE_VD( fpu::f32_cubeFaceIdx(uint8_t(FS1.u32[e]), FS2.f32[e]) );
    set_fp_exceptions();
}


void insn_cubesgnsc_ps(insn_t inst)
{
    require_feature_gfx();
    require_fp_active();
    DISASM_FD_FS1_FS2("cubesgnsc.ps");
    WRITE_VD( fpu::f32_cubeFaceSignS(uint8_t(FS1.u32[e]), FS2.f32[e]) );
}


void insn_cubesgntc_ps(insn_t inst)
{
    require_feature_gfx();
    require_fp_active();
    DISASM_FD_FS1_FS2("cubesgntc.ps");
    WRITE_VD( fpu::f32_cubeFaceSignT(uint8_t(FS1.u32[e]), FS2.f32[e]) );
}


void insn_fcvt_f10_ps(insn_t inst)
{
    require_feature_gfx();
    require_fp_active();
    DISASM_FD_FS1_FRM("fcvt.f10.ps");
    set_rounding_mode(FRM);
    WRITE_VD( fpu::f32_to_f10(FS1.f32[e]) );
    set_fp_exceptions();
}


void insn_fcvt_f11_ps(insn_t inst)
{
    require_feature_gfx();
    require_fp_active();
    DISASM_FD_FS1_FRM("fcvt.f11.ps");
    set_rounding_mode(FRM);
    WRITE_VD( fpu::f32_to_f11(FS1.f32[e]) );
    set_fp_exceptions();
}


void insn_fcvt_ps_f10(insn_t inst)
{
    require_feature_gfx();
    require_fp_active();
    DISASM_FD_FS1("fcvt.ps.f10");
    WRITE_VD( fpu::f10_to_f32(FS1.f10[2*e]) );
    set_fp_exceptions();
}


void insn_fcvt_ps_f11(insn_t inst)
{
    require_feature_gfx();
    require_fp_active();
    DISASM_FD_FS1("fcvt.ps.f11");
    WRITE_VD( fpu::f11_to_f32(FS1.f11[2*e]) );
    set_fp_exceptions();
}


void insn_fcvt_ps_rast(insn_t inst)
{
    require_feature_gfx();
    require_fp_active();
    DISASM_FD_FS1_RM("fcvt.ps.rast");
    set_rounding_mode(FRM);
    WRITE_VD( fpu::fxp1516_to_f32(FS1.i32[e]) );
    set_fp_exceptions();
}


void insn_fcvt_ps_sn16(insn_t inst)
{
    require_feature_gfx();
    require_fp_active();
    DISASM_FD_FS1("fcvt.ps.sn16");
    WRITE_VD( fpu::sn16_to_f32(FS1.i16[2*e]) );
    set_fp_exceptions();
}


void insn_fcvt_ps_sn8(insn_t inst)
{
    require_feature_gfx();
    require_fp_active();
    DISASM_FD_FS1("fcvt.ps.sn8");
    WRITE_VD( fpu::sn8_to_f32(FS1.i8[4*e]) );
    set_fp_exceptions();
}


void insn_fcvt_ps_un10(insn_t inst)
{
    require_feature_gfx();
    require_fp_active();
    DISASM_FD_FS1_FRM("fcvt.ps.un10");
    set_rounding_mode(FRM);
    WRITE_VD( fpu::un10_to_f32(FS1.u16[2*e]) );
    set_fp_exceptions();
}


void insn_fcvt_ps_un16(insn_t inst)
{
    require_feature_gfx();
    require_fp_active();
    DISASM_FD_FS1_FRM("fcvt.ps.un16");
    set_rounding_mode(FRM);
    WRITE_VD( fpu::un16_to_f32(FS1.u16[2*e]) );
    set_fp_exceptions();
}


void insn_fcvt_ps_un2(insn_t inst)
{
    require_feature_gfx();
    require_fp_active();
    DISASM_FD_FS1_FRM("fcvt.ps.un2");
    set_rounding_mode(FRM);
    WRITE_VD( fpu::un2_to_f32(FS1.u8[4*e]) );
    set_fp_exceptions();
}


void insn_fcvt_ps_un24(insn_t inst)
{
    require_feature_gfx();
    require_fp_active();
    DISASM_FD_FS1_FRM("fcvt.ps.un24");
    set_rounding_mode(FRM);
    WRITE_VD( fpu::un24_to_f32(FS1.u32[e]) );
    set_fp_exceptions();
}


void insn_fcvt_ps_un8(insn_t inst)
{
    require_feature_gfx();
    require_fp_active();
    DISASM_FD_FS1_FRM("fcvt.ps.un8");
    set_rounding_mode(FRM);
    WRITE_VD( fpu::un8_to_f32(FS1.u8[4*e]) );
    set_fp_exceptions();
}


void insn_fcvt_rast_ps(insn_t inst)
{
    require_feature_gfx();
    require_fp_active();
    DISASM_FD_FS1_FRM("fcvt.rast.ps");
    set_rounding_mode(FRM);
    WRITE_VD( fpu::f32_to_fxp1714(FS1.f32[e]) );
    set_fp_exceptions();
}


void insn_fcvt_sn16_ps(insn_t inst)
{
    require_feature_gfx();
    require_fp_active();
    DISASM_FD_FS1_FRM("fcvt.sn16.ps");
    WRITE_VD( fpu::f32_to_sn16(FS1.f32[e]) );
    set_fp_exceptions();
}


void insn_fcvt_sn8_ps(insn_t inst)
{
    require_feature_gfx();
    require_fp_active();
    DISASM_FD_FS1_FRM("fcvt.sn8.ps");
    WRITE_VD( fpu::f32_to_sn8(FS1.f32[e]) );
    set_fp_exceptions();
}


void insn_fcvt_un10_ps(insn_t inst)
{
    require_feature_gfx();
    require_fp_active();
    DISASM_FD_FS1_FRM("fcvt.un10.ps");
    WRITE_VD( fpu::f32_to_un10(FS1.f32[e]) );
    set_fp_exceptions();
}


void insn_fcvt_un16_ps(insn_t inst)
{
    require_feature_gfx();
    require_fp_active();
    DISASM_FD_FS1_FRM("fcvt.un16.ps");
    WRITE_VD( fpu::f32_to_un16(FS1.f32[e]) );
    set_fp_exceptions();
}


void insn_fcvt_un24_ps(insn_t inst)
{
    require_feature_gfx();
    require_fp_active();
    DISASM_FD_FS1_FRM("fcvt.un24.ps");
    WRITE_VD( fpu::f32_to_un24(FS1.f32[e]) );
    set_fp_exceptions();
}


void insn_fcvt_un2_ps(insn_t inst)
{
    require_feature_gfx();
    require_fp_active();
    DISASM_FD_FS1_FRM("fcvt.un2.ps");
    WRITE_VD( fpu::f32_to_un2(FS1.f32[e]) );
    set_fp_exceptions();
}


void insn_fcvt_un8_ps(insn_t inst)
{
    require_feature_gfx();
    require_fp_active();
    DISASM_FD_FS1_FRM("fcvt.un8.ps");
    WRITE_VD( fpu::f32_to_un8(FS1.f32[e]) );
    set_fp_exceptions();
}


void insn_frcp_fix_rast(insn_t inst)
{
    require_feature_gfx();
    require_fp_active();
    DISASM_FD_FS1_FS2_FRM("frcp.fix.rast");
    set_rounding_mode(FRM);
    WRITE_VD( frcp_fix_rast_vs_gold(FS1.i32[e], FS2.i32[e]) );
    set_fp_exceptions();
}


} // namespace bemu
