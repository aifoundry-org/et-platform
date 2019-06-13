/* vim: set ts=8 sw=4 et sta cin cino=\:0s,l1,g0,N-s,E-s,i0,+2s,(0,W2s : */

#include "decode.h"
#include "emu_gio.h"
#include "insn.h"
#include "insn_func.h"
#include "log.h"
#include "mmu.h"
#include "processor.h"
#include "utility.h"

// FIXME: Replace with "processor.h"
#include "emu_defines.h"
extern std::array<Processor,EMU_NUM_THREADS> cpu;

// namespace bemu {


void insn_c_ld(insn_t inst)
{
    C_DISASM_LOAD_RS2P_RS1P_IMMLSD("c.ld");
    uint64_t tmp = mmu_load64(C_RS1P + C_IMMLSD);
    WRITE_C_RS2P(tmp);
}


void insn_c_ldsp(insn_t inst)
{
    C_DISASM_LOAD_LDSP("c.ldsp");
    uint64_t tmp = mmu_load64(X2 + C_IMMLDSP);
    WRITE_C_RS1(tmp);
}


void insn_c_lw(insn_t inst)
{
    C_DISASM_LOAD_RS2P_RS1P_IMMLSW("c.lw");
    uint64_t tmp = sext<32>(mmu_load32(C_RS1P + C_IMMLSW));
    WRITE_C_RS2P(tmp);
}


void insn_c_lwsp(insn_t inst)
{
    C_DISASM_LOAD_LWSP("c.lwsp");
    uint64_t tmp = sext<32>(mmu_load32(X2 + C_IMMLWSP));
    WRITE_C_RS1(tmp);
}


void insn_c_sd(insn_t inst)
{
    C_DISASM_STORE_RS2P_RS1P_IMMLSD("c.sd");
    mmu_store64(C_RS1P + C_IMMLSD, C_RS2P);
}


void insn_c_sdsp(insn_t inst)
{
    C_DISASM_STORE_SWSP("c.sdsp");
    mmu_store64(X2 + C_IMMSDSP, C_RS2);
}


void insn_c_sw(insn_t inst)
{
    C_DISASM_STORE_RS2P_RS1P_IMMLSW("c.sd");
    mmu_store32(C_RS1P + C_IMMLSW, uint32_t(C_RS2P));
}


void insn_c_swsp(insn_t inst)
{
    C_DISASM_STORE_SWSP("c.swsp");
    mmu_store32(X2 + C_IMMSWSP, uint32_t(C_RS2));
}


//} // namespace bemu
