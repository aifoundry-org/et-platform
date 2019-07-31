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

// FIXME: Replace with "processor.h"
#include "emu_defines.h"
extern std::array<Processor,EMU_NUM_THREADS> cpu;

//namespace bemu {


void insn_amoaddg_d(insn_t inst)
{
    DISASM_AMO_RD_RS1_RS2("amoaddg.d");
    uint64_t tmp = mmu_global_atomic64(RS1, RS2, std::plus<uint64_t>());
    LOAD_WRITE_RD(tmp);
}


void insn_amoaddg_w(insn_t inst)
{
    DISASM_AMO_RD_RS1_RS2("amoaddg.w");
    uint32_t tmp = mmu_global_atomic32(RS1, uint32_t(RS2), std::plus<uint32_t>());
    LOAD_WRITE_RD(sext<32>(tmp));
}


void insn_amoaddl_d(insn_t inst)
{
    DISASM_AMO_RD_RS1_RS2("amoaddl.d");
    uint64_t tmp = mmu_local_atomic64(RS1, RS2, std::plus<uint64_t>());
    LOAD_WRITE_RD(tmp);
}


void insn_amoaddl_w(insn_t inst)
{
    DISASM_AMO_RD_RS1_RS2("amoaddl.w");
    uint32_t tmp = mmu_local_atomic32(RS1, uint32_t(RS2), std::plus<uint32_t>());
    LOAD_WRITE_RD(sext<32>(tmp));
}


void insn_amoandg_d(insn_t inst)
{
    DISASM_AMO_RD_RS1_RS2("amoandg.d");
    uint64_t tmp = mmu_global_atomic64(RS1, RS2, std::bit_and<uint64_t>());
    LOAD_WRITE_RD(tmp);
}


void insn_amoandg_w(insn_t inst)
{
    DISASM_AMO_RD_RS1_RS2("amoandg.w");
    uint32_t tmp = mmu_global_atomic32(RS1, uint32_t(RS2), std::bit_and<uint32_t>());
    LOAD_WRITE_RD(sext<32>(tmp));
}


void insn_amoandl_d(insn_t inst)
{
    DISASM_AMO_RD_RS1_RS2("amoandl.d");
    uint64_t tmp = mmu_local_atomic64(RS1, RS2, std::bit_and<uint64_t>());
    LOAD_WRITE_RD(tmp);
}


void insn_amoandl_w(insn_t inst)
{
    DISASM_AMO_RD_RS1_RS2("amoandl.w");
    uint32_t tmp = mmu_local_atomic32(RS1, uint32_t(RS2), std::bit_and<uint32_t>());
    LOAD_WRITE_RD(sext<32>(tmp));
}


void insn_amocmpswapg_d (insn_t inst)
{
    DISASM_AMO_RD_RS1_RS2("amocmpswapg.d");
    LOG_REG(":", 31);
    uint64_t tmp = mmu_global_compare_exchange64(RS1, X31, RS2);
    LOAD_WRITE_RD(tmp);
}


void insn_amocmpswapg_w (insn_t inst)
{
    DISASM_AMO_RD_RS1_RS2("amocmpswapg.w");
    LOG_REG(":", 31);
    uint32_t tmp = mmu_global_compare_exchange32(RS1, uint32_t(X31), uint32_t(RS2));
    LOAD_WRITE_RD(sext<32>(tmp));
}


void insn_amocmpswapl_d (insn_t inst)
{
    DISASM_AMO_RD_RS1_RS2("amocmpswapl.d");
    LOG_REG(":", 31);
    uint64_t tmp = mmu_local_compare_exchange64(RS1, X31, RS2);
    LOAD_WRITE_RD(tmp);
}


void insn_amocmpswapl_w (insn_t inst)
{
    DISASM_AMO_RD_RS1_RS2("amocmpswapl.w");
    LOG_REG(":", 31);
    uint32_t tmp = mmu_local_compare_exchange32(RS1, uint32_t(X31), uint32_t(RS2));
    LOAD_WRITE_RD(sext<32>(tmp));
}


