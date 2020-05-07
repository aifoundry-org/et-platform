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
#include "fpu/fpu.h"
#include "fpu/fpu_casts.h"
#include "insn.h"
#include "insn_func.h"
#include "log.h"
#include "processor.h"
#include "traps.h"
#include "utility.h"

namespace bemu {


extern std::array<Hart,EMU_NUM_THREADS> cpu;


void insn_fadd_s(insn_t inst)
{
    require_fp_active();
    DISASM_FD_FS1_FS2_RM("fadd.s");
    set_rounding_mode(RM);
    WRITE_FD( fpu::f32_add(FS1.f32[0], FS2.f32[0]) );
    set_fp_exceptions();
}


void insn_fdiv_s(insn_t inst)
{
    DISASM_FD_FS1_FS2_RM("fdiv.s");
    throw trap_mcode_instruction(inst.bits);
}


void insn_fmadd_s(insn_t inst)
{
    require_fp_active();
    DISASM_FD_FS1_FS2_FS3_RM("fmadd.s");
    set_rounding_mode(RM);
    WRITE_FD( fpu::f32_mulAdd(FS1.f32[0], FS2.f32[0], FS3.f32[0]) );
    set_fp_exceptions();
}


void insn_fmax_s(insn_t inst)
{
    require_fp_active();
    DISASM_FD_FS1_FS2("fmax.s");
    WRITE_FD( fpu::f32_maximumNumber(FS1.f32[0], FS2.f32[0]) );
    set_fp_exceptions();
}


void insn_fmin_s(insn_t inst)
{
    require_fp_active();
    DISASM_FD_FS1_FS2("fmin.s");
    WRITE_FD( fpu::f32_minimumNumber(FS1.f32[0], FS2.f32[0]) );
    set_fp_exceptions();
}


void insn_fmsub_s(insn_t inst)
{
    require_fp_active();
    DISASM_FD_FS1_FS2_FS3_RM("fmsub.s");
    set_rounding_mode(RM);
    WRITE_FD( fpu::f32_mulSub(FS1.f32[0], FS2.f32[0], FS3.f32[0]) );
    set_fp_exceptions();
}


void insn_fmul_s(insn_t inst)
{
    require_fp_active();
    DISASM_FD_FS1_FS2_RM("fmul.s");
    set_rounding_mode(RM);
    WRITE_FD( fpu::f32_mul(FS1.f32[0], FS2.f32[0]) );
    set_fp_exceptions();
}


void insn_fnmadd_s(insn_t inst)
{
    require_fp_active();
    DISASM_FD_FS1_FS2_FS3_RM("fnmadd.s");
    set_rounding_mode(RM);
    WRITE_FD( fpu::f32_subMulAdd(FS1.f32[0], FS2.f32[0], FS3.f32[0]) );
    set_fp_exceptions();
}


void insn_fnmsub_s(insn_t inst)
{
    require_fp_active();
    DISASM_FD_FS1_FS2_FS3_RM("fnmsub.s");
    set_rounding_mode(RM);
    WRITE_FD( fpu::f32_subMulSub(FS1.f32[0], FS2.f32[0], FS3.f32[0]) );
    set_fp_exceptions();
}


void insn_fsqrt_s(insn_t inst)
{
    DISASM_FD_FS1_RM("fsqrt.s");
    throw trap_mcode_instruction(inst.bits);
}


void insn_fsub_s(insn_t inst)
{
    require_fp_active();
    DISASM_FD_FS1_FS2_RM("fsub.s");
    set_rounding_mode(RM);
    WRITE_FD( fpu::f32_sub(FS1.f32[0], FS2.f32[0]) );
    set_fp_exceptions();
}


void insn_fcvt_l_s(insn_t inst)
{
    DISASM_RD_FS1_RM("fcvt.l.s");
    throw trap_mcode_instruction(inst.bits);
}


void insn_fcvt_lu_s(insn_t inst)
{
    DISASM_RD_FS1_RM("fcvt.lu.s");
    throw trap_mcode_instruction(inst.bits);
}


void insn_fcvt_w_s(insn_t inst)
{
    require_fp_active();
    DISASM_RD_FS1_RM("fcvt.w.s");
    set_rounding_mode(RM);
    int32_t tmp = fpu::f32_to_i32(FS1.f32[0]);
    LATE_WRITE_RD(sext<32>(tmp));
    set_fp_exceptions();
}


void insn_fcvt_wu_s(insn_t inst)
{
    require_fp_active();
    DISASM_RD_FS1_RM("fcvt.wu.s");
    set_rounding_mode(RM);
    uint32_t tmp = fpu::f32_to_ui32(FS1.f32[0]);
    LATE_WRITE_RD(sext<32>(tmp));
    set_fp_exceptions();
}


void insn_fcvt_s_l(insn_t inst)
{
    DISASM_FD_RS1_RM("fcvt.s.l");
    throw trap_mcode_instruction(inst.bits);
}


void insn_fcvt_s_lu(insn_t inst)
{
    DISASM_FD_RS1_RM("fcvt.s.lu");
    throw trap_mcode_instruction(inst.bits);
}


void insn_fcvt_s_w(insn_t inst)
{
    require_fp_active();
    DISASM_FD_RS1_RM("fcvt.s.w");
    set_rounding_mode(RM);
    WRITE_FD( fpu::i32_to_f32(RS1) );
    set_fp_exceptions();
}


void insn_fcvt_s_wu(insn_t inst)
{
    require_fp_active();
    DISASM_FD_RS1_RM("fcvt.s.wu");
    set_rounding_mode(RM);
    WRITE_FD( fpu::ui32_to_f32(RS1) );
    set_fp_exceptions();
}


void insn_fsgnj_s(insn_t inst)
{
    require_fp_active();
    DISASM_FD_FS1_FS2("fsgnj.s");
    WRITE_FD( fpu::f32_copySign(FS1.f32[0], FS2.f32[0]) );
}


void insn_fsgnjn_s(insn_t inst)
{
    require_fp_active();
    DISASM_FD_FS1_FS2("fsgnjn.s");
    WRITE_FD( fpu::f32_copySignNot(FS1.f32[0], FS2.f32[0]) );
}


void insn_fsgnjx_s(insn_t inst)
{
    require_fp_active();
    DISASM_FD_FS1_FS2("fsgnjx.s");
    WRITE_FD( fpu::f32_copySignXor(FS1.f32[0], FS2.f32[0]) );
}


void insn_fmv_w_x(insn_t inst)
{
    require_fp_active();
    DISASM_FD_RS1("fmv.w.x");
    WRITE_FD( uint32_t(RS1) );
}


void insn_fmv_x_w(insn_t inst)
{
    require_fp_active();
    DISASM_RD_FS1("fmv.x.w");
    LATE_WRITE_RD(sext<32>(FS1.u32[0]));
}


void insn_feq_s(insn_t inst)
{
    require_fp_active();
    DISASM_RD_FS1_FS2("feq.s");
    bool tmp = fpu::f32_eq(FS1.f32[0], FS2.f32[0]);
    LATE_WRITE_RD(tmp);
    set_fp_exceptions();
}


void insn_fle_s(insn_t inst)
{
    require_fp_active();
    DISASM_RD_FS1_FS2("fle.s");
    bool tmp = fpu::f32_le(FS1.f32[0], FS2.f32[0]);
    LATE_WRITE_RD(tmp);
    set_fp_exceptions();
}


void insn_flt_s(insn_t inst)
{
    require_fp_active();
    DISASM_RD_FS1_FS2("flt.s");
    bool tmp = fpu::f32_lt(FS1.f32[0], FS2.f32[0]);
    LATE_WRITE_RD(tmp);
    set_fp_exceptions();
}


void insn_fclass_s(insn_t inst)
{
    require_fp_active();
    DISASM_RD_FS1("fclass.s");
    LATE_WRITE_RD(sext<32>(fpu::f32_classify(FS1.f32[0])));
}


} // namespace bemu
