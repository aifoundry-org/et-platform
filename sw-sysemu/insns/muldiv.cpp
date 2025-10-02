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
#include "insn.h"
#include "insn_func.h"
#include "insn_util.h"
#include "log.h"
#include "processor.h"
#include "utility.h"

namespace bemu {


static inline int64_t idiv(int64_t a, int64_t b)
{ return b ? ((a == INT64_MIN && b == -1LL) ? a : (a / b)) : UINT64_MAX; }


static inline uint64_t udiv(uint64_t a, uint64_t b)
{ return b ? (a / b) : UINT64_MAX; }


static inline uint64_t udivw(uint64_t a, uint64_t b)
{ return b ? sext<32>(a / b) : UINT64_MAX; }


static inline int64_t idivw(int64_t a, int64_t b)
{ return b ? sext<32>(a / b) : UINT64_MAX; }


static inline int64_t mulh(int64_t a, int64_t b)
{ return (__int128_t(a) * __int128_t(b)) >> 64; }


static inline int64_t mulhsu(int64_t a, uint64_t b)
{ return (__int128_t(a) * __uint128_t(b)) >> 64; }


static inline uint64_t mulhu(uint64_t a, uint64_t b)
{ return (__uint128_t(a) * __uint128_t(b)) >> 64; }


static inline int64_t irem(int64_t a, int64_t b)
{ return b ? ((a == INT64_MIN && b == -1LL) ? 0 : (a % b)) : a; }


static inline uint64_t urem(uint64_t a, uint64_t b)
{ return b ? (a % b) : a; }


static inline uint64_t uremw(uint64_t a, uint64_t b)
{ return b ? sext<32>(a % b) : sext<32>(a); }


static inline int64_t iremw(int64_t a, int64_t b)
{ return (b == 0) ? a : sext<32>(a % b); }


void insn_div(Hart& cpu)
{
    DISASM_RD_RS1_RS2("div");
    LATE_WRITE_RD(idiv(RS1, RS2));
}


void insn_divu(Hart& cpu)
{
    DISASM_RD_RS1_RS2("divu");
    LATE_WRITE_RD(udiv(RS1, RS2));
}


void insn_divuw(Hart& cpu)
{
    DISASM_RD_RS1_RS2("divuw");
    LATE_WRITE_RD(udivw(uint32_t(RS1), uint32_t(RS2)));
}


void insn_divw(Hart& cpu)
{
    DISASM_RD_RS1_RS2("divw");
    LATE_WRITE_RD(idivw(sext<32>(RS1), sext<32>(RS2)));
}


void insn_mul(Hart& cpu)
{
    DISASM_RD_RS1_RS2("mul");
    LATE_WRITE_RD(RS1 * RS2);
}


void insn_mulh(Hart& cpu)
{
    DISASM_RD_RS1_RS2("mulh");
    LATE_WRITE_RD(mulh(RS1, RS2));
}


void insn_mulhsu(Hart& cpu)
{
    DISASM_RD_RS1_RS2("mulhsu");
    LATE_WRITE_RD(mulhsu(RS1, RS2));
}


void insn_mulhu(Hart& cpu)
{
    DISASM_RD_RS1_RS2("mulhu");
    LATE_WRITE_RD(mulhu(RS1, RS2));
}


void insn_mulw(Hart& cpu)
{
    DISASM_RD_RS1_RS2("mulw");
    LATE_WRITE_RD(sext<32>(RS1 * RS2));
}


void insn_rem(Hart& cpu)
{
    DISASM_RD_RS1_RS2("rem");
    LATE_WRITE_RD(irem(RS1, RS2));
}


void insn_remu(Hart& cpu)
{
    DISASM_RD_RS1_RS2("remu");
    LATE_WRITE_RD(urem(RS1, RS2));
}


void insn_remuw(Hart& cpu)
{
    DISASM_RD_RS1_RS2("remuw");
    LATE_WRITE_RD(uremw(uint32_t(RS1), uint32_t(RS2)));
}


void insn_remw(Hart& cpu)
{
    DISASM_RD_RS1_RS2("remw");
    LATE_WRITE_RD(iremw(sext<32>(RS1), sext<32>(RS2)));
}


} // namespace bemu
