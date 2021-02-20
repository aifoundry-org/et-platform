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


void insn_c_beqz(Hart& cpu)
{
    C_DISASM_RS1P_BIMM("c.beqz");
    if (C_RS1P == 0)
        WRITE_PC(PC + C_BIMM);
}


void insn_c_bnez(Hart& cpu)
{
    C_DISASM_RS1P_BIMM("c.bneqz");
    if (C_RS1P != 0)
        WRITE_PC(PC + C_BIMM);
}


void insn_c_j(Hart& cpu)
{
    C_DISASM_JIMM("c.j");
    WRITE_X0(NPC);
    WRITE_PC(PC + C_JIMM);
}


void insn_c_jalr(Hart& cpu)
{
    C_DISASM_RS1("c.jalr");
    uint64_t tmp = C_RS1 & ~1ull;
    WRITE_X1(NPC);
    WRITE_PC(tmp);
}


void insn_c_jr(Hart& cpu)
{
    C_DISASM_RS1("c.jr");
    WRITE_X0(NPC);
    WRITE_PC(C_RS1 & ~1ull);
}


} // namespace bemu
