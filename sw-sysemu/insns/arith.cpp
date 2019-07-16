/* vim: set ts=8 sw=4 et sta cin cino=\:0s,l1,g0,N-s,E-s,i0,+2s,(0,W2s : */

#include "decode.h"
#include "emu_gio.h"
#include "insn.h"
#include "insn_func.h"
#include "log.h"
#include "processor.h"
#include "utility.h"

// FIXME: Replace with "processor.h"
#include "emu_defines.h"
extern std::array<Processor,EMU_NUM_THREADS> cpu;
extern uint64_t current_pc;

//namespace bemu {


void insn_reserved(insn_t inst)
{
    DISASM_NOARG("illegal opcode");
    throw trap_illegal_instruction(inst.bits);
}


void insn_add(insn_t inst)
{
    DISASM_RD_RS1_RS2("add");
    WRITE_RD(RS1 + RS2);
}


void insn_addi(insn_t inst)
{
    DISASM_RD_RS1_IIMM("addi");
    WRITE_RD(RS1 + IIMM);
}


void insn_addiw(insn_t inst)
{
    DISASM_RD_RS1_IIMM("addiw");
    WRITE_RD(sext<32>(RS1 + IIMM));
}


void insn_addw(insn_t inst)
{
    DISASM_RD_RS1_RS2("addw");
    WRITE_RD(sext<32>(RS1 + RS2));
}


void insn_and(insn_t inst)
{
    DISASM_RD_RS1_RS2("and");
    WRITE_RD(RS1 & RS2);
}


void insn_andi(insn_t inst)
{
    DISASM_RD_RS1_IIMM("andi");
    WRITE_RD(RS1 & IIMM);
}


void insn_auipc(insn_t inst)
{
    DISASM_RD_UIMM("auipc");
    LOG_PC(":");
    WRITE_RD(PC + UIMM);
}


void insn_lui(insn_t inst)
{
    DISASM_RD_UIMM("lui");
    WRITE_RD(UIMM);
}


void insn_or(insn_t inst)
{
    DISASM_RD_RS1_RS2("or");
    WRITE_RD(RS1 | RS2);
}


void insn_ori(insn_t inst)
{
    DISASM_RD_RS1_IIMM("ori");
    WRITE_RD(RS1 | IIMM);
}


void insn_sll(insn_t inst)
{
    DISASM_RD_RS1_RS2("sll");
    WRITE_RD(RS1 << (RS2 % 64));
}


void insn_slli(insn_t inst)
{
    DISASM_RD_RS1_SHAMT6("slli");
    WRITE_RD(RS1 << SHAMT6);
}


void insn_slliw(insn_t inst)
{
    DISASM_RD_RS1_SHAMT5("slliw");
    WRITE_RD(sext<32>(RS1 << SHAMT5));
}

void insn_sllw(insn_t inst)
{
    DISASM_RD_RS1_RS2("sllw");
    WRITE_RD(sext<32>(RS1 << (RS2 % 32)));
}


void insn_slt(insn_t inst)
{
    DISASM_RD_RS1_RS2("slt");
    WRITE_RD(int64_t(RS1) < int64_t(RS2));
}


void insn_slti(insn_t inst)
{
    DISASM_RD_RS1_IIMM("slti");
    WRITE_RD(int64_t(RS1) < IIMM);
}


void insn_sltiu(insn_t inst)
{
    DISASM_RD_RS1_IIMM("sltiu");
    WRITE_RD(RS1 < uint64_t(IIMM));
}

void insn_sltu(insn_t inst)
{
    DISASM_RD_RS1_RS2("sltu");
    WRITE_RD(RS1 < RS2);
}


void insn_sra(insn_t inst)
{
    DISASM_RD_RS1_RS2("sra");
    WRITE_RD(int64_t(RS1) >> (RS2 % 64));
}


void insn_srai(insn_t inst)
{
    DISASM_RD_RS1_SHAMT6("srai");
    WRITE_RD(int64_t(RS1) >> SHAMT6);
}


void insn_sraiw(insn_t inst)
{
    DISASM_RD_RS1_SHAMT5("sraiw");
    WRITE_RD(sext<32>(int32_t(RS1) >> SHAMT5));
}


void insn_sraw(insn_t inst)
{
    DISASM_RD_RS1_RS2("sraw");
    WRITE_RD(sext<32>(int32_t(RS1) >> (RS2 % 32)));
}


void insn_srl(insn_t inst)
{
    DISASM_RD_RS1_RS2("srl");
    WRITE_RD(RS1 >> (RS2 % 64));
}


void insn_srli(insn_t inst)
{
    DISASM_RD_RS1_SHAMT6("srli");
    WRITE_RD(RS1 >> SHAMT6);
}


void insn_srliw(insn_t inst)
{
    DISASM_RD_RS1_SHAMT5("srliw");
    WRITE_RD(sext<32>(uint32_t(RS1) >> SHAMT5));
}


void insn_srlw(insn_t inst)
{
    DISASM_RD_RS1_RS2("srlw");
    WRITE_RD(sext<32>(uint32_t(RS1) >> (RS2 % 32)));
}


void insn_sub(insn_t inst)
{
    DISASM_RD_RS1_RS2("sub");
    WRITE_RD(RS1 - RS2);
}


void insn_subw(insn_t inst)
{
    DISASM_RD_RS1_RS2("subw");
    WRITE_RD(sext<32>(RS1 - RS2));
}


void insn_xor(insn_t inst)
{
    DISASM_RD_RS1_RS2("xor");
    WRITE_RD(RS1 ^ RS2);
}


void insn_xori(insn_t inst)
{
    DISASM_RD_RS1_IIMM("xori");
    WRITE_RD(RS1 ^ IIMM);
}


//} // namespace bemu
