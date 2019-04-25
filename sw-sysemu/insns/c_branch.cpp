/* vim: set ts=8 sw=4 et sta cin cino=\:0s,l1,g0,N-s,E-s,i0,+2s,(0,W2s : */

#include "decode.h"
#include "emu_gio.h"
#include "insn.h"
#include "insn_func.h"
#include "log.h"
#include "utility.h"

// FIXME: Replace with "state.h"
#include "emu_defines.h"
extern uint64_t xregs[EMU_NUM_THREADS][NXREGS];
extern uint8_t csr_prv[EMU_NUM_THREADS];
extern uint64_t current_pc;


// namespace bemu {


void insn_c_beqz(insn_t inst)
{
    C_DISASM_RS1P_BIMM("c.beqz");
    if (C_RS1P == 0)
        WRITE_PC(PC + C_BIMM);
}


void insn_c_bnez(insn_t inst)
{
    C_DISASM_RS1P_BIMM("c.bneqz");
    if (C_RS1P != 0)
        WRITE_PC(PC + C_BIMM);
}


void insn_c_j(insn_t inst)
{
    C_DISASM_JIMM("c.j");
    WRITE_X0(NPC);
    WRITE_PC(PC + C_JIMM);
}


void insn_c_jalr(insn_t inst)
{
    C_DISASM_RS1("c.jalr");
    uint64_t tmp = C_RS1 & ~1ull;
    WRITE_X1(NPC);
    WRITE_PC(tmp);
}


void insn_c_jr(insn_t inst)
{
    C_DISASM_RS1("c.jr");
    WRITE_X0(NPC);
    WRITE_PC(C_RS1 & ~1ull);
}


//} // namespace bemu
