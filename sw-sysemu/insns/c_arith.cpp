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
#include "insn.h"
#include "insn_func.h"
#include "log.h"
#include "processor.h"
#include "utility.h"

namespace bemu {


void insn_c_add(Hart& cpu) {
    C_DISASM_RDS1_RS2("c.add");
    WRITE_C_RS1(C_RS1 + C_RS2);
}


void insn_c_addi(Hart& cpu)
{
    C_DISASM_RDS1_IMM6("c.addi");
    WRITE_C_RS1(C_RS1 + C_IMM6);
}


void insn_c_addi16sp(Hart& cpu)
{
    C_DISASM_NZIMMADDI16SP("c.addi16sp");
    WRITE_X2(X2 + C_NZIMMADDI16SP);
}


void insn_c_addi4spn(Hart& cpu)
{
    C_DISASM_RS2P_NZUIMMADDI4SPN("c.addi4spn");
    WRITE_C_RS2P(X2 + C_NZUIMMADDI4SPN);
}


void insn_c_addiw(Hart& cpu)
{
    C_DISASM_RDS1_IMM6("c.addiw");
    WRITE_C_RS1(sext<32>(C_RS1 + C_IMM6));
}


void insn_c_addw(Hart& cpu) {
    C_DISASM_RDS1P_RS2P("c.addw");
    WRITE_C_RS1P(sext<32>(C_RS1P + C_RS2P));
}


void insn_c_and(Hart& cpu) {
    C_DISASM_RDS1P_RS2P("c.and");
    WRITE_C_RS1P(C_RS1P & C_RS2P);
}


void insn_c_andi(Hart& cpu)
{
    C_DISASM_RDS1P_IMM6("c.andi");
    WRITE_C_RS1P(C_RS1P & C_IMM6);
}


void insn_c_illegal(Hart& cpu)
{
    DISASM_NOARG("illegal compressed opcode");
    throw trap_illegal_instruction(0);
}


void insn_c_li(Hart& cpu)
{
    C_DISASM_RS1_IMM6("c.li");
    WRITE_C_RS1(C_IMM6);
}


void insn_c_lui(Hart& cpu)
{
    C_DISASM_RS1_NZIMMLUI("c.lui");
    WRITE_C_RS1(C_NZIMMLUI);
}


void insn_c_mv(Hart& cpu) {
    C_DISASM_RS1_RS2("c.mv");
    WRITE_C_RS1(C_RS2);
}


void insn_c_or(Hart& cpu) {
    C_DISASM_RDS1P_RS2P("c.or");
    WRITE_C_RS1P(C_RS1P | C_RS2P);
}


void insn_c_reserved(Hart& cpu)
{
    DISASM_NOARG("illegal compressed opcode");
    throw trap_illegal_instruction(0);
}


void insn_c_slli(Hart& cpu)
{
    C_DISASM_RDS1_SHAMT("c.slli");
    WRITE_C_RS1(C_RS1 << C_SHAMT);
}


void insn_c_srai(Hart& cpu)
{
    C_DISASM_RDS1P_SHAMT("c.srai");
    WRITE_C_RS1P(int64_t(C_RS1P) >> C_SHAMT);
}


void insn_c_srli(Hart& cpu)
{
    C_DISASM_RDS1P_SHAMT("c.srli");
    WRITE_C_RS1P(C_RS1P >> C_SHAMT);
}


void insn_c_sub(Hart& cpu) {
    C_DISASM_RDS1P_RS2P("c.sub");
    WRITE_C_RS1P(C_RS1P - C_RS2P);
}


void insn_c_subw(Hart& cpu) {
    C_DISASM_RDS1P_RS2P("c.subw");
    WRITE_C_RS1P(sext<32>(C_RS1P - C_RS2P));
}


void insn_c_xor(Hart& cpu) {
    C_DISASM_RDS1P_RS2P("c.xor");
    WRITE_C_RS1P(C_RS1P ^ C_RS2P);
}


} // namespace bemu
