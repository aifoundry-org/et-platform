/* vim: set ts=8 sw=4 et sta cin cino=\:0s,l1,g0,N-s,E-s,i0,+2s,(0,W2s : */

#include "decode.h"
#include "emu_gio.h"
#include "insn.h"
#include "insn_func.h"
#include "log.h"
#include "mmu.h"
#include "utility.h"
#include "fpu/fpu_casts.h"

// FIXME: Replace with "state.h"
#include "emu_defines.h"
extern uint64_t xregs[EMU_NUM_THREADS][NXREGS];
extern freg_t   fregs[EMU_NUM_THREADS][NFREGS];
extern uint8_t csr_prv[EMU_NUM_THREADS];
extern uint32_t csr_fcsr[EMU_NUM_THREADS];
extern uint64_t csr_mstatus[EMU_NUM_THREADS];

//namespace bemu {


void insn_flw(insn_t inst)
{
    require_fp_active();
    DISASM_LOAD_FD_RS1_IIMM("flw");
    WRITE_FD(mmu_load32(RS1 + IIMM));
}


void insn_fsw(insn_t inst)
{
    require_fp_active();
    DISASM_STORE_FS2_RS1_SIMM("fsw");
    mmu_store32(RS1 + SIMM, FS2.u32[0]);
}


//} // namespace bemu
