/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#include "atomics.h"
#include "emu_defines.h"
#include "emu_gio.h"
#include "fpu/fpu_casts.h"
#include "insn.h"
#include "insn_func.h"
#include "insn_util.h"
#include "log.h"
#include "mmu.h"
#include "processor.h"
#include "utility.h"

namespace bemu {


void insn_famoaddg_pi(Hart& cpu)
{
    require_fp_active();
    DISASM_AMO_FD_FS1_RS2("famoaddg.pi");
    GSCAMO(mmu_global_atomic32(cpu, RS2 + FS1.i32[e], FD.u32[e], std::plus<uint32_t>()));
}


void insn_famoaddl_pi(Hart& cpu)
{
    require_fp_active();
    DISASM_AMO_FD_FS1_RS2("famoaddl.pi");
    GSCAMO(mmu_local_atomic32(cpu, RS2 + FS1.i32[e], FD.u32[e], std::plus<uint32_t>()));
}


void insn_famoandg_pi(Hart& cpu)
{
    require_fp_active();
    DISASM_AMO_FD_FS1_RS2("famoandg.pi");
    GSCAMO(mmu_global_atomic32(cpu, RS2 + FS1.i32[e], FD.u32[e], std::bit_and<uint32_t>()));
}


void insn_famoandl_pi(Hart& cpu)
{
    require_fp_active();
    DISASM_AMO_FD_FS1_RS2("famoandl.pi");
    GSCAMO(mmu_local_atomic32(cpu, RS2 + FS1.i32[e], FD.u32[e], std::bit_and<uint32_t>()));
}


void insn_famomaxg_pi(Hart& cpu)
{
    require_fp_active();
    DISASM_AMO_FD_FS1_RS2("famomaxg.pi");
    GSCAMO(mmu_global_atomic32(cpu, RS2 + FS1.i32[e], FD.u32[e], maximum<int32_t>()));
}


void insn_famomaxg_ps(Hart& cpu)
{
    require_fp_active();
    DISASM_AMO_FD_FS1_RS2("famomaxg.ps");
    GSCAMO(mmu_global_atomic32(cpu, RS2 + FS1.i32[e], FD.u32[e], f32_maximum()));
}


void insn_famomaxl_pi(Hart& cpu)
{
    require_fp_active();
    DISASM_AMO_FD_FS1_RS2("famomaxl.pi");
    GSCAMO(mmu_local_atomic32(cpu, RS2 + FS1.i32[e], FD.u32[e], maximum<int32_t>()));
}


void insn_famomaxl_ps(Hart& cpu)
{
    require_fp_active();
    DISASM_AMO_FD_FS1_RS2("famomaxl.ps");
    GSCAMO(mmu_local_atomic32(cpu, RS2 + FS1.i32[e], FD.u32[e], f32_maximum()));
}


void insn_famomaxug_pi(Hart& cpu)
{
    require_fp_active();
    DISASM_AMO_FD_FS1_RS2("famomaxug.pi");
    GSCAMO(mmu_global_atomic32(cpu, RS2 + FS1.i32[e], FD.u32[e], maximum<uint32_t>()));
}


void insn_famomaxul_pi(Hart& cpu)
{
    require_fp_active();
    DISASM_AMO_FD_FS1_RS2("famomaxul.pi");
    GSCAMO(mmu_local_atomic32(cpu, RS2 + FS1.i32[e], FD.u32[e], maximum<uint32_t>()));
}


void insn_famoming_pi(Hart& cpu)
{
    require_fp_active();
    DISASM_AMO_FD_FS1_RS2("famoming.pi");
    GSCAMO(mmu_global_atomic32(cpu, RS2 + FS1.i32[e], FD.u32[e], minimum<int32_t>()));
}


void insn_famoming_ps(Hart& cpu)
{
    require_fp_active();
    DISASM_AMO_FD_FS1_RS2("famoming.ps");
    GSCAMO(mmu_global_atomic32(cpu, RS2 + FS1.i32[e], FD.u32[e], f32_minimum()));
}


void insn_famominl_pi(Hart& cpu)
{
    require_fp_active();
    DISASM_AMO_FD_FS1_RS2("famominl.pi");
    GSCAMO(mmu_local_atomic32(cpu, RS2 + FS1.i32[e], FD.u32[e], minimum<int32_t>()));
}


void insn_famominl_ps(Hart& cpu)
{
    require_fp_active();
    DISASM_AMO_FD_FS1_RS2("famominl.ps");
    GSCAMO(mmu_local_atomic32(cpu, RS2 + FS1.i32[e], FD.u32[e], f32_minimum()));
}


void insn_famominug_pi(Hart& cpu)
{
    require_fp_active();
    DISASM_AMO_FD_FS1_RS2("famominug.pi");
    GSCAMO(mmu_global_atomic32(cpu, RS2 + FS1.i32[e], FD.u32[e], minimum<uint32_t>()));
}


void insn_famominul_pi(Hart& cpu)
{
    require_fp_active();
    DISASM_AMO_FD_FS1_RS2("famominul.pi");
    GSCAMO(mmu_local_atomic32(cpu, RS2 + FS1.i32[e], FD.u32[e], minimum<uint32_t>()));
}


void insn_famoorg_pi(Hart& cpu)
{
    require_fp_active();
    DISASM_AMO_FD_FS1_RS2("famoorg.pi");
    GSCAMO(mmu_global_atomic32(cpu, RS2 + FS1.i32[e], FD.u32[e], std::bit_or<uint32_t>()));
}


void insn_famoorl_pi(Hart& cpu)
{
    require_fp_active();
    DISASM_AMO_FD_FS1_RS2("famoorl.pi");
    GSCAMO(mmu_local_atomic32(cpu, RS2 + FS1.i32[e], FD.u32[e], std::bit_or<uint32_t>()));
}


void insn_famoswapg_pi(Hart& cpu)
{
    require_fp_active();
    DISASM_AMO_FD_FS1_RS2("famoswapg.pi");
    GSCAMO(mmu_global_atomic32(cpu, RS2 + FS1.i32[e], FD.u32[e], replace<uint32_t>()));
}


void insn_famoswapl_pi(Hart& cpu)
{
    require_fp_active();
    DISASM_AMO_FD_FS1_RS2("famoswapl.pi");
    GSCAMO(mmu_local_atomic32(cpu, RS2 + FS1.i32[e], FD.u32[e], replace<uint32_t>()));
}


void insn_famoxorg_pi(Hart& cpu)
{
    require_fp_active();
    DISASM_AMO_FD_FS1_RS2("famoxorg.pi");
    GSCAMO(mmu_global_atomic32(cpu, RS2 + FS1.i32[e], FD.u32[e], std::bit_xor<uint32_t>()));
}


void insn_famoxorl_pi(Hart& cpu)
{
    require_fp_active();
    DISASM_AMO_FD_FS1_RS2("famoxorl.pi");
    GSCAMO(mmu_local_atomic32(cpu, RS2 + FS1.i32[e], FD.u32[e], std::bit_xor<uint32_t>()));
}


} // namespace bemu