void insn_amomaxg_d(insn_t inst)
{
    DISASM_AMO_RD_RS1_RS2("amomaxg.d");
    uint64_t tmp = mmu_global_atomic64(RS1, RS2, maximum<int64_t>());
    LOAD_WRITE_RD(tmp);
}


void insn_amomaxg_w(insn_t inst)
{
    DISASM_AMO_RD_RS1_RS2("amomaxg.w");
    uint32_t tmp = mmu_global_atomic32(RS1, uint32_t(RS2), maximum<int32_t>());
    LOAD_WRITE_RD(sext<32>(tmp));
}


void insn_amomaxl_d(insn_t inst)
{
    DISASM_AMO_RD_RS1_RS2("amomaxl.d");
    uint64_t tmp = mmu_local_atomic64(RS1, RS2, maximum<int64_t>());
    LOAD_WRITE_RD(tmp);
}


void insn_amomaxl_w(insn_t inst)
{
    DISASM_AMO_RD_RS1_RS2("amomaxl.w");
    uint32_t tmp = mmu_local_atomic32(RS1, uint32_t(RS2), maximum<int32_t>());
    LOAD_WRITE_RD(sext<32>(tmp));
}


void insn_amomaxug_d(insn_t inst)
{
    DISASM_AMO_RD_RS1_RS2("amomaxug.d");
    uint64_t tmp = mmu_global_atomic64(RS1, RS2, maximum<uint64_t>());
    LOAD_WRITE_RD(tmp);
}


void insn_amomaxug_w(insn_t inst)
{
    DISASM_AMO_RD_RS1_RS2("amomaxug.w");
    uint32_t tmp = mmu_global_atomic32(RS1, uint32_t(RS2), maximum<uint32_t>());
    LOAD_WRITE_RD(sext<32>(tmp));
}


void insn_amomaxul_d(insn_t inst)
{
    DISASM_AMO_RD_RS1_RS2("amomaxul.d");
    uint64_t tmp = mmu_local_atomic64(RS1, RS2, maximum<uint64_t>());
    LOAD_WRITE_RD(tmp);
}


void insn_amomaxul_w(insn_t inst)
{
    DISASM_AMO_RD_RS1_RS2("amomaxul.w");
    uint32_t tmp = mmu_local_atomic32(RS1, uint32_t(RS2), maximum<uint32_t>());
    LOAD_WRITE_RD(sext<32>(tmp));
}


void insn_amoming_d(insn_t inst)
{
    DISASM_AMO_RD_RS1_RS2("amoming.d");
    uint64_t tmp = mmu_global_atomic64(RS1, RS2, minimum<int64_t>());
    LOAD_WRITE_RD(tmp);
}


void insn_amoming_w(insn_t inst)
{
    DISASM_AMO_RD_RS1_RS2("amoming.w");
    uint32_t tmp = mmu_global_atomic32(RS1, uint32_t(RS2), minimum<int32_t>());
    LOAD_WRITE_RD(sext<32>(tmp));
}


void insn_amominl_d(insn_t inst)
{
    DISASM_AMO_RD_RS1_RS2("amominl.d");
    uint64_t tmp = mmu_local_atomic64(RS1, RS2, minimum<int64_t>());
    LOAD_WRITE_RD(tmp);
}


void insn_amominl_w(insn_t inst)
{
    DISASM_AMO_RD_RS1_RS2("amominl.w");
    uint32_t tmp = mmu_local_atomic32(RS1, uint32_t(RS2), minimum<int32_t>());
    LOAD_WRITE_RD(sext<32>(tmp));
}


void insn_amominug_d(insn_t inst)
{
    DISASM_AMO_RD_RS1_RS2("amominug.d");
    uint64_t tmp = mmu_global_atomic64(RS1, RS2, minimum<uint64_t>());
    LOAD_WRITE_RD(tmp);
}


void insn_amominug_w(insn_t inst)
{
    DISASM_AMO_RD_RS1_RS2("amominug.w");
    uint32_t tmp = mmu_global_atomic32(RS1, uint32_t(RS2), minimum<uint32_t>());
    LOAD_WRITE_RD(sext<32>(tmp));
}


