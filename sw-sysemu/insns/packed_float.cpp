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
#include "fpu/fpu.h"
#include "fpu/fpu_casts.h"
#include "insn.h"
#include "insn_func.h"
#include "insn_util.h"
#include "log.h"
#include "processor.h"
#include "traps.h"
#include "utility.h"

namespace bemu {


void insn_fadd_ps(Hart& cpu)
{
    require_fp_active();
    DISASM_FD_FS1_FS2_RM("fadd.ps");
    set_rounding_mode(cpu, RM);
    WRITE_VD( fpu::f32_add(FS1.f32[e], FS2.f32[e]) );
    set_fp_exceptions(cpu);
}


void insn_fbci_ps(Hart& cpu)
{
    require_fp_active();
    DISASM_FD_F32IMM("fbci.ps");
    WRITE_VD( F32IMM );
}


void insn_fbcx_ps(Hart& cpu)
{
    require_fp_active();
    DISASM_FD_RS1("fbcx.ps");
    INTMV_VD( uint32_t(RS1) );
}


void insn_fclass_ps(Hart& cpu)
{
    require_fp_active();
    DISASM_FD_FS1("fclass.ps");
    WRITE_VD( fpu::f32_classify(FS1.f32[e]) );
}


void insn_fcmov_ps(Hart& cpu)
{
    require_fp_active();
    DISASM_FD_FS1_FS2_FS3("fcmov.ps");
    INTMV_VD( FS1.u32[e] ? FS2.u32[e] : FS3.u32[e] );
}


void insn_fcmovm_ps(Hart& cpu)
{
    require_fp_active();
    DISASM_FD_FS1_FS2("fcmovm.ps");
    LOG_MREG(":", 0);
    INTMV_VD_NOMASK( M0[e] ? FS1.u32[e] : FS2.u32[e] );
}


void insn_fcvt_f16_ps(Hart& cpu)
{
    require_fp_active();
    DISASM_FD_FS1_FRM("fcvt.f16.ps");
    set_rounding_mode(cpu, FRM);
    WRITE_VD( fpu::f32_to_f16(FS1.f32[e]) );
    set_fp_exceptions(cpu);
}


void insn_fcvt_ps_f16(Hart& cpu)
{
    require_fp_active();
    DISASM_FD_FS1("fcvt.ps.f16");
    WRITE_VD( fpu::f16_to_f32(FS1.f16[2*e]) );
    set_fp_exceptions(cpu);
}


void insn_fcvt_ps_pw(Hart& cpu)
{
    require_fp_active();
    DISASM_FD_FS1_RM("fcvt.ps.pw");
    set_rounding_mode(cpu, RM);
    WRITE_VD( fpu::i32_to_f32(FS1.i32[e]) );
    set_fp_exceptions(cpu);
}


void insn_fcvt_ps_pwu(Hart& cpu)
{
    require_fp_active();
    DISASM_FD_FS1_RM("fcvt.ps.pwu");
    set_rounding_mode(cpu, RM);
    WRITE_VD( fpu::ui32_to_f32(FS1.u32[e]) );
    set_fp_exceptions(cpu);
}


void insn_fcvt_pw_ps(Hart& cpu)
{
    require_fp_active();
    DISASM_FD_FS1_RM("fcvt.pw.ps");
    set_rounding_mode(cpu, RM);
    WRITE_VD( fpu::f32_to_i32(FS1.f32[e]) );
    set_fp_exceptions(cpu);
}


void insn_fcvt_pwu_ps(Hart& cpu)
{
    require_fp_active();
    DISASM_FD_FS1_RM("fcvt.pwu.ps");
    set_rounding_mode(cpu, RM);
    WRITE_VD( fpu::f32_to_ui32(FS1.f32[e]) );
    set_fp_exceptions(cpu);
}


void insn_fdiv_ps(Hart& cpu)
{
    DISASM_FD_FS1_FS2_RM("fdiv.ps");
    throw trap_mcode_instruction(cpu.inst.bits);
}


void insn_feq_ps(Hart& cpu)
{
    require_fp_active();
    DISASM_FD_FS1_FS2("feq.ps");
    WRITE_VD( fpu::f32_eq(FS1.f32[e], FS2.f32[e]) ? UINT32_MAX : 0 );
    set_fp_exceptions(cpu);
}


void insn_feqm_ps(Hart& cpu)
{
    require_fp_active();
    DISASM_MD_FS1_FS2("feqm.ps");
    WRITE_VMD( fpu::f32_eq(FS1.f32[e], FS2.f32[e]) );
    set_fp_exceptions(cpu);
}


void insn_ffrc_ps(Hart& cpu)
{
    require_fp_active();
    DISASM_FD_FS1("ffrc.ps");
    WRITE_VD( fpu::f32_frac(FS1.f32[e]) );
    set_fp_exceptions(cpu);
}


void insn_fle_ps(Hart& cpu)
{
    require_fp_active();
    DISASM_FD_FS1_FS2("fle.ps");
    WRITE_VD( fpu::f32_le(FS1.f32[e], FS2.f32[e]) ? UINT32_MAX : 0 );
    set_fp_exceptions(cpu);
}


void insn_flem_ps(Hart& cpu)
{
    require_fp_active();
    DISASM_MD_FS1_FS2("flem.ps");
    WRITE_VMD( fpu::f32_le(FS1.f32[e], FS2.f32[e]) );
    set_fp_exceptions(cpu);
}


void insn_flt_ps(Hart& cpu)
{
    require_fp_active();
    DISASM_FD_FS1_FS2("flt.ps");
    WRITE_VD( fpu::f32_lt(FS1.f32[e], FS2.f32[e]) ? UINT32_MAX : 0 );
    set_fp_exceptions(cpu);
}


void insn_fltm_ps(Hart& cpu)
{
    require_fp_active();
    DISASM_MD_FS1_FS2("fltm.ps");
    WRITE_VMD( fpu::f32_lt(FS1.f32[e], FS2.f32[e]) );
    set_fp_exceptions(cpu);
}


void insn_fmadd_ps(Hart& cpu)
{
    require_fp_active();
    DISASM_FD_FS1_FS2_FS3_RM("fmadd.ps");
    set_rounding_mode(cpu, RM);
    WRITE_VD( fpu::f32_mulAdd(FS1.f32[e], FS2.f32[e], FS3.f32[e]) );
    set_fp_exceptions(cpu);
}


void insn_fmax_ps(Hart& cpu)
{
    require_fp_active();
    DISASM_FD_FS1_FS2("fmax.ps");
    WRITE_VD( fpu::f32_maximumNumber(FS1.f32[e], FS2.f32[e]) );
    set_fp_exceptions(cpu);
}


void insn_fmin_ps(Hart& cpu)
{
    require_fp_active();
    DISASM_FD_FS1_FS2("fmin.ps");
    WRITE_VD( fpu::f32_minimumNumber(FS1.f32[e], FS2.f32[e]) );
    set_fp_exceptions(cpu);
}


void insn_fmsub_ps(Hart& cpu)
{
    require_fp_active();
    DISASM_FD_FS1_FS2_FS3_RM("fmsub.ps");
    set_rounding_mode(cpu, RM);
    WRITE_VD( fpu::f32_mulSub(FS1.f32[e], FS2.f32[e], FS3.f32[e]) );
    set_fp_exceptions(cpu);
}


void insn_fmul_ps(Hart& cpu)
{
    require_fp_active();
    DISASM_FD_FS1_FS2_RM("fmul.ps");
    set_rounding_mode(cpu, RM);
    WRITE_VD( fpu::f32_mul(FS1.f32[e], FS2.f32[e]) );
    set_fp_exceptions(cpu);
}


void insn_fmvs_x_ps(Hart& cpu)
{
    require_fp_active();
    DISASM_RD_FS1_UIMM3("fmvs.x.ps");
    LATE_WRITE_RD(sext<32>(FS1.u32[UIMM3]));
}


void insn_fmvz_x_ps(Hart& cpu)
{
    require_fp_active();
    DISASM_RD_FS1_UIMM3("fmvz.x.ps");
    LATE_WRITE_RD(FS1.u32[UIMM3]);
}


void insn_fnmadd_ps(Hart& cpu)
{
    require_fp_active();
    DISASM_FD_FS1_FS2_FS3_RM("fnmadd.ps");
    set_rounding_mode(cpu, RM);
    WRITE_VD( fpu::f32_subMulAdd(FS1.f32[e], FS2.f32[e], FS3.f32[e]) );
    set_fp_exceptions(cpu);
}


void insn_fnmsub_ps(Hart& cpu)
{
    require_fp_active();
    DISASM_FD_FS1_FS2_FS3_RM("fnmsub.ps");
    set_rounding_mode(cpu, RM);
    WRITE_VD( fpu::f32_subMulSub(FS1.f32[e], FS2.f32[e], FS3.f32[e]) );
    set_fp_exceptions(cpu);
}


void insn_fround_ps(Hart& cpu)
{
    require_fp_active();
    DISASM_FD_FS1_RM("fround.ps");
    set_rounding_mode(cpu, RM);
    WRITE_VD( fpu::f32_roundToInt(FS1.f32[e]) );
    set_fp_exceptions(cpu);
}


void insn_fsgnj_ps(Hart& cpu)
{
    require_fp_active();
    DISASM_FD_FS1_FS2("fsgnj.ps");
    INTMV_VD( fpu::f32_copySign(FS1.f32[e], FS2.f32[e]) );
}


void insn_fsgnjn_ps(Hart& cpu)
{
    require_fp_active();
    DISASM_FD_FS1_FS2("fsgnjn.ps");
    INTMV_VD( fpu::f32_copySignNot(FS1.f32[e], FS2.f32[e]) );
}


void insn_fsgnjx_ps(Hart& cpu)
{
    require_fp_active();
    DISASM_FD_FS1_FS2("fsgnjx.ps");
    INTMV_VD( fpu::f32_copySignXor(FS1.f32[e], FS2.f32[e]) );
}


void insn_fsqrt_ps(Hart& cpu)
{
    DISASM_FD_FS1_RM("fsqrt.ps");
    throw trap_mcode_instruction(cpu.inst.bits);
}


void insn_fsub_ps(Hart& cpu)
{
    require_fp_active();
    DISASM_FD_FS1_FS2_RM("fsub.ps");
    set_rounding_mode(cpu, RM);
    WRITE_VD( fpu::f32_sub(FS1.f32[e], FS2.f32[e]) );
    set_fp_exceptions(cpu);
}


void insn_fswizz_ps(Hart& cpu)
{
    require_fp_active();
    DISASM_FD_FS1_UIMM8("fswizz.ps");
    freg_t tmp = FS1;
    INTMV_VD( tmp.u32[(e & ~3) | ((UIMM8 >> ((2*e) % 8)) & 3)] );
}


} // namespace bemu
