/* vim: set ts=8 sw=4 et sta cin cino=\:0s,l1,g0,N-s,E-s,i0,+2s,(0,W2s : */

#include "decode.h"
#include "emu_gio.h"
#include "insn.h"
#include "insn_func.h"
#include "log.h"
#include "mmu.h"
#include "processor.h"
#include "utility.h"
#include "atomics.h"
#include "fpu/fpu_casts.h"

// FIXME: Replace with "processor.h"
#include "emu_defines.h"
extern std::array<Processor,EMU_NUM_THREADS> cpu;

//namespace bemu {


void insn_famoaddg_pi(insn_t inst)
{
    DISASM_AMO_FD_FS1_RS2("famoaddg.pi");
    GSCAMO(mmu_global_atomic32(RS2 + FS1.i32[e], FD.u32[e], std::plus<uint32_t>()));
}


void insn_famoaddl_pi(insn_t inst)
{
    DISASM_AMO_FD_FS1_RS2("famoaddl.pi");
    GSCAMO(mmu_local_atomic32(RS2 + FS1.i32[e], FD.u32[e], std::plus<uint32_t>()));
}


void insn_famoandg_pi(insn_t inst)
{
    DISASM_AMO_FD_FS1_RS2("famoandg.pi");
    GSCAMO(mmu_global_atomic32(RS2 + FS1.i32[e], FD.u32[e], std::bit_and<uint32_t>()));
}


void insn_famoandl_pi(insn_t inst)
{
    DISASM_AMO_FD_FS1_RS2("famoandl.pi");
    GSCAMO(mmu_local_atomic32(RS2 + FS1.i32[e], FD.u32[e], std::bit_and<uint32_t>()));
}


void insn_famomaxg_pi(insn_t inst)
{
    DISASM_AMO_FD_FS1_RS2("famomaxg.pi");
    GSCAMO(mmu_global_atomic32(RS2 + FS1.i32[e], FD.u32[e], maximum<int32_t>()));
}


void insn_famomaxg_ps(insn_t inst)
{
    DISASM_AMO_FD_FS1_RS2("famomaxg.ps");
    GSCAMO(mmu_global_atomic32(RS2 + FS1.i32[e], FD.u32[e], f32_maximum()));
}


void insn_famomaxl_pi(insn_t inst)
{
    DISASM_AMO_FD_FS1_RS2("famomaxl.pi");
    GSCAMO(mmu_local_atomic32(RS2 + FS1.i32[e], FD.u32[e], maximum<int32_t>()));
}


void insn_famomaxl_ps(insn_t inst)
{
    DISASM_AMO_FD_FS1_RS2("famomaxl.ps");
    GSCAMO(mmu_local_atomic32(RS2 + FS1.i32[e], FD.u32[e], f32_maximum()));
}


void insn_famomaxug_pi(insn_t inst)
{
    DISASM_AMO_FD_FS1_RS2("famomaxug.pi");
    GSCAMO(mmu_global_atomic32(RS2 + FS1.i32[e], FD.u32[e], maximum<uint32_t>()));
}


void insn_famomaxul_pi(insn_t inst)
{
    DISASM_AMO_FD_FS1_RS2("famomaxul.pi");
    GSCAMO(mmu_local_atomic32(RS2 + FS1.i32[e], FD.u32[e], maximum<uint32_t>()));
}


void insn_famoming_pi(insn_t inst)
{
    DISASM_AMO_FD_FS1_RS2("famoming.pi");
    GSCAMO(mmu_global_atomic32(RS2 + FS1.i32[e], FD.u32[e], minimum<int32_t>()));
}


void insn_famoming_ps(insn_t inst)
{
    DISASM_AMO_FD_FS1_RS2("famoming.ps");
    GSCAMO(mmu_global_atomic32(RS2 + FS1.i32[e], FD.u32[e], f32_minimum()));
}


void insn_famominl_pi(insn_t inst)
{
    DISASM_AMO_FD_FS1_RS2("famominl.pi");
    GSCAMO(mmu_local_atomic32(RS2 + FS1.i32[e], FD.u32[e], minimum<int32_t>()));
}


void insn_famominl_ps(insn_t inst)
{
    DISASM_AMO_FD_FS1_RS2("famominl.ps");
    GSCAMO(mmu_local_atomic32(RS2 + FS1.i32[e], FD.u32[e], f32_minimum()));
}


void insn_famominug_pi(insn_t inst)
{
    DISASM_AMO_FD_FS1_RS2("famominug.pi");
    GSCAMO(mmu_global_atomic32(RS2 + FS1.i32[e], FD.u32[e], minimum<uint32_t>()));
}


void insn_famominul_pi(insn_t inst)
{
    DISASM_AMO_FD_FS1_RS2("famominul.pi");
    GSCAMO(mmu_local_atomic32(RS2 + FS1.i32[e], FD.u32[e], minimum<uint32_t>()));
}


void insn_famoorg_pi(insn_t inst)
{
    DISASM_AMO_FD_FS1_RS2("famoorg.pi");
    GSCAMO(mmu_global_atomic32(RS2 + FS1.i32[e], FD.u32[e], std::bit_or<uint32_t>()));
}


void insn_famoorl_pi(insn_t inst)
{
    DISASM_AMO_FD_FS1_RS2("famoorl.pi");
    GSCAMO(mmu_local_atomic32(RS2 + FS1.i32[e], FD.u32[e], std::bit_or<uint32_t>()));
}


void insn_famoswapg_pi(insn_t inst)
{
    DISASM_AMO_FD_FS1_RS2("famoswapg.pi");
    GSCAMO(mmu_global_atomic32(RS2 + FS1.i32[e], FD.u32[e], replace<uint32_t>()));
}


void insn_famoswapl_pi(insn_t inst)
{
    DISASM_AMO_FD_FS1_RS2("famoswapl.pi");
    GSCAMO(mmu_local_atomic32(RS2 + FS1.i32[e], FD.u32[e], replace<uint32_t>()));
}


void insn_famoxorg_pi(insn_t inst)
{
    DISASM_AMO_FD_FS1_RS2("famoxorg.pi");
    GSCAMO(mmu_global_atomic32(RS2 + FS1.i32[e], FD.u32[e], std::bit_xor<uint32_t>()));
}


void insn_famoxorl_pi(insn_t inst)
{
    DISASM_AMO_FD_FS1_RS2("famoxorl.pi");
    GSCAMO(mmu_local_atomic32(RS2 + FS1.i32[e], FD.u32[e], std::bit_xor<uint32_t>()));
}


//} // namespace bemu