void insn_amominul_d(insn_t inst)
{
    DISASM_AMO_RD_RS1_RS2("amominul.d");
    uint64_t tmp = mmu_local_atomic64(RS1, RS2, minimum<uint64_t>());
    LOAD_WRITE_RD(tmp);
}


void insn_amominul_w(insn_t inst)
{
    DISASM_AMO_RD_RS1_RS2("amominul.w");
    uint32_t tmp = mmu_local_atomic32(RS1, uint32_t(RS2), minimum<uint32_t>());
    LOAD_WRITE_RD(sext<32>(tmp));
}


void insn_amoorg_d(insn_t inst)
{
    DISASM_AMO_RD_RS1_RS2("amoorg.d");
    uint64_t tmp = mmu_global_atomic64(RS1, RS2, std::bit_or<uint64_t>());
    LOAD_WRITE_RD(tmp);
}


void insn_amoorg_w(insn_t inst)
{
    DISASM_AMO_RD_RS1_RS2("amoorg.w");
    uint32_t tmp = mmu_global_atomic32(RS1, uint32_t(RS2), std::bit_or<uint32_t>());
    LOAD_WRITE_RD(sext<32>(tmp));
}


void insn_amoorl_d(insn_t inst)
{
    DISASM_AMO_RD_RS1_RS2("amoorl.d");
    uint64_t tmp = mmu_local_atomic64(RS1, RS2, std::bit_or<uint64_t>());
    LOAD_WRITE_RD(tmp);
}


void insn_amoorl_w(insn_t inst)
{
    DISASM_AMO_RD_RS1_RS2("amoorl.w");
    uint32_t tmp = mmu_local_atomic32(RS1, uint32_t(RS2), std::bit_or<uint32_t>());
    LOAD_WRITE_RD(sext<32>(tmp));
}


void insn_amoswapg_d(insn_t inst)
{
    DISASM_AMO_RD_RS1_RS2("amoswapg.d");
    uint64_t tmp = mmu_global_atomic64(RS1, RS2, replace<uint64_t>());
    LOAD_WRITE_RD(tmp);
}


void insn_amoswapg_w(insn_t inst)
{
    DISASM_AMO_RD_RS1_RS2("amoswapg.w");
    uint32_t tmp = mmu_global_atomic32(RS1, uint32_t(RS2), replace<uint32_t>());
    LOAD_WRITE_RD(sext<32>(tmp));
}


void insn_amoswapl_d(insn_t inst)
{
    DISASM_AMO_RD_RS1_RS2("amoswapl.d");
    uint64_t tmp = mmu_local_atomic64(RS1, RS2, replace<uint64_t>());
    LOAD_WRITE_RD(tmp);
}


void insn_amoswapl_w(insn_t inst)
{
    DISASM_AMO_RD_RS1_RS2("amoswapl.w");
    uint32_t tmp = mmu_local_atomic32(RS1, uint32_t(RS2), replace<uint32_t>());
    LOAD_WRITE_RD(sext<32>(tmp));
}


void insn_amoxorg_d(insn_t inst)
{
    DISASM_AMO_RD_RS1_RS2("amoxorg.d");
    uint64_t tmp = mmu_global_atomic64(RS1, RS2, std::bit_xor<uint64_t>());
    LOAD_WRITE_RD(tmp);
}


void insn_amoxorg_w(insn_t inst)
{
    DISASM_AMO_RD_RS1_RS2("amoxorg.w");
    uint32_t tmp = mmu_global_atomic32(RS1, uint32_t(RS2), std::bit_xor<uint32_t>());
    LOAD_WRITE_RD(sext<32>(tmp));
}


void insn_amoxorl_d(insn_t inst)
{
    DISASM_AMO_RD_RS1_RS2("amoxorl.d");
    uint64_t tmp = mmu_local_atomic64(RS1, RS2, std::bit_xor<uint64_t>());
    LOAD_WRITE_RD(tmp);
}


void insn_amoxorl_w(insn_t inst)
{
    DISASM_AMO_RD_RS1_RS2("amoxorl.w");
    uint32_t tmp = mmu_local_atomic32(RS1, uint32_t(RS2), std::bit_xor<uint32_t>());
    LOAD_WRITE_RD(sext<32>(tmp));
}


//} // namespace bemu
