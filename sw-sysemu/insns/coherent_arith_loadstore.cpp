/* vim: set ts=8 sw=4 et sta cin cino=\:0s,l1,g0,N-s,E-s,i0,+2s,(0,W2s : */

#include "decode.h"
#include "emu_gio.h"
#include "insn.h"
#include "insn_func.h"
#include "log.h"
#include "mmu.h"
#include "utility.h"

// FIXME: Replace with "state.h"
#include "emu_defines.h"
extern uint64_t xregs[EMU_NUM_THREADS][NXREGS];
extern uint8_t csr_prv[EMU_NUM_THREADS];

//namespace bemu {


void insn_sbg(insn_t inst)
{
    DISASM_STORE_RS2_RS1("sbg");
    mmu_store8(RS1, uint8_t(RS2));
}


void insn_sbl(insn_t inst)
{
    DISASM_STORE_RS2_RS1("sbl");
    mmu_store8(RS1, uint8_t(RS2));
}


void insn_shg(insn_t inst)
{
    DISASM_STORE_RS2_RS1("shg");
    mmu_aligned_store16(RS1, uint16_t(RS2));
}


void insn_shl(insn_t inst)
{
    DISASM_STORE_RS2_RS1("shl");
    mmu_aligned_store16(RS1, uint16_t(RS2));
}


//} // namespace bemu
