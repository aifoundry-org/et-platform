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

//namespace bemu {


void insn_fence(insn_t inst __attribute__((unused)))
{
    DISASM_NOARG("fence");
}


void insn_lb(insn_t inst)
{
    DISASM_LOAD_RD_RS1_IIMM("lb");
    uint64_t tmp = sext<8>(mmu_load8(RS1 + IIMM));
    LOAD_WRITE_RD(tmp);
}


void insn_lbu(insn_t inst)
{
    DISASM_LOAD_RD_RS1_IIMM("lbu");
    uint64_t tmp = mmu_load8(RS1 + IIMM);
    LOAD_WRITE_RD(tmp);
}


void insn_ld(insn_t inst)
{
    DISASM_LOAD_RD_RS1_IIMM("ld");
    uint64_t tmp = mmu_load64(RS1 + IIMM);
    LOAD_WRITE_RD(tmp);
}


void insn_lh(insn_t inst)
{
    DISASM_LOAD_RD_RS1_IIMM("lh");
    uint64_t tmp = sext<16>(mmu_load16(RS1 + IIMM));
    LOAD_WRITE_RD(tmp);
}


void insn_lhu(insn_t inst)
{
    DISASM_LOAD_RD_RS1_IIMM("lhu");
    uint64_t tmp = mmu_load16(RS1 + IIMM);
    LOAD_WRITE_RD(tmp);
}


void insn_lw(insn_t inst)
{
    DISASM_LOAD_RD_RS1_IIMM("lw");
    uint64_t tmp = sext<32>(mmu_load32(RS1 + IIMM));
    LOAD_WRITE_RD(tmp);
}


void insn_lwu(insn_t inst)
{
    DISASM_LOAD_RD_RS1_IIMM("lw");
    uint64_t tmp = mmu_load32(RS1 + IIMM);
    LOAD_WRITE_RD(tmp);
}


void insn_sb(insn_t inst)
{
    DISASM_STORE_RS2_RS1_SIMM("sb");
    mmu_store8(RS1 + SIMM, uint8_t(RS2));
}


void insn_sd(insn_t inst)
{
    DISASM_STORE_RS2_RS1_SIMM("sd");
    mmu_store64(RS1 + SIMM, RS2);
}


void insn_sh(insn_t inst)
{
    DISASM_STORE_RS2_RS1_SIMM("sh");
    mmu_store16(RS1 + SIMM, uint16_t(RS2));
}


void insn_sw(insn_t inst)
{
    DISASM_STORE_RS2_RS1_SIMM("sw");
    mmu_store32(RS1 + SIMM, uint32_t(RS2));
}


//} // namespace bemu
