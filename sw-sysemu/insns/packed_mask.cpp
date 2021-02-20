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
#include "esrs.h"
#include "fpu/fpu.h"
#include "fpu/fpu_casts.h"
#include "insn.h"
#include "insn.h"
#include "insn_func.h"
#include "insn_util.h"
#include "log.h"
#include "processor.h"
#include "system.h"
#include "traps.h"
#include "utility.h"

namespace bemu {


static inline size_t popcount1(const mreg_t& m)
{ return m.count(); }


static inline size_t popcount0(const mreg_t& m)
{ return MLEN - m.count(); }


void insn_maskand(Hart& cpu)
{
    require_fp_active();
    DISASM_MD_MS1_MS2("maskand");
    WRITE_MD(MS1 & MS2);
}


void insn_masknot(Hart& cpu)
{
    require_fp_active();
    DISASM_MD_MS1("masknot");
    WRITE_MD(~MS1);
}


void insn_maskor(Hart& cpu)
{
    require_fp_active();
    DISASM_MD_MS1_MS2("maskor");
    WRITE_MD(MS1 | MS2);
}


void insn_maskpopc(Hart& cpu)
{
    require_fp_active();
    DISASM_RD_MS1("maskpopc");
    LATE_WRITE_RD(popcount1(MS1));
}

// LCOV_EXCL_START
void insn_maskpopc_rast(Hart& cpu)
{
    require_feature_gfx();
    require_fp_active();
    DISASM_RD_MS1_MS2_UMSK4("maskpopc.rast");
    mreg_t m1, m2;
    switch (UMSK4) {
    case 0  : m2 = 0x0f; m1 = 0x0f; break;
    case 1  : m2 = 0x3c; m1 = 0x3c; break;
    case 2  : m2 = 0xf0; m1 = 0xf0; break;
    default : m2 = 0xff; m1 = 0xff; break;
    }
    LATE_WRITE_RD(popcount1(MS1 & m1) + popcount1(MS2 & m2));
}
// LCOV_EXCL_STOP

void insn_maskpopcz(Hart& cpu)
{
    require_fp_active();
    DISASM_RD_MS1("maskpopcz");
    LATE_WRITE_RD(popcount0(MS1));
}


void insn_maskxor(Hart& cpu)
{
    require_fp_active();
    DISASM_MD_MS1_MS2("maskxor");
    WRITE_MD(MS1 ^ MS2);
}


void insn_mov_m_x(Hart& cpu)
{
    require_fp_active();
    DISASM_MD_RS1_UIMM8("mov.m.x");
    WRITE_MD(uint32_t(RS1) | UIMM8);
}


void insn_mova_m_x(Hart& cpu)
{
    require_fp_active();
    DISASM_RS1("mova.m.x");
    uint64_t tmp = RS1;
    WRITE_MREG(0, tmp >> (0*MLEN));
    WRITE_MREG(1, tmp >> (1*MLEN));
    WRITE_MREG(2, tmp >> (2*MLEN));
    WRITE_MREG(3, tmp >> (3*MLEN));
    WRITE_MREG(4, tmp >> (4*MLEN));
    WRITE_MREG(5, tmp >> (5*MLEN));
    WRITE_MREG(6, tmp >> (6*MLEN));
    WRITE_MREG(7, tmp >> (7*MLEN));
}


void insn_mova_x_m(Hart& cpu)
{
    require_fp_active();
    DISASM_RD_ALLMASK("mova.x.m");
    LATE_WRITE_RD((cpu.mregs[0].to_ullong() << (0*MLEN)) +
                  (cpu.mregs[1].to_ullong() << (1*MLEN)) +
                  (cpu.mregs[2].to_ullong() << (2*MLEN)) +
                  (cpu.mregs[3].to_ullong() << (3*MLEN)) +
                  (cpu.mregs[4].to_ullong() << (4*MLEN)) +
                  (cpu.mregs[5].to_ullong() << (5*MLEN)) +
                  (cpu.mregs[6].to_ullong() << (6*MLEN)) +
                  (cpu.mregs[7].to_ullong() << (7*MLEN)));
}


} // namespace bemu
