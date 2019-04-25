/* vim: set ts=8 sw=4 et sta cin cino=\:0s,l1,g0,N-s,E-s,i0,+2s,(0,W2s : */

#include "decode.h"
#include "emu_gio.h"
#include "insn.h"
#include "insn_func.h"
#include "log.h"
#include "utility.h"

// FIXME: Replace with "state.h"
extern uint8_t csr_prv[EMU_NUM_THREADS];

//namespace bemu {


void insn_fence_i(insn_t inst)
{
    DISASM_NOARG("fence.i");
    throw trap_mcode_instruction(inst.bits);
}


//} // namespace bemu
