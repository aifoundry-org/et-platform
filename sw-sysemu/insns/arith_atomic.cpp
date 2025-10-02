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
#include "insn.h"
#include "insn_func.h"
#include "insn_util.h"
#include "log.h"
#include "mmu.h"
#include "processor.h"
#include "utility.h"

namespace bemu {


void insn_amoaddg_d(Hart& cpu)
{
    DISASM_AMO_RD_RS1_RS2("amoaddg.d");
    uint64_t tmp = mmu_global_atomic64(cpu, RS1, RS2, std::plus<uint64_t>());
    LOAD_WRITE_RD(tmp);
}


void insn_amoaddg_w(Hart& cpu)
{
    DISASM_AMO_RD_RS1_RS2("amoaddg.w");
    uint32_t tmp = mmu_global_atomic32(cpu, RS1, uint32_t(RS2), std::plus<uint32_t>());
    LOAD_WRITE_RD(sext<32>(tmp));
}


void insn_amoaddl_d(Hart& cpu)
{
    DISASM_AMO_RD_RS1_RS2("amoaddl.d");
    uint64_t tmp = mmu_local_atomic64(cpu, RS1, RS2, std::plus<uint64_t>());
    LOAD_WRITE_RD(tmp);
}


void insn_amoaddl_w(Hart& cpu)
{
    DISASM_AMO_RD_RS1_RS2("amoaddl.w");
    uint32_t tmp = mmu_local_atomic32(cpu, RS1, uint32_t(RS2), std::plus<uint32_t>());
    LOAD_WRITE_RD(sext<32>(tmp));
}


void insn_amoandg_d(Hart& cpu)
{
    DISASM_AMO_RD_RS1_RS2("amoandg.d");
    uint64_t tmp = mmu_global_atomic64(cpu, RS1, RS2, std::bit_and<uint64_t>());
    LOAD_WRITE_RD(tmp);
}


void insn_amoandg_w(Hart& cpu)
{
    DISASM_AMO_RD_RS1_RS2("amoandg.w");
    uint32_t tmp = mmu_global_atomic32(cpu, RS1, uint32_t(RS2), std::bit_and<uint32_t>());
    LOAD_WRITE_RD(sext<32>(tmp));
}


void insn_amoandl_d(Hart& cpu)
{
    DISASM_AMO_RD_RS1_RS2("amoandl.d");
    uint64_t tmp = mmu_local_atomic64(cpu, RS1, RS2, std::bit_and<uint64_t>());
    LOAD_WRITE_RD(tmp);
}


void insn_amoandl_w(Hart& cpu)
{
    DISASM_AMO_RD_RS1_RS2("amoandl.w");
    uint32_t tmp = mmu_local_atomic32(cpu, RS1, uint32_t(RS2), std::bit_and<uint32_t>());
    LOAD_WRITE_RD(sext<32>(tmp));
}


void insn_amocmpswapg_d (Hart& cpu)
{
    DISASM_AMO_RD_RS1_RS2("amocmpswapg.d");
    LOG_REG(":", 31);
    uint64_t tmp = mmu_global_compare_exchange64(cpu, RS1, X31, RS2);
    LOAD_WRITE_RD(tmp);
}


void insn_amocmpswapg_w (Hart& cpu)
{
    DISASM_AMO_RD_RS1_RS2("amocmpswapg.w");
    LOG_REG(":", 31);
    uint32_t tmp = mmu_global_compare_exchange32(cpu, RS1, uint32_t(X31), uint32_t(RS2));
    LOAD_WRITE_RD(sext<32>(tmp));
}


void insn_amocmpswapl_d (Hart& cpu)
{
    DISASM_AMO_RD_RS1_RS2("amocmpswapl.d");
    LOG_REG(":", 31);
    uint64_t tmp = mmu_local_compare_exchange64(cpu, RS1, X31, RS2);
    LOAD_WRITE_RD(tmp);
}


void insn_amocmpswapl_w (Hart& cpu)
{
    DISASM_AMO_RD_RS1_RS2("amocmpswapl.w");
    LOG_REG(":", 31);
    uint32_t tmp = mmu_local_compare_exchange32(cpu, RS1, uint32_t(X31), uint32_t(RS2));
    LOAD_WRITE_RD(sext<32>(tmp));
}


void insn_amomaxg_d(Hart& cpu)
{
    DISASM_AMO_RD_RS1_RS2("amomaxg.d");
    uint64_t tmp = mmu_global_atomic64(cpu, RS1, RS2, maximum<int64_t>());
    LOAD_WRITE_RD(tmp);
}


void insn_amomaxg_w(Hart& cpu)
{
    DISASM_AMO_RD_RS1_RS2("amomaxg.w");
    uint32_t tmp = mmu_global_atomic32(cpu, RS1, uint32_t(RS2), maximum<int32_t>());
    LOAD_WRITE_RD(sext<32>(tmp));
}


void insn_amomaxl_d(Hart& cpu)
{
    DISASM_AMO_RD_RS1_RS2("amomaxl.d");
    uint64_t tmp = mmu_local_atomic64(cpu, RS1, RS2, maximum<int64_t>());
    LOAD_WRITE_RD(tmp);
}


void insn_amomaxl_w(Hart& cpu)
{
    DISASM_AMO_RD_RS1_RS2("amomaxl.w");
    uint32_t tmp = mmu_local_atomic32(cpu, RS1, uint32_t(RS2), maximum<int32_t>());
    LOAD_WRITE_RD(sext<32>(tmp));
}


void insn_amomaxug_d(Hart& cpu)
{
    DISASM_AMO_RD_RS1_RS2("amomaxug.d");
    uint64_t tmp = mmu_global_atomic64(cpu, RS1, RS2, maximum<uint64_t>());
    LOAD_WRITE_RD(tmp);
}


void insn_amomaxug_w(Hart& cpu)
{
    DISASM_AMO_RD_RS1_RS2("amomaxug.w");
    uint32_t tmp = mmu_global_atomic32(cpu, RS1, uint32_t(RS2), maximum<uint32_t>());
    LOAD_WRITE_RD(sext<32>(tmp));
}


void insn_amomaxul_d(Hart& cpu)
{
    DISASM_AMO_RD_RS1_RS2("amomaxul.d");
    uint64_t tmp = mmu_local_atomic64(cpu, RS1, RS2, maximum<uint64_t>());
    LOAD_WRITE_RD(tmp);
}


void insn_amomaxul_w(Hart& cpu)
{
    DISASM_AMO_RD_RS1_RS2("amomaxul.w");
    uint32_t tmp = mmu_local_atomic32(cpu, RS1, uint32_t(RS2), maximum<uint32_t>());
    LOAD_WRITE_RD(sext<32>(tmp));
}


void insn_amoming_d(Hart& cpu)
{
    DISASM_AMO_RD_RS1_RS2("amoming.d");
    uint64_t tmp = mmu_global_atomic64(cpu, RS1, RS2, minimum<int64_t>());
    LOAD_WRITE_RD(tmp);
}


void insn_amoming_w(Hart& cpu)
{
    DISASM_AMO_RD_RS1_RS2("amoming.w");
    uint32_t tmp = mmu_global_atomic32(cpu, RS1, uint32_t(RS2), minimum<int32_t>());
    LOAD_WRITE_RD(sext<32>(tmp));
}


void insn_amominl_d(Hart& cpu)
{
    DISASM_AMO_RD_RS1_RS2("amominl.d");
    uint64_t tmp = mmu_local_atomic64(cpu, RS1, RS2, minimum<int64_t>());
    LOAD_WRITE_RD(tmp);
}


void insn_amominl_w(Hart& cpu)
{
    DISASM_AMO_RD_RS1_RS2("amominl.w");
    uint32_t tmp = mmu_local_atomic32(cpu, RS1, uint32_t(RS2), minimum<int32_t>());
    LOAD_WRITE_RD(sext<32>(tmp));
}


void insn_amominug_d(Hart& cpu)
{
    DISASM_AMO_RD_RS1_RS2("amominug.d");
    uint64_t tmp = mmu_global_atomic64(cpu, RS1, RS2, minimum<uint64_t>());
    LOAD_WRITE_RD(tmp);
}


void insn_amominug_w(Hart& cpu)
{
    DISASM_AMO_RD_RS1_RS2("amominug.w");
    uint32_t tmp = mmu_global_atomic32(cpu, RS1, uint32_t(RS2), minimum<uint32_t>());
    LOAD_WRITE_RD(sext<32>(tmp));
}


void insn_amominul_d(Hart& cpu)
{
    DISASM_AMO_RD_RS1_RS2("amominul.d");
    uint64_t tmp = mmu_local_atomic64(cpu, RS1, RS2, minimum<uint64_t>());
    LOAD_WRITE_RD(tmp);
}


void insn_amominul_w(Hart& cpu)
{
    DISASM_AMO_RD_RS1_RS2("amominul.w");
    uint32_t tmp = mmu_local_atomic32(cpu, RS1, uint32_t(RS2), minimum<uint32_t>());
    LOAD_WRITE_RD(sext<32>(tmp));
}


void insn_amoorg_d(Hart& cpu)
{
    DISASM_AMO_RD_RS1_RS2("amoorg.d");
    uint64_t tmp = mmu_global_atomic64(cpu, RS1, RS2, std::bit_or<uint64_t>());
    LOAD_WRITE_RD(tmp);
}


void insn_amoorg_w(Hart& cpu)
{
    DISASM_AMO_RD_RS1_RS2("amoorg.w");
    uint32_t tmp = mmu_global_atomic32(cpu, RS1, uint32_t(RS2), std::bit_or<uint32_t>());
    LOAD_WRITE_RD(sext<32>(tmp));
}


void insn_amoorl_d(Hart& cpu)
{
    DISASM_AMO_RD_RS1_RS2("amoorl.d");
    uint64_t tmp = mmu_local_atomic64(cpu, RS1, RS2, std::bit_or<uint64_t>());
    LOAD_WRITE_RD(tmp);
}


void insn_amoorl_w(Hart& cpu)
{
    DISASM_AMO_RD_RS1_RS2("amoorl.w");
    uint32_t tmp = mmu_local_atomic32(cpu, RS1, uint32_t(RS2), std::bit_or<uint32_t>());
    LOAD_WRITE_RD(sext<32>(tmp));
}


void insn_amoswapg_d(Hart& cpu)
{
    DISASM_AMO_RD_RS1_RS2("amoswapg.d");
    uint64_t tmp = mmu_global_atomic64(cpu, RS1, RS2, replace<uint64_t>());
    LOAD_WRITE_RD(tmp);
}


void insn_amoswapg_w(Hart& cpu)
{
    DISASM_AMO_RD_RS1_RS2("amoswapg.w");
    uint32_t tmp = mmu_global_atomic32(cpu, RS1, uint32_t(RS2), replace<uint32_t>());
    LOAD_WRITE_RD(sext<32>(tmp));
}


void insn_amoswapl_d(Hart& cpu)
{
    DISASM_AMO_RD_RS1_RS2("amoswapl.d");
    uint64_t tmp = mmu_local_atomic64(cpu, RS1, RS2, replace<uint64_t>());
    LOAD_WRITE_RD(tmp);
}


void insn_amoswapl_w(Hart& cpu)
{
    DISASM_AMO_RD_RS1_RS2("amoswapl.w");
    uint32_t tmp = mmu_local_atomic32(cpu, RS1, uint32_t(RS2), replace<uint32_t>());
    LOAD_WRITE_RD(sext<32>(tmp));
}


void insn_amoxorg_d(Hart& cpu)
{
    DISASM_AMO_RD_RS1_RS2("amoxorg.d");
    uint64_t tmp = mmu_global_atomic64(cpu, RS1, RS2, std::bit_xor<uint64_t>());
    LOAD_WRITE_RD(tmp);
}


void insn_amoxorg_w(Hart& cpu)
{
    DISASM_AMO_RD_RS1_RS2("amoxorg.w");
    uint32_t tmp = mmu_global_atomic32(cpu, RS1, uint32_t(RS2), std::bit_xor<uint32_t>());
    LOAD_WRITE_RD(sext<32>(tmp));
}


void insn_amoxorl_d(Hart& cpu)
{
    DISASM_AMO_RD_RS1_RS2("amoxorl.d");
    uint64_t tmp = mmu_local_atomic64(cpu, RS1, RS2, std::bit_xor<uint64_t>());
    LOAD_WRITE_RD(tmp);
}


void insn_amoxorl_w(Hart& cpu)
{
    DISASM_AMO_RD_RS1_RS2("amoxorl.w");
    uint32_t tmp = mmu_local_atomic32(cpu, RS1, uint32_t(RS2), std::bit_xor<uint32_t>());
    LOAD_WRITE_RD(sext<32>(tmp));
}


} // namespace bemu
