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
#include "insn.h"
#include "insn_func.h"
#include "insn_util.h"
#include "log.h"
#include "processor.h"
#include "traps.h"
#include "utility.h"

namespace bemu {


static inline uint8_t sat8(int32_t x)
{ return std::min(INT8_MAX, std::max(INT8_MIN, x)); }


static inline uint8_t satu8(int32_t x)
{ return std::min(UINT8_MAX, std::max(0, x)); }


void insn_fadd_pi(Hart& cpu)
{
    require_fp_active();
    DISASM_FD_FS1_FS2("fadd.pi");
    WRITE_VD( FS1.u32[e] + FS2.u32[e] );
}


void insn_faddi_pi(Hart& cpu)
{
    require_fp_active();
    DISASM_FD_FS1_VIMM("faddi.pi");
    WRITE_VD( FS1.u32[e] + VIMM );
}


void insn_fand_pi(Hart& cpu)
{
    require_fp_active();
    DISASM_FD_FS1_FS2("fand.pi");
    WRITE_VD( FS1.u32[e] & FS2.u32[e] );
}


void insn_fandi_pi(Hart& cpu)
{
    require_fp_active();
    DISASM_FD_FS1_UVIMM("fandi.pi");
    WRITE_VD( FS1.u32[e] & uint32_t(VIMM) );
}


void insn_fbci_pi(Hart& cpu)
{
    require_fp_active();
    DISASM_FD_I32IMM("fbci.pi");
    WRITE_VD( I32IMM );
}


void insn_fdiv_pi(Hart& cpu)
{
    DISASM_FD_FS1_FS2("fdiv.pi");
    throw trap_mcode_instruction(cpu.inst.bits);
}


void insn_fdivu_pi(Hart& cpu)
{
    DISASM_FD_FS1_FS2("fdivu.pi");
    throw trap_mcode_instruction(cpu.inst.bits);
}


void insn_feq_pi(Hart& cpu)
{
    require_fp_active();
    DISASM_FD_FS1_FS2("feq.pi");
    WRITE_VD( (FS1.u32[e] == FS2.u32[e]) ? UINT32_MAX : 0 );
}


void insn_fle_pi(Hart& cpu)
{
    require_fp_active();
    DISASM_FD_FS1_FS2("fle.pi");
    WRITE_VD( (FS1.i32[e] <= FS2.i32[e]) ? UINT32_MAX : 0 );
}


void insn_flt_pi(Hart& cpu)
{
    require_fp_active();
    DISASM_FD_FS1_FS2("flt.pi");
    WRITE_VD( (FS1.i32[e] < FS2.i32[e]) ? UINT32_MAX : 0 );
}


void insn_fltm_pi(Hart& cpu)
{
    require_fp_active();
    DISASM_MD_FS1_FS2("fltm.pi");
    WRITE_VMD( FS1.i32[e] < FS2.i32[e] );
}


void insn_fltu_pi(Hart& cpu)
{
    require_fp_active();
    DISASM_FD_FS1_FS2("fltu.pi");
    WRITE_VD( (FS1.u32[e] < FS2.u32[e]) ? UINT32_MAX : 0 );
}


void insn_fmax_pi(Hart& cpu)
{
    require_fp_active();
    DISASM_FD_FS1_FS2("fmax.pi");
    WRITE_VD( std::max(FS1.i32[e], FS2.i32[e]) );
}


void insn_fmaxu_pi(Hart& cpu)
{
    require_fp_active();
    DISASM_FD_FS1_FS2("fmaxu.pi");
    WRITE_VD( std::max(FS1.u32[e], FS2.u32[e]) );
}


void insn_fmin_pi(Hart& cpu)
{
    require_fp_active();
    DISASM_FD_FS1_FS2("fmin.pi");
    WRITE_VD( std::min(FS1.i32[e], FS2.i32[e]) );
}


void insn_fminu_pi(Hart& cpu)
{
    require_fp_active();
    DISASM_FD_FS1_FS2("fminu.pi");
    WRITE_VD( std::min(FS1.u32[e], FS2.u32[e]) );
}


void insn_fmul_pi(Hart& cpu)
{
    require_fp_active();
    DISASM_FD_FS1_FS2("fmul.pi");
    WRITE_VD( FS1.u32[e] * FS2.u32[e] );
}


void insn_fmulh_pi(Hart& cpu)
{
    require_fp_active();
    DISASM_FD_FS1_FS2("fmulh.pi");
    WRITE_VD( (int64_t(FS1.i32[e]) * int64_t(FS2.i32[e]) >> 32) );
}


void insn_fmulhu_pi(Hart& cpu)
{
    require_fp_active();
    DISASM_FD_FS1_FS2("fmulhu.pi");
    WRITE_VD( (uint64_t(FS1.u32[e]) * uint64_t(FS2.u32[e]) >> 32) );
}


void insn_fnot_pi(Hart& cpu)
{
    require_fp_active();
    DISASM_FD_FS1("fnot.pi");
    WRITE_VD( ~FS1.u32[e] );
}


void insn_for_pi(Hart& cpu)
{
    require_fp_active();
    DISASM_FD_FS1_FS2("for.pi");
    WRITE_VD( FS1.u32[e] | FS2.u32[e] );
}


void insn_fpackrepb_pi(Hart& cpu)
{
    require_fp_active();
    DISASM_FD_FS1("fpackrepb.pi");
    freg_t tmp = FS1;
    WRITE_VD( uint32_t(tmp.u8[(16*e+0)%(VLEN/8)])
              | (uint32_t(tmp.u8[(16*e+4)%(VLEN/8)]) << 8)
              | (uint32_t(tmp.u8[(16*e+8)%(VLEN/8)]) << 16)
              | (uint32_t(tmp.u8[(16*e+12)%(VLEN/8)]) << 24) );
}


void insn_fpackreph_pi(Hart& cpu)
{
    require_fp_active();
    DISASM_FD_FS1("fpackreph.pi");
    freg_t tmp = FS1;
    WRITE_VD( uint32_t(tmp.u16[(4*e+0)%(VLEN/16)])
              | (uint32_t(tmp.u16[(4*e+2)%(VLEN/16)]) << 16) );
}


void insn_frem_pi(Hart& cpu)
{
    DISASM_FD_FS1_FS2("frem.pi");
    throw trap_mcode_instruction(cpu.inst.bits);
}


void insn_fremu_pi(Hart& cpu)
{
    DISASM_FD_FS1_FS2("fremu.pi");
    throw trap_mcode_instruction(cpu.inst.bits);
}


void insn_fsat8_pi(Hart& cpu)
{
    require_fp_active();
    DISASM_FD_FS1("fsat8.pi");
    WRITE_VD( sat8(FS1.i32[e]) );
}


void insn_fsatu8_pi(Hart& cpu)
{
    require_fp_active();
    DISASM_FD_FS1("fsatu8.pi");
    WRITE_VD( satu8(FS1.i32[e]) );
}


void insn_fsetm_pi(Hart& cpu)
{
    require_fp_active();
    DISASM_MD_FS1("fsetm.pi");
    WRITE_VMD( !!FS1.u32[e] );
}


void insn_fsll_pi(Hart& cpu)
{
    require_fp_active();
    DISASM_FD_FS1_FS2("fsll.pi");
    WRITE_VD( (FS2.u32[e] >= 32) ? 0 : (FS1.u32[e] << FS2.u32[e]) );
}


void insn_fslli_pi(Hart& cpu)
{
    require_fp_active();
    DISASM_FD_FS1_SHAMT5("fslli.pi");
    WRITE_VD( FS1.u32[e] << SHAMT5 );
}


void insn_fsra_pi(Hart& cpu)
{
    require_fp_active();
    DISASM_FD_FS1_FS2("fsra.pi");
    WRITE_VD( FS1.i32[e] >> std::min(FS2.u32[e], 31u) );
}


void insn_fsrai_pi(Hart& cpu)
{
    require_fp_active();
    DISASM_FD_FS1_SHAMT5("fsrai.pi");
    WRITE_VD( FS1.i32[e] >> SHAMT5 );
}


void insn_fsrl_pi(Hart& cpu)
{
    require_fp_active();
    DISASM_FD_FS1_FS2("fsrl.pi");
    WRITE_VD( (FS2.u32[e] >= 32) ? 0 : (FS1.u32[e] >> FS2.u32[e]) );
}


void insn_fsrli_pi(Hart& cpu)
{
    require_fp_active();
    DISASM_FD_FS1_SHAMT5("fsrli.pi");
    WRITE_VD( FS1.u32[e] >> SHAMT5 );
}


void insn_fsub_pi(Hart& cpu)
{
    require_fp_active();
    DISASM_FD_FS1_FS2("fsub.pi");
    WRITE_VD( FS1.u32[e] - FS2.u32[e] );
}


void insn_fxor_pi(Hart& cpu)
{
    require_fp_active();
    DISASM_FD_FS1_FS2("fxor.pi");
    WRITE_VD( FS1.u32[e] ^ FS2.u32[e] );
}


} // namespace bemu
