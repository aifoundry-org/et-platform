/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#include "emu_gio.h"
#include "esrs.h"
#include "insn.h"
#include "insn_func.h"
#include "insn_util.h"
#include "log.h"
#include "mmu.h"
#include "processor.h"
#include "system.h"
#include "utility.h"
#ifdef SYS_EMU
#include "sys_emu.h"
#endif


#define PROGBUF_START (ESR_ABSCMD)
#define PROGBUF_END   (ESR_ABSCMD + 16)


namespace bemu {


// Helper enum / functions to update hastatus0
enum hastatus0_offset {
    hastatus0_halted = 0,
    hastatus0_running = 16,
    hastatus0_resumeack = 32,
    hastatus0_havereset = 48,
};


static inline void set_hastatus0(Hart& cpu, unsigned offset)
{
    const uint64_t mask = (1ull << index_in_neigh(cpu));
    cpu.chip->neigh_esrs[neigh_index(cpu)].hastatus0 |= (mask << offset);
}


static inline void clear_hastatus0(Hart& cpu, unsigned offset)
{
    const uint64_t mask = (1ull << index_in_neigh(cpu));
    cpu.chip->neigh_esrs[neigh_index(cpu)].hastatus0 &= ~(mask << offset);
}


// Messaging extension (from msgport.cpp)
void configure_port(Hart&, unsigned, uint32_t);


// Instruction execution function
using insn_exec_funct_t = void (*)(Hart&);

// Instruction decode function
using insn_decode_func_t = insn_exec_funct_t (*)(uint32_t, uint16_t&);


void tensor_wait_execute(Hart& cpu, Hart::Waiting what);


static Privilege get_privilege(uint64_t val)
{
    switch (val & 0x3) {
    case 0: return Privilege::U;
    case 1: return Privilege::S;
    case 3: return Privilege::M;
    default: return Privilege::U;
    }
}


static const char* debug_cause(Debug_entry::Cause cause)
{
    using Cause = Debug_entry::Cause;
    switch (cause) {
    case Cause::ebreak: return "ebreak";
    case Cause::trigger: return "trigger module";
    case Cause::haltreq: return "halt request";
    case Cause::step: return "single step";
    default:
        assert(0 && "Unreachable");
        return "unknown";
    }
}


// -----------------------------------------------------------------------------
//
// 32-bit instruction decoding
//
// -----------------------------------------------------------------------------

static insn_exec_funct_t dec_load(uint32_t bits, uint16_t& flags)
{
    static const insn_exec_funct_t functab[8] = {
        /* 000 */ insn_lb,
        /* 001 */ insn_lh,
        /* 010 */ insn_lw,
        /* 011 */ insn_ld,
        /* 100 */ insn_lbu,
        /* 101 */ insn_lhu,
        /* 110 */ insn_lwu,
        /* 111 */ insn_reserved
    };
    unsigned funct3 = (bits >> 12) & 7;
    flags |= Instruction::flag_LOAD;
    return functab[funct3];
}


static insn_exec_funct_t dec_load_fp(uint32_t bits, uint16_t& flags)
{
    unsigned funct3 = (bits >> 12) & 7;
    switch (funct3) {
    case 0x2: flags |= Instruction::flag_LOAD; return insn_flw;
    case 0x5: flags |= Instruction::flag_LOAD; return insn_flq2;
    default : return insn_reserved;
    }
}


static insn_exec_funct_t dec_custom0(uint32_t bits,
                                     uint16_t& flags __attribute__((unused)))
{
    unsigned funct3 = (bits >> 12) & 7;
    unsigned funct7 = (bits >> 25);
    switch (funct3) {
    case 0x0: return insn_fbc_ps;
    case 0x1: switch (funct7) {
              case 0x04: return insn_fg32b_ps;
              case 0x08: return insn_fg32h_ps;
              case 0x10: return insn_fg32w_ps;
              case 0x24: return insn_fgb_ps;
              case 0x28: return insn_fgh_ps;
              case 0x30: return insn_fgw_ps;
              case 0x44: return insn_fsc32b_ps;
              case 0x48: return insn_fsc32h_ps;
              case 0x50: return insn_fsc32w_ps;
              case 0x64: return insn_fscb_ps;
              case 0x68: return insn_fsch_ps;
              case 0x70: return insn_fscw_ps;
              default  : return insn_reserved;
              }
    case 0x2: return insn_flw_ps;
    case 0x3: return insn_fbcx_ps;
    case 0x4: flags |= Instruction::flag_CMO;
              switch (funct7) {
              case 0x03: return insn_famoaddl_pi;
              case 0x07: return insn_famoswapl_pi;
              case 0x0b: return insn_famoandl_pi;
              case 0x0f: return insn_famoorl_pi;
              case 0x13: return insn_famoxorl_pi;
              case 0x14: return insn_famomaxl_ps;
              case 0x17: return insn_famominl_pi;
              case 0x18: return insn_famominl_ps;
              case 0x1b: return insn_famomaxl_pi;
              case 0x1f: return insn_famominul_pi;
              case 0x23: return insn_famomaxul_pi;
              case 0x43: return insn_famoaddg_pi;
              case 0x47: return insn_famoswapg_pi;
              case 0x4b: return insn_famoandg_pi;
              case 0x4f: return insn_famoorg_pi;
              case 0x53: return insn_famoxorg_pi;
              case 0x54: return insn_famomaxg_ps;
              case 0x57: return insn_famoming_pi;
              case 0x58: return insn_famoming_ps;
              case 0x5b: return insn_famomaxg_pi;
              case 0x5f: return insn_famominug_pi;
              case 0x63: return insn_famomaxug_pi;
              default  : return insn_reserved;
              }
    case 0x6: return insn_fsw_ps;
    case 0x7: flags |= Instruction::flag_CMO;
              switch (funct7) {
              case 0x08: return (bits & 0x01f00000) ? insn_reserved : insn_flwl_ps; // rs2==0
              case 0x09: return (bits & 0x01f00000) ? insn_reserved : insn_flwg_ps; // rs2==0
              case 0x28: return (bits & 0x01f00000) ? insn_reserved : insn_fswl_ps; // rs2==0
              case 0x29: return (bits & 0x01f00000) ? insn_reserved : insn_fswg_ps; // rs2==0
              case 0x48: return insn_fgwl_ps;
              case 0x44: return insn_fghl_ps;
              case 0x40: return insn_fgbl_ps;
              case 0x49: return insn_fgwg_ps;
              case 0x45: return insn_fghg_ps;
              case 0x41: return insn_fgbg_ps;
              case 0x68: return insn_fscwl_ps;
              case 0x64: return insn_fschl_ps;
              case 0x60: return insn_fscbl_ps;
              case 0x69: return insn_fscwg_ps;
              case 0x65: return insn_fschg_ps;
              case 0x61: return insn_fscbg_ps;
              default  : return insn_reserved;
              }
    default : return insn_reserved;
    }
}


static insn_exec_funct_t dec_misc_mem(uint32_t bits,
                                      uint16_t& flags __attribute__((unused)))
{
    unsigned funct3 = (bits >> 12) & 7;
    /*unsigned fm = (bits >> 28);*/
    switch (funct3) {
    case 0x0: return /*(fm == 8) ? insn_fence_tso :*/ insn_fence;
    case 0x1: return insn_fence_i;
    default : return insn_reserved;
    }
}


static insn_exec_funct_t dec_op_imm(uint32_t bits,
                                    uint16_t& flags __attribute__((unused)))
{
    unsigned funct3 = (bits >> 12) & 7;
    unsigned funct6 = (bits >> 26);
    switch (funct3) {
    case 0x0: return insn_addi;
    case 0x1: return (funct6 == 0) ? insn_slli : insn_reserved;
    case 0x2: return insn_slti;
    case 0x3: return insn_sltiu;
    case 0x4: return insn_xori;
    case 0x5: switch (funct6) {
              case 0x00: return insn_srli;
              case 0x10: return insn_srai;
              default  : return insn_reserved;
              }
    case 0x6: return insn_ori;
    case 0x7: return insn_andi;
    default : return insn_reserved;
    }
}


static insn_exec_funct_t dec_auipc(uint32_t bits __attribute__((unused)),
                                   uint16_t& flags __attribute__((unused)))
{
    return insn_auipc;
}


static insn_exec_funct_t dec_op_imm_32(uint32_t bits,
                                       uint16_t& flags __attribute__((unused)))
{
    unsigned funct3 = (bits >> 12) & 7;
    unsigned funct7 = (bits >> 25);
    switch (funct3) {
    case 0x0: return insn_addiw;
    case 0x1: return (funct7 == 0) ? insn_slliw : insn_reserved;
    case 0x5: switch (funct7) {
              case 0x00: return insn_srliw;
              case 0x20: return insn_sraiw;
              default  : return insn_reserved;
              }
    default : return insn_reserved;
    }
}


static insn_exec_funct_t dec_insn_48b_0(uint32_t bits __attribute__((unused)),
                                        uint16_t& flags __attribute__((unused)))
{
    return insn_fbci_ps;
}


static insn_exec_funct_t dec_store(uint32_t bits,
                                   uint16_t& flags __attribute__((unused)))
{
    static const insn_exec_funct_t functab[8] = {
        /* 000 */ insn_sb,
        /* 001 */ insn_sh,
        /* 010 */ insn_sw,
        /* 011 */ insn_sd,
        /* 100 */ insn_reserved,
        /* 101 */ insn_reserved,
        /* 110 */ insn_reserved,
        /* 111 */ insn_reserved
    };
    unsigned funct3 = (bits >> 12) & 7;
    return functab[funct3];
}


static insn_exec_funct_t dec_store_fp(uint32_t bits,
                                      uint16_t& flags __attribute__((unused)))
{
    unsigned funct3 = (bits >> 12) & 7;
    switch (funct3) {
    case 0x2: return insn_fsw;
    case 0x5: return insn_fsq2;
    default : return insn_reserved;
    }
}


static insn_exec_funct_t dec_custom1(uint32_t bits __attribute__((unused)),
                                     uint16_t& flags __attribute__((unused)))
{
    return insn_reserved;
}


static insn_exec_funct_t dec_amo(uint32_t bits __attribute__((unused)),
                                 uint16_t& flags __attribute__((unused)))
{
#if 0
    unsigned funct3 = (bits >> 12) & 7;
    unsigned funct5 = (bits >> 27);
    switch (funct3) {
    case 0x2: flags |= Instruction::flag_CMO;
              switch (funct5) {
              case 0x00: return insn_amoadd_w;
              case 0x01: return insn_amoswap_w;
              case 0x02: return insn_lr_w;
              case 0x03: return insn_sc_w;
              case 0x04: return insn_amoxor_w;
              case 0x08: return insn_amoor_w;
              case 0x0c: return insn_amoand_w;
              case 0x10: return insn_amomin_w;
              case 0x14: return insn_amomax_w;
              case 0x18: return insn_amominu_w;
              case 0x1c: return insn_amomaxu_w;
              default  : return insn_reserved;
              }
    case 0x3: flags |= Instruction::flag_CMO;
              switch (funct5) {
              case 0x00: return insn_amoadd_d;
              case 0x01: return insn_amoswap_d;
              case 0x02: return insn_lr_d;
              case 0x03: return insn_sc_d;
              case 0x04: return insn_amoxor_d;
              case 0x08: return insn_amoor_d;
              case 0x0c: return insn_amoand_d;
              case 0x10: return insn_amomin_d;
              case 0x14: return insn_amomax_d;
              case 0x18: return insn_amominu_d;
              case 0x1c: return insn_amomaxu_d;
              default  : return insn_reserved;
              }
    default : return insn_reserved;
    }
#else
    return insn_reserved;
#endif
}


static insn_exec_funct_t dec_op(uint32_t bits,
                                uint16_t& flags __attribute__((unused)))
{
    static const insn_exec_funct_t functab00[8] = {
        /* 000 */ insn_add,
        /* 001 */ insn_sll,
        /* 010 */ insn_slt,
        /* 011 */ insn_sltu,
        /* 100 */ insn_xor,
        /* 101 */ insn_srl,
        /* 110 */ insn_or,
        /* 111 */ insn_and
    };
    static const insn_exec_funct_t functab01[8] = {
        /* 000 */ insn_mul,
        /* 001 */ insn_mulh,
        /* 010 */ insn_mulhsu,
        /* 011 */ insn_mulhu,
        /* 100 */ insn_div,
        /* 101 */ insn_divu,
        /* 110 */ insn_rem,
        /* 111 */ insn_remu
    };
    static const insn_exec_funct_t functab20[8] = {
        /* 000 */ insn_sub,
        /* 001 */ insn_reserved,
        /* 010 */ insn_reserved,
        /* 011 */ insn_reserved,
        /* 100 */ insn_reserved,
        /* 101 */ insn_sra,
        /* 110 */ insn_reserved,
        /* 111 */ insn_reserved
    };
    unsigned funct3 = (bits >> 12) & 7;
    unsigned funct7 = (bits >> 25);
    switch (funct7) {
    case 0x00: return functab00[funct3];
    case 0x01: return functab01[funct3];
    case 0x20: return functab20[funct3];
    default  : return insn_reserved;
    }
}


static insn_exec_funct_t dec_lui(uint32_t bits __attribute__((unused)),
                                 uint16_t& flags __attribute__((unused)))
{
    return insn_lui;
}


static insn_exec_funct_t dec_op_32(uint32_t bits,
                                   uint16_t& flags __attribute__((unused)))
{
#if 0
    static const insn_exec_funct_t functab00[8] = {
        /* 000 */ insn_addw,
        /* 001 */ insn_sllw,
        /* 010 */ insn_reserved,
        /* 011 */ insn_reserved,
        /* 100 */ insn_reserved,
        /* 101 */ insn_srlw,
        /* 110 */ insn_reserved,
        /* 111 */ insn_reserved
    };
    static const insn_exec_funct_t functab01[8] = {
        /* 000 */ insn_mulw,
        /* 001 */ insn_reserved,
        /* 010 */ insn_reserved,
        /* 011 */ insn_reserved,
        /* 100 */ insn_divw,
        /* 101 */ insn_divuw,
        /* 110 */ insn_remw,
        /* 111 */ insn_remuw
    };
    static const insn_exec_funct_t functab20[8] = {
        /* 000 */ insn_subw,
        /* 001 */ insn_reserved,
        /* 010 */ insn_reserved,
        /* 011 */ insn_reserved,
        /* 100 */ insn_reserved,
        /* 101 */ insn_sraw,
        /* 110 */ insn_reserved,
        /* 111 */ insn_reserved
    };
    unsigned funct3 = (bits >> 12) & 7;
    unsigned funct7 = (bits >> 25);
    switch (funct7) {
    case 0x00: return functab00[funct3];
    case 0x01: return functab01[funct3];
    case 0x20: return functab20[funct3];
    default  : return insn_reserved;
    }
#else
    unsigned funct3 = (bits >> 12) & 7;
    unsigned funct7 = (bits >> 25);
    switch (funct3) {
    case 0x0: switch (funct7) {
              case 0x00: return insn_addw;
              case 0x01: return insn_mulw;
              case 0x20: return insn_subw;
              default  : return insn_reserved;
              }
    case 0x1: return (funct7 == 0) ? insn_sllw : insn_reserved;
    case 0x2: flags |= Instruction::flag_CMO;
              switch (funct7) {
              case 0x00: return insn_amoaddl_w;
              case 0x01: return insn_amoaddg_w;
              case 0x04: return insn_amoswapl_w;
              case 0x05: return insn_amoswapg_w;
              case 0x10: return insn_amoxorl_w;
              case 0x11: return insn_amoxorg_w;
              case 0x20: return insn_amoorl_w;
              case 0x21: return insn_amoorg_w;
              case 0x30: return insn_amoandl_w;
              case 0x31: return insn_amoandg_w;
              case 0x40: return insn_amominl_w;
              case 0x41: return insn_amoming_w;
              case 0x50: return insn_amomaxl_w;
              case 0x51: return insn_amomaxg_w;
              case 0x60: return insn_amominul_w;
              case 0x61: return insn_amominug_w;
              case 0x70: return insn_amomaxul_w;
              case 0x71: return insn_amomaxug_w;
              case 0x78: return insn_amocmpswapl_w;
              case 0x79: return insn_amocmpswapg_w;
              default  : return insn_reserved;
              }
    case 0x3: flags |= Instruction::flag_CMO;
              switch (funct7) {
              case 0x00: return insn_amoaddl_d;
              case 0x01: return insn_amoaddg_d;
              case 0x04: return insn_amoswapl_d;
              case 0x05: return insn_amoswapg_d;
              case 0x08: return insn_sbl;
              case 0x09: return insn_sbg;
              case 0x0C: return insn_shl;
              case 0x0D: return insn_shg;
              case 0x10: return insn_amoxorl_d;
              case 0x11: return insn_amoxorg_d;
              case 0x20: return insn_amoorl_d;
              case 0x21: return insn_amoorg_d;
              case 0x30: return insn_amoandl_d;
              case 0x31: return insn_amoandg_d;
              case 0x40: return insn_amominl_d;
              case 0x41: return insn_amoming_d;
              case 0x50: return insn_amomaxl_d;
              case 0x51: return insn_amomaxg_d;
              case 0x60: return insn_amominul_d;
              case 0x61: return insn_amominug_d;
              case 0x70: return insn_amomaxul_d;
              case 0x71: return insn_amomaxug_d;
              case 0x78: return insn_amocmpswapl_d;
              case 0x79: return insn_amocmpswapg_d;
              default  : return insn_reserved;
              }
    case 0x4: return (funct7 == 0x01) ? insn_divw : insn_reserved;
    case 0x5: switch (funct7) {
              case 0x00: return insn_srlw;
              case 0x01: return insn_divuw;
              case 0x20: return insn_sraw;
              default  : return insn_reserved;
              }
    case 0x6: switch (funct7) {
              case 0x01: return insn_remw;
              case 0x40: return insn_packb;
              default  : return insn_reserved;
              }
    case 0x7: switch (funct7) {
              case 0x01: return insn_remuw;
              case 0x40: return insn_bitmixb;
              default  : return insn_reserved;
              }
    default : return insn_reserved;
    }
#endif
}


static insn_exec_funct_t dec_insn_64b(uint32_t bits,
                                      uint16_t& flags __attribute__((unused)))
{
    static const insn_exec_funct_t functab0[4] = {
        /* 00 */ insn_reserved,
        /* 01 */ insn_reserved,
        /* 10 */ insn_faddi_pi,
        /* 11 */ insn_reserved
    };
    static const insn_exec_funct_t functab1[4] = {
        /* 00 */ insn_reserved,
        /* 01 */ insn_reserved,
        /* 10 */ insn_fandi_pi,
        /* 11 */ insn_reserved
    };
    static const insn_exec_funct_t functab2[4] = {
        /* 00 */ insn_reserved,
        /* 01 */ insn_reserved,
        /* 10 */ insn_fcmov_ps,
        /* 11 */ insn_reserved
    };
    unsigned funct3 = (bits >> 12) & 7;
    unsigned fmt = (bits >> 25) & 3;
    switch (funct3) {
    case 0x0: return functab0[fmt];
    case 0x1: return functab1[fmt];
    case 0x2: return functab2[fmt];
    default : return insn_reserved;
    }
}


static insn_exec_funct_t dec_madd(uint32_t bits,
                                  uint16_t& flags __attribute__((unused)))
{
    unsigned funct2 = (bits >> 25) & 2;
    return (funct2 == 0) ? insn_fmadd_s : insn_reserved;
}


static insn_exec_funct_t dec_msub(uint32_t bits,
                                  uint16_t& flags __attribute__((unused)))
{
    unsigned funct2 = (bits >> 25) & 2;
    return (funct2 == 0) ? insn_fmsub_s : insn_reserved;
}


static insn_exec_funct_t dec_nmsub(uint32_t bits,
                                   uint16_t& flags __attribute__((unused)))
{
    unsigned funct2 = (bits >> 25) & 2;
    return (funct2 == 0) ? insn_fnmsub_s : insn_reserved;
}


static insn_exec_funct_t dec_nmadd(uint32_t bits,
                                   uint16_t& flags __attribute__((unused)))
{
    unsigned funct2 = (bits >> 25) & 2;
    return (funct2 == 0) ? insn_fnmadd_s : insn_reserved;
}


static insn_exec_funct_t dec_op_fp(uint32_t bits,
                                   uint16_t& flags __attribute__((unused)))
{
    unsigned funct3 = (bits >> 12) & 7;
    unsigned funct7 = (bits >> 25);
    unsigned rs2 = (bits >> 20) & 31;
    switch (funct7) {
    case 0x00: return insn_fadd_s;
    case 0x04: return insn_fsub_s;
    case 0x08: return insn_fmul_s;
    case 0x0c: return insn_fdiv_s;
    case 0x10: switch (funct3) {
               case 0x0: return insn_fsgnj_s;
               case 0x1: return insn_fsgnjn_s;
               case 0x2: return insn_fsgnjx_s;
               default : return insn_reserved;
               }
    case 0x14: switch (funct3) {
               case 0x0: return insn_fmin_s;
               case 0x1: return insn_fmax_s;
               default : return insn_reserved;
               }
    case 0x2c: return (rs2 == 0) ? insn_fsqrt_s : insn_reserved;
    case 0x50: switch (funct3) {
               case 0x0: return insn_fle_s;
               case 0x1: return insn_flt_s;
               case 0x2: return insn_feq_s;
               default : return insn_reserved;
               }
    case 0x60: switch (rs2) {
               case 0x00: return insn_fcvt_w_s;
               case 0x01: return insn_fcvt_wu_s;
               case 0x02: return insn_fcvt_l_s;
               case 0x03: return insn_fcvt_lu_s;
               default  : return insn_reserved;
               }
    case 0x68: switch (rs2) {
               case 0x00: return insn_fcvt_s_w;
               case 0x01: return insn_fcvt_s_wu;
               case 0x02: return insn_fcvt_s_l;
               case 0x03: return insn_fcvt_s_lu;
               default  : return insn_reserved;
               }
    case 0x70: switch (funct3) {
               case 0x0: return (rs2 == 0) ? insn_fmv_x_w  : insn_reserved;
               case 0x1: return (rs2 == 0) ? insn_fclass_s : insn_reserved;
               default : return insn_reserved;
               }
    case 0x78: return (rs2 | funct3) ? insn_reserved : insn_fmv_w_x;
    default  : return insn_reserved;
    }
}


static insn_exec_funct_t dec_reserved0(uint32_t bits __attribute__((unused)),
                                       uint16_t& flags __attribute__((unused)))
{
    return insn_reserved;
}


static insn_exec_funct_t dec_custom2(uint32_t bits,
                                     uint16_t& flags __attribute__((unused)))
{
    static const insn_exec_funct_t functab[4] = {
        /* 00 */ insn_fmadd_ps,
        /* 01 */ insn_fmsub_ps,
        /* 10 */ insn_fnmsub_ps,
        /* 11 */ insn_fnmadd_ps,
    };
    unsigned fmt = (bits >> 25) & 3;
    return functab[fmt];
}


static insn_exec_funct_t dec_insn_48b_1(uint32_t bits __attribute__((unused)),
                                        uint16_t& flags __attribute__((unused)))
{
    return insn_fbci_pi;
}


static insn_exec_funct_t dec_branch(uint32_t bits,
                                    uint16_t& flags __attribute__((unused)))
{
    static const insn_exec_funct_t functab[8] = {
        /* 000 */ insn_beq,
        /* 001 */ insn_bne,
        /* 010 */ insn_reserved,
        /* 011 */ insn_reserved,
        /* 100 */ insn_blt,
        /* 101 */ insn_bge,
        /* 110 */ insn_bltu,
        /* 111 */ insn_bgeu
    };
    unsigned funct3 = (bits >> 12) & 7;
    return functab[funct3];
}


static insn_exec_funct_t dec_jalr(uint32_t bits __attribute__((unused)),
                                  uint16_t& flags __attribute__((unused)))
{
    unsigned funct3 = (bits >> 12) & 7;
    if (funct3 != 0) return insn_reserved;
    return insn_jalr;
}


static insn_exec_funct_t dec_reserved1(uint32_t bits __attribute__((unused)),
                                       uint16_t& flags __attribute__((unused)))
{
    return insn_reserved;
}


static insn_exec_funct_t dec_jal(uint32_t bits __attribute__((unused)),
                                 uint16_t& flags __attribute__((unused)))
{
    return insn_jal;
}


static insn_exec_funct_t dec_system(uint32_t bits, uint16_t& flags)
{
    static const insn_exec_funct_t functab[8] = {
        /* 000 */ nullptr,
        /* 001 */ insn_csrrw,
        /* 010 */ insn_csrrs,
        /* 011 */ insn_csrrc,
        /* 100 */ insn_reserved,
        /* 101 */ insn_csrrwi,
        /* 110 */ insn_csrrsi,
        /* 111 */ insn_csrrci
    };
    unsigned funct3 = (bits >> 12) & 7;
    uint16_t imm12 = (bits >> 20) & 0xfff;
    uint16_t rd = (bits >> 7) & 31;
    uint16_t rs1 = (bits >> 15) & 31;
    if (funct3 == 0) {
        switch (imm12) {
        case 0x000: return (rd | rs1) ? insn_reserved : insn_ecall;
        case 0x001: return (rd | rs1) ? insn_reserved : insn_ebreak;
        //case 0x002: return (rd | rs1) ? insn_reserved : insn_uret;
        case 0x102: return (rd | rs1) ? insn_reserved : insn_sret;
        case 0x302: return (rd | rs1) ? insn_reserved : insn_mret;
        case 0x105: return (rd | rs1) ? insn_reserved : insn_wfi;
        }
        if ((imm12 >= 0x120) && (imm12 <= 0x13f) && (rd == 0)) {
            return insn_sfence_vma;
        }
        return insn_reserved;
    }
    if ((funct3 == 1) || (funct3 == 5)) {
        // csrrw, csrrwi
        if (rd) {
            flags |= Instruction::flag_CSR_READ;
        }
        flags |= Instruction::flag_CSR_WRITE;
    }
    else if (funct3 != 4) {
        // csrs, csrc, csrsi, csrci
        if (rs1) {
            flags |= Instruction::flag_CSR_WRITE;
        }
        flags |= Instruction::flag_CSR_READ;
    }
    switch (imm12) {
    case 0x800 : flags |= Instruction::flag_REDUCE; break;
    case 0x801 : flags |= Instruction::flag_TENSOR_FMA; break;
    case 0x806 : flags |= Instruction::flag_TENSOR_QUANT; break;
    case 0x820 : flags |= Instruction::flag_FLB; break;
    case 0x83f : flags |= Instruction::flag_TENSOR_LOAD; break;
    case 0x87f : flags |= Instruction::flag_TENSOR_STORE; break;
    }
    return functab[funct3];
}


static insn_exec_funct_t dec_reserved2(uint32_t bits,
                                       uint16_t& flags __attribute__((unused)))
{
    unsigned funct3 = (bits >> 12) & 7;
    unsigned funct7 = (bits >> 25);
    return (funct3 | funct7) ? insn_reserved : insn_fcmovm_ps;
}


static insn_exec_funct_t dec_custom3(uint32_t bits,
                                     uint16_t& flags __attribute__((unused)))
{
    unsigned funct3 = (bits >> 12) & 7;
    unsigned funct7 = (bits >> 25);
    unsigned rs2 = (bits >> 20) & 31;
    switch (funct7) {
    case 0x00: return insn_fadd_ps;
    case 0x03: switch (funct3) {
               case 0x0: return insn_fadd_pi;
               case 0x1: return insn_fsll_pi;
               case 0x2: return (rs2 == 0) ? insn_fnot_pi : insn_reserved;
               case 0x3: switch (rs2) {
                         case 0x0: return insn_fsat8_pi;
                         case 0x1: return insn_fsatu8_pi;
                         default : return insn_reserved;
                         }
               case 0x4: return insn_fxor_pi;
               case 0x5: return insn_fsrl_pi;
               case 0x6: return insn_for_pi;
               case 0x7: return insn_fand_pi;
               default : return insn_reserved;
               }
    case 0x04: return insn_fsub_ps;
    case 0x07: switch (funct3) {
               case 0x0: return insn_fsub_pi;
               case 0x5: return insn_fsra_pi;
               default : return insn_reserved;
               }
    case 0x08: return insn_fmul_ps;
    case 0x0b: switch (funct3) {
               case 0x0: return insn_fmul_pi;
               case 0x1: return insn_fmulh_pi;
               case 0x2: return insn_fmulhu_pi;
               default : return insn_reserved;
               }
    case 0x0c: return insn_fdiv_ps;
    case 0x0f: switch (funct3) {
               case 0x0: return insn_fdiv_pi;
               case 0x1: return insn_fdivu_pi;
               case 0x2: return insn_frem_pi;
               case 0x3: return insn_fremu_pi;
               default : return insn_reserved;
               }
    case 0x10: switch (funct3) {
               case 0x0: return insn_fsgnj_ps;
               case 0x1: return insn_fsgnjn_ps;
               case 0x2: return insn_fsgnjx_ps;
               default : return insn_reserved;
               }
    case 0x13: switch (funct3) {
               case 0x0: return (rs2 == 0) ? insn_fpackrepb_pi : insn_reserved;
               case 0x1: return (rs2 == 0) ? insn_fpackreph_pi : insn_reserved;
               default : return insn_reserved;
               }
    case 0x14: switch (funct3) {
               case 0x0: return insn_fmin_ps;
               case 0x1: return insn_fmax_ps;
               default : return insn_reserved;
               }
    case 0x17: switch (funct3) {
               case 0x0: return insn_fmin_pi;
               case 0x1: return insn_fmax_pi;
               case 0x2: return insn_fminu_pi;
               case 0x3: return insn_fmaxu_pi;
               default : return insn_reserved;
               }
    case 0x18: return insn_frcp_fix_rast;
    case 0x1f: return (bits & 0x00007c00) ? insn_reserved : insn_fltm_pi; // funct3|rd[4:3] == 0
    case 0x27: switch (funct3) {
               case 0x1: return insn_fslli_pi;
               case 0x5: return insn_fsrli_pi;
               case 0x7: return insn_fsrai_pi;
               default : return insn_reserved;
               }
    case 0x29: return (bits & 0x01fc7000) ? insn_reserved : insn_maskpopc;  // rs2|rs1[4:3]|funct3
    case 0x2a: return (bits & 0x01fc7000) ? insn_reserved : insn_maskpopcz; // rs2|rs1[4:3]|funct3
    case 0x2b: return (bits & 0x00000c00) ? insn_reserved : insn_mov_m_x;   // rd[4:3] == 0
    case 0x2c: switch (rs2) {
               case 0x00: return (funct3 == 0) ? insn_fsqrt_ps : insn_reserved;
               case 0x01: return insn_fround_ps;
               case 0x02: return (funct3 == 0) ? insn_ffrc_ps : insn_reserved;
               case 0x03: return (funct3 == 0) ? insn_flog_ps : insn_reserved;
               case 0x04: return (funct3 == 0) ? insn_fexp_ps : insn_reserved;
               case 0x06: return (funct3 == 0) ? insn_fsin_ps : insn_reserved;
               case 0x07: return (funct3 == 0) ? insn_frcp_ps : insn_reserved;
               case 0x08: return (funct3 == 0) ? insn_frsq_ps : insn_reserved;
               default  : return insn_reserved;
               }
    case 0x2f: return (funct3 == 0) ? insn_maskpopc_rast : insn_reserved;
    case 0x33: switch (funct3) {
               case 0x2: return (bits & 0x01fc0c00) ? insn_reserved : insn_masknot; // rs2|rs1[4:3]|rd[4:3] == 0
               case 0x4: return (bits & 0x018c0c00) ? insn_reserved : insn_maskxor; // rs2[4:3]|rs1[4:3]|rd[4:3] == 0
               case 0x6: return (bits & 0x018c0c00) ? insn_reserved : insn_maskor;  // rs2[4:3]|rs1[4:3]|rd[4:3] == 0
               case 0x7: return (bits & 0x018c0c00) ? insn_reserved : insn_maskand; // rs2[4:3]|rs1[4:3]|rd[4:3] == 0
               default : return insn_reserved;
               }
    case 0x44: switch (funct3) {
               case 0x0: return insn_cubeface_ps;
               case 0x1: return insn_cubefaceidx_ps;
               case 0x2: return insn_cubesgnsc_ps;
               case 0x3: return insn_cubesgntc_ps;
               default : return insn_reserved;
               }
    case 0x50: switch (funct3) {
               case 0x0: return insn_fle_ps;
               case 0x1: return insn_flt_ps;
               case 0x2: return insn_feq_ps;
               case 0x4: return (bits & 0x00000c00) ? insn_reserved : insn_flem_ps; // rd[4:3] == 0
               case 0x5: return (bits & 0x00000c00) ? insn_reserved : insn_fltm_ps; // rd[4:3] == 0
               case 0x6: return (bits & 0x00000c00) ? insn_reserved : insn_feqm_ps; // rd[4:3] == 0
               default : return insn_reserved;
               }
    case 0x53: switch (funct3) {
               case 0x0: return insn_fle_pi;
               case 0x1: return insn_flt_pi;
               case 0x2: return insn_feq_pi;
               case 0x3: return insn_fltu_pi;
               case 0x4: return (bits & 0x01f00c00) ? insn_reserved : insn_fsetm_pi;  // rs2|rd[4:3] == 0
               default : return insn_reserved;
               }
    case 0x60: switch (rs2) {
               case 0x00: return insn_fcvt_pw_ps;
               case 0x01: return insn_fcvt_pwu_ps;
               case 0x02: return (funct3 == 0) ? insn_fcvt_rast_ps : insn_reserved;
               default  : return insn_reserved;
               }
    case 0x68: switch (rs2) {
               case 0x00: return insn_fcvt_ps_pw;
               case 0x01: return insn_fcvt_ps_pwu;
               case 0x02: return insn_fcvt_ps_rast;
               case 0x08: return (funct3 == 0) ? insn_fcvt_ps_f10  : insn_reserved;
               case 0x09: return (funct3 == 0) ? insn_fcvt_ps_f11  : insn_reserved;
               case 0x0a: return (funct3 == 0) ? insn_fcvt_ps_f16  : insn_reserved;
               case 0x10: return (funct3 == 0) ? insn_fcvt_ps_un24 : insn_reserved;
               case 0x11: return (funct3 == 0) ? insn_fcvt_ps_un16 : insn_reserved;
               case 0x12: return (funct3 == 0) ? insn_fcvt_ps_un10 : insn_reserved;
               case 0x13: return (funct3 == 0) ? insn_fcvt_ps_un8  : insn_reserved;
               case 0x17: return (funct3 == 0) ? insn_fcvt_ps_un2  : insn_reserved;
               case 0x19: return (funct3 == 0) ? insn_fcvt_ps_sn16 : insn_reserved;
               case 0x1b: return (funct3 == 0) ? insn_fcvt_ps_sn8  : insn_reserved;
               default  : return insn_reserved;
               }
    case 0x6b: if ((bits & 0x01fff000) == 0x00000000) return insn_mova_x_m; // rs2|rs1|funct3 == 0
               if ((bits & 0x01f07f80) == 0x00001000) return insn_mova_m_x; // rs2|rd == 0, funct3 == 1
               return insn_reserved;
    case 0x6c: switch (rs2) {
               case 0x08: return (funct3 == 0) ? insn_fcvt_f11_ps  : insn_reserved;
               case 0x09: return (funct3 == 0) ? insn_fcvt_f16_ps  : insn_reserved;
               case 0x0b: return (funct3 == 0) ? insn_fcvt_f10_ps  : insn_reserved;
               case 0x10: return (funct3 == 0) ? insn_fcvt_un24_ps : insn_reserved;
               case 0x11: return (funct3 == 0) ? insn_fcvt_un16_ps : insn_reserved;
               case 0x12: return (funct3 == 0) ? insn_fcvt_un10_ps : insn_reserved;
               case 0x13: return (funct3 == 0) ? insn_fcvt_un8_ps  : insn_reserved;
               case 0x17: return (funct3 == 0) ? insn_fcvt_un2_ps  : insn_reserved;
               case 0x19: return (funct3 == 0) ? insn_fcvt_sn16_ps : insn_reserved;
               case 0x1b: return (funct3 == 0) ? insn_fcvt_sn8_ps  : insn_reserved;
               default  : return insn_reserved;
               }
    case 0x70: switch (funct3) {
               case 0x0: return (bits & 0x01800000) ? insn_reserved : insn_fmvz_x_ps; // rs2[4:3] == 0
               case 0x1: return (bits & 0x01f00000) ? insn_reserved : insn_fclass_ps; // rs2 == 0
               case 0x2: return (bits & 0x01800000) ? insn_reserved : insn_fmvs_x_ps; // rs2[4:3] == 0
               default : return insn_reserved;
               }
    case 0x73: return insn_fswizz_ps;
    default  : return insn_reserved;
    }
}


static insn_exec_funct_t dec_insn_80b(uint32_t bits __attribute__((unused)),
                                      uint16_t& flags __attribute__((unused)))
{
    return insn_reserved;
}


// -----------------------------------------------------------------------------
//
// 16-bit instruction decoding
//
// -----------------------------------------------------------------------------

// ----- Quadrant 0 -----

static insn_exec_funct_t dec_c_addi4spn(uint32_t bits,
                                        uint16_t& flags __attribute__((unused)))
{
    if (bits != 0) {
        uint16_t nzuimm = (bits >> 5) & 0xff;
        return (nzuimm != 0) ? insn_c_addi4spn : insn_c_reserved;
    }
    return insn_c_illegal;
}


static insn_exec_funct_t dec_c_fld(uint32_t bits __attribute__((unused)),
                                   uint16_t& flags __attribute__((unused)))
{
    return insn_c_reserved; // RV64D: insn_c_fld
}


static insn_exec_funct_t dec_c_lw(uint32_t bits __attribute__((unused)),
                                  uint16_t& flags)
{
    flags |= Instruction::flag_LOAD;
    return insn_c_lw;
}


static insn_exec_funct_t dec_c_ld(uint32_t bits __attribute__((unused)),
                                  uint16_t& flags)
{
    flags |= Instruction::flag_LOAD;
    return insn_c_ld;
}


static insn_exec_funct_t dec_c_reserved(uint32_t bits __attribute__((unused)),
                                        uint16_t& flags __attribute__((unused)))
{
    return insn_c_reserved;
}


static insn_exec_funct_t dec_c_fsd(uint32_t bits __attribute__((unused)),
                                   uint16_t& flags __attribute__((unused)))
{
    return insn_c_reserved; // RV64D: insn_c_fsd
}


static insn_exec_funct_t dec_c_sw(uint32_t bits __attribute__((unused)),
                                  uint16_t& flags __attribute__((unused)))
{
    return insn_c_sw;
}


static insn_exec_funct_t dec_c_sd(uint32_t bits __attribute__((unused)),
                                  uint16_t& flags __attribute__((unused)))
{
    return insn_c_sd;
}


// ----- Quadrant 1 -----

static insn_exec_funct_t dec_c_addi(uint32_t bits __attribute__((unused)),
                                    uint16_t& flags __attribute__((unused)))
{
    return insn_c_addi;
}


static insn_exec_funct_t dec_c_addiw(uint32_t bits,
                                     uint16_t& flags __attribute__((unused)))
{
    uint16_t rs1_rd = (bits >> 7) & 31;
    return (rs1_rd == 0) ? insn_c_reserved : insn_c_addiw;
}


static insn_exec_funct_t dec_c_li(uint32_t bits __attribute__((unused)),
                                  uint16_t& flags __attribute__((unused)))
{
    return insn_c_li;
}


static insn_exec_funct_t dec_c_lui_addi16sp(uint32_t bits,
                                            uint16_t& flags __attribute__((unused)))
{
    uint16_t nzimm = ((bits >> 7) & 0x20) | ((bits >> 2) & 0x1f);
    if (nzimm != 0) {
        uint16_t rs1_rd = (bits >> 7) & 31;
        return (rs1_rd == 2) ? insn_c_addi16sp : insn_c_lui;
    }
    return insn_c_reserved;
}


static insn_exec_funct_t dec_c_misc_alu(uint32_t bits,
                                        uint16_t& flags __attribute__((unused)))
{
    // bits 12|11:10|6:5
    static const insn_exec_funct_t functab[32] = {
                   /*xxx.00*/   /*xxx.01*/   /*xxx.10*/       /*xxx.11*/
        /*000.xx*/ insn_c_srli, insn_c_srli, insn_c_srli,     insn_c_srli,
        /*001.xx*/ insn_c_srai, insn_c_srai, insn_c_srai,     insn_c_srai,
        /*010.xx*/ insn_c_andi, insn_c_andi, insn_c_andi,     insn_c_andi,
        /*011.xx*/ insn_c_sub,  insn_c_xor,  insn_c_or,       insn_c_and,
        /*100.xx*/ insn_c_srli, insn_c_srli, insn_c_srli,     insn_c_srli,
        /*101.xx*/ insn_c_srai, insn_c_srai, insn_c_srai,     insn_c_srai,
        /*110.xx*/ insn_c_andi, insn_c_andi, insn_c_andi,     insn_c_andi,
        /*111.xx*/ insn_c_subw, insn_c_addw, insn_c_reserved, insn_c_reserved
    };
    uint16_t idx = ((bits >> 8) & 0x1c) | ((bits >> 5) & 0x3);
    return functab[idx];
}


static insn_exec_funct_t dec_c_j(uint32_t bits __attribute__((unused)),
                                 uint16_t& flags __attribute__((unused)))
{
    return insn_c_j;
}


static insn_exec_funct_t dec_c_beqz(uint32_t bits __attribute__((unused)),
                                    uint16_t& flags __attribute__((unused)))
{
    return insn_c_beqz;
}


static insn_exec_funct_t dec_c_bnez(uint32_t bits __attribute__((unused)),
                                    uint16_t& flags __attribute__((unused)))
{
    return insn_c_bnez;
}


// ----- Quadrant 2 -----

static insn_exec_funct_t dec_c_slli(uint32_t bits __attribute__((unused)),
                                    uint16_t& flags __attribute__((unused)))
{
    return insn_c_slli;
}


static insn_exec_funct_t dec_c_fldsp(uint32_t bits __attribute__((unused)),
                                     uint16_t& flags __attribute__((unused)))
{
    return insn_c_reserved; // RV64D: insn_c_fldsp
}


static insn_exec_funct_t dec_c_lwsp(uint32_t bits, uint16_t& flags)
{
    uint16_t rs1_rd = (bits >> 7) & 31;
    flags |= Instruction::flag_LOAD;
    return (rs1_rd == 0) ? insn_c_reserved : insn_c_lwsp;
}


static insn_exec_funct_t dec_c_ldsp(uint32_t bits, uint16_t& flags)
{
    uint16_t rs1_rd = (bits >> 7) & 31;
    flags |= Instruction::flag_LOAD;
    return (rs1_rd == 0) ? insn_c_reserved : insn_c_ldsp;
}


static insn_exec_funct_t dec_c_jalr_mv_add(uint32_t bits,
                                           uint16_t& flags __attribute__((unused)))
{
    // idx[2] = bits[12]
    // idx[1] = (rs1_rd != 0)
    // idx[0] = (rs2 != 0)
    static const insn_exec_funct_t functab[8] = {
        /* 000 */ insn_c_reserved,
        /* 001 */ insn_c_mv,
        /* 010 */ insn_c_jr,
        /* 011 */ insn_c_mv,
        /* 100 */ insn_c_ebreak,
        /* 101 */ insn_c_add,
        /* 110 */ insn_c_jalr,
        /* 111 */ insn_c_add
    };
    uint16_t idx = ((bits >> 10) & 0x4)
                 | (((bits >> 7) & 31) ? 0x2 : 0x0)
                 | (((bits >> 2) & 31) ? 0x1 : 0x0);
    return functab[idx];
}


static insn_exec_funct_t dec_c_fsdsp(uint32_t bits __attribute__((unused)),
                                     uint16_t& flags __attribute__((unused)))
{
    return insn_c_reserved;
}


static insn_exec_funct_t dec_c_swsp(uint32_t bits __attribute__((unused)),
                                    uint16_t& flags __attribute__((unused)))
{
    return insn_c_swsp;
}


static insn_exec_funct_t dec_c_sdsp(uint32_t bits __attribute__((unused)),
                                    uint16_t& flags __attribute__((unused)))
{
    return insn_c_sdsp;
}


// -----------------------------------------------------------------------------
//
// Decode and execute an instruction
//
// -----------------------------------------------------------------------------

// Opcode map inst[1:0]=11b, indexed using inst[6:2]
static const insn_decode_func_t functab32b[32] = {
               /*xx.000*/  /*xx.001*/    /*xx.010*/     /*xx.011*/    /*xx.100*/   /*xx.101*/     /*xx.110*/     /*xx.111*/
    /*00.xxx*/ dec_load,   dec_load_fp,  dec_custom0,   dec_misc_mem, dec_op_imm,  dec_auipc,     dec_op_imm_32, dec_insn_48b_0,
    /*01.xxx*/ dec_store,  dec_store_fp, dec_custom1,   dec_amo,      dec_op,      dec_lui,       dec_op_32,     dec_insn_64b,
    /*10.xxx*/ dec_madd,   dec_msub,     dec_nmsub,     dec_nmadd,    dec_op_fp,   dec_reserved0, dec_custom2,   dec_insn_48b_1,
    /*11.xxx*/ dec_branch, dec_jalr,     dec_reserved1, dec_jal,      dec_system,  dec_reserved2, dec_custom3,   dec_insn_80b
};


// Opcode map inst[1:0]={00b,01b,10b}, indexed using inst[15:13,1:0]
static const insn_decode_func_t functab16b[32] = {
                /*xxx.00*/      /*xxx.01*/          /*xxx.10*/         /*xxx.11*/
    /*000.xx*/  dec_c_addi4spn, dec_c_addi,         dec_c_slli,        nullptr,
    /*001.xx*/  dec_c_fld,      dec_c_addiw,        dec_c_fldsp,       nullptr,
    /*010.xx*/  dec_c_lw,       dec_c_li,           dec_c_lwsp,        nullptr,
    /*011.xx*/  dec_c_ld,       dec_c_lui_addi16sp, dec_c_ldsp,        nullptr,
    /*100.xx*/  dec_c_reserved, dec_c_misc_alu,     dec_c_jalr_mv_add, nullptr,
    /*101.xx*/  dec_c_fsd,      dec_c_j,            dec_c_fsdsp,       nullptr,
    /*110.xx*/  dec_c_sw,       dec_c_beqz,         dec_c_swsp,        nullptr,
    /*111.xx*/  dec_c_sd,       dec_c_bnez,         dec_c_sdsp,        nullptr
};


// FIXME: we need a better place to put this code, but it uses all these
// decode tables only visible to this file...
uintptr_t decode(uint32_t bits)
{
    Instruction inst = { bits, 0 };
    insn_exec_funct_t exec_fn = nullptr;

    if ((inst.bits & 0x3) == 0x3) {
        int idx = ((inst.bits >> 2) & 0x1f);
        exec_fn = functab32b[idx](inst.bits, inst.flags);
    } else {
        int idx = ((inst.bits >> 11) & 0x1c) | (inst.bits & 0x03);
        exec_fn = functab16b[idx](inst.bits, inst.flags);
    }
    return reinterpret_cast<uintptr_t>(exec_fn);
}


void Hart::execute()
{
    // Decode the fetched bits
    insn_exec_funct_t exec_fn = nullptr;
    if ((inst.bits & 0x3) == 0x3) {
        int idx = ((inst.bits >> 2) & 0x1f);
        exec_fn = functab32b[idx](inst.bits, inst.flags);
        npc = sextVA(pc + 4);
    } else {
        int idx = ((inst.bits >> 11) & 0x1c) | (inst.bits & 0x03);
        exec_fn = functab16b[idx](inst.bits, inst.flags);
        npc = sextVA(pc + 2);
    }
    if ((minstmask >> 32) != 0) {
        if (((inst.bits ^ minstmatch) & uint32_t(minstmask)) == 0)
            throw trap_mcode_instruction(inst.bits);
    }
    (exec_fn)(*this);
}


// -----------------------------------------------------------------------------
//
// Trap execution
//
// -----------------------------------------------------------------------------

static void trap_to_smode(Hart& cpu, uint64_t cause, uint64_t val)
{
    bool interrupt = (cause & 0x8000000000000000ULL);
    int code = (cause & 63);
    assert(cpu.prv <= Privilege::S);

#ifdef SYS_EMU
    if (cpu.chip->emu()->get_display_trap_info()) {
        LOG_HART(INFO, cpu, "\tTrapping to S-mode with cause 0x%" PRIx64 " and tval 0x%" PRIx64, cause, val);
    } else
#endif
    {
        LOG_HART(DEBUG, cpu, "\tTrapping to S-mode with cause 0x%" PRIx64 " and tval 0x%" PRIx64, cause, val);
    }

    // If checking against RTL, after clearing, the correspoding MIP bit will
    // be set again to 1 if the pending bit was not really cleared in the RTL
    // just before entering the interrupt again.
    //
    // TODO: you won't be able to read the MIP CSR from code or you'll get
    // checker errors (you would have errors if the interrupt is cleared by a
    // memory mapped store).
#ifndef SYS_EMU
    if (interrupt) {
        if (code == SUPERVISOR_EXTERNAL_INTERRUPT) {
            // Clear external supervisor interrupt
            if (cpu.ext_seip & (1ull << code)) {
                LOG_HART(DEBUG, cpu, "%s", "\tClearing external supervisor interrupt");
            }
            cpu.ext_seip &= ~(1ull << code);
        } else {
            cpu.mip &= ~(1ull << code);
        }
    }
#endif

    // Take sie
    uint64_t mstatus = cpu.mstatus;
    uint64_t sie = (mstatus >> 1) & 0x1;
    // Set spie = sie, sie = 0, spp = prv
    cpu.mstatus = (mstatus & 0xFFFFFFFFFFFFFEDDULL) | (static_cast<uint64_t>(cpu.prv) << 8) | (sie << 5);
    // Set scause, stval and sepc
    cpu.scause = cause & 0x800000000000001FULL;
    cpu.stval = sextVA(val);
    cpu.sepc = sextVA(cpu.pc & ~1ULL);
    // Jump to stvec
    cpu.set_prv(Privilege::S);

    // compute address where to jump to
    uint64_t tvec = cpu.stvec;
    if ((tvec & 1) && interrupt) {
        tvec += code * 4;
    }
    tvec &= ~0x1ULL;
    WRITE_PC(tvec);

    notify_trap(cpu, cpu.mstatus, cpu.scause, cpu.stval, cpu.sepc);
}


static void trap_to_mmode(Hart& cpu, uint64_t cause, uint64_t val)
{
    bool interrupt = (cause & 0x8000000000000000ULL);
    int code = (cause & 63);

    // Check if we should deletegate the trap to S-mode
    if ((cpu.prv < Privilege::M) && ((interrupt ? cpu.mideleg : cpu.medeleg) & (1ull<<code))) {
        trap_to_smode(cpu, cause, val);
        return;
    }

    // if checking against RTL, clear the correspoding MIP bit it will be set
    // to 1 again if the pending bit was not really cleared just before
    // entering the interrupt again
    // TODO: you won't be able to read the MIP CSR from code or you'll get
    // checker errors (you would have errors if the interrupt is cleared by a
    // memory mapped store)
#ifndef SYS_EMU
    if (interrupt) {
        if (code == SUPERVISOR_EXTERNAL_INTERRUPT) {
            // Clear external supervisor interrupt
            if (cpu.ext_seip & (1ull << code)) {
                LOG_HART(DEBUG, cpu, "%s", "\tClearing external supervisor interrupt");
            }
            cpu.ext_seip &= ~(1ull << code);
        } else {
            cpu.mip &= ~(1ull << code);
        }
    }
#endif

#ifdef SYS_EMU
    if (cpu.chip->emu()->get_display_trap_info()) {
        LOG_HART(INFO, cpu, "\tTrapping to M-mode with cause 0x%" PRIx64 " and tval 0x%" PRIx64, cause, val);
    } else
#endif
    {
        LOG_HART(DEBUG, cpu, "\tTrapping to M-mode with cause 0x%" PRIx64 " and tval 0x%" PRIx64, cause, val);
    }

    // Take mie
    uint64_t mstatus = cpu.mstatus;
    uint64_t mie = (mstatus >> 3) & 0x1;
    // Set mpie = mie, mie = 0, mpp = prv
    cpu.mstatus = (mstatus & 0xFFFFFFFFFFFFE777ULL) | (static_cast<uint64_t>(cpu.prv) << 11) | (mie << 7);
    // Set mcause, mtval and mepc
    cpu.mcause = cause & 0x800000000000001FULL;
    cpu.mtval = sextVA(val);
    cpu.mepc = sextVA(cpu.pc & ~1ULL);
    // Jump to mtvec
    cpu.set_prv(Privilege::M);

    // compute address where to jump to
    uint64_t tvec = cpu.mtvec;
    if ((tvec & 1) && interrupt) {
        tvec += code * 4;
    }
    tvec &= ~0x1ULL;
    WRITE_PC(tvec);

    notify_trap(cpu, cpu.mstatus, cpu.mcause, cpu.mtval, cpu.mepc);
}


void Hart::take_trap(const Trap& t)
{
    // Invalidate the fetch buffer when changing VM mode or permissions
    fetch_pc = -1;

    trap_to_mmode(*this, t.cause(), t.tval());
}


void Hart::raise_interrupt(int cause, uint64_t data)
{
    switch (cause) {
    case USER_SOFTWARE_INTERRUPT:
        LOG_HART(DEBUG, *this, "%s", "Raising user software interrupt");
        break;
    case SUPERVISOR_SOFTWARE_INTERRUPT:
        LOG_HART(DEBUG, *this, "%s", "Raising supervisor software interrupt");
        break;
    case MACHINE_SOFTWARE_INTERRUPT:
        LOG_HART(DEBUG, *this, "%s", "Raising machine software interrupt");
        break;
    case USER_TIMER_INTERRUPT:
        LOG_HART(DEBUG, *this, "%s", "Raising user timer interrupt");
        break;
    case SUPERVISOR_TIMER_INTERRUPT:
        LOG_HART(DEBUG, *this, "%s", "Raising supervisor timer interrupt");
        break;
    case MACHINE_TIMER_INTERRUPT:
        LOG_HART(DEBUG, *this, "%s", "Raising machine timer interrupt");
        break;
    case USER_EXTERNAL_INTERRUPT:
        LOG_HART(DEBUG, *this, "%s", "Raising user external interrupt");
        break;
    case SUPERVISOR_EXTERNAL_INTERRUPT:
        LOG_HART(DEBUG, *this, "%s", "Raising supervisor external interrupt");
        break;
    case MACHINE_EXTERNAL_INTERRUPT:
        LOG_HART(DEBUG, *this, "%s", "Raising machine external interrupt");
        break;
    case BAD_IPI_REDIRECT_INTERRUPT:
        LOG_HART(DEBUG, *this, "%s", "Raising bad IPI redirect interrupt");
        break;
    case ICACHE_ECC_COUNTER_OVERFLOW_INTERRUPT:
        LOG_HART(DEBUG, *this, "%s", "Raising instruction ECC counter overflow interrupt");
        break;
    case BUS_ERROR_INTERRUPT:
        LOG_HART(DEBUG, *this, "%s", "Raising bus error interrupt");
        break;
    default:
        LOG_HART(DEBUG, *this, "Raising unknown interrupt (cause = %d)", cause);
        break;
    }

    if (cause == SUPERVISOR_EXTERNAL_INTERRUPT) {
        ext_seip |= (1ULL << cause);
    } else {
        mip |= (1ULL << cause);
        if (cause == BUS_ERROR_INTERRUPT) {
            mbusaddr = zextPA(data);
        }
    }

    // If there are no locally enabled pending interrupts, or the hart is in
    // exclusive mode, then the hart should not react to the interrupt.
    uint64_t xip = (mip | ext_seip) & mie;
    if (!xip || core->excl_mode) {
        return;
    }

    stop_waiting(Waiting::interrupt);
}


void Hart::clear_interrupt(int cause)
{
    switch (cause) {
    case USER_SOFTWARE_INTERRUPT:
        LOG_HART(DEBUG, *this, "%s", "Clearing user software interrupt");
        break;
    case SUPERVISOR_SOFTWARE_INTERRUPT:
        LOG_HART(DEBUG, *this, "%s", "Clearing supervisor software interrupt");
        break;
    case MACHINE_SOFTWARE_INTERRUPT:
        LOG_HART(DEBUG, *this, "%s", "Clearing machine software interrupt");
        break;
    case USER_TIMER_INTERRUPT:
        LOG_HART(DEBUG, *this, "%s", "Clearing user timer interrupt");
        break;
    case SUPERVISOR_TIMER_INTERRUPT:
        LOG_HART(DEBUG, *this, "%s", "Clearing supervisor timer interrupt");
        break;
    case MACHINE_TIMER_INTERRUPT:
        LOG_HART(DEBUG, *this, "%s", "Clearing machine timer interrupt");
        break;
    case USER_EXTERNAL_INTERRUPT:
        LOG_HART(DEBUG, *this, "%s", "Clearing user external interrupt");
        break;
    case SUPERVISOR_EXTERNAL_INTERRUPT:
        LOG_HART(DEBUG, *this, "%s", "Clearing supervisor external interrupt");
        break;
    case MACHINE_EXTERNAL_INTERRUPT:
        LOG_HART(DEBUG, *this, "%s", "Clearing machine external interrupt");
        break;
    case BAD_IPI_REDIRECT_INTERRUPT:
        LOG_HART(DEBUG, *this, "%s", "Clearing bad IPI redirect interrupt");
        break;
    case ICACHE_ECC_COUNTER_OVERFLOW_INTERRUPT:
        LOG_HART(DEBUG, *this, "%s", "Clearing instruction ECC counter overflow interrupt");
        break;
    case BUS_ERROR_INTERRUPT:
        LOG_HART(DEBUG, *this, "%s", "Clearing bus error interrupt");
        break;
    default:
        LOG_HART(DEBUG, *this, "Clearing unknown interrupt (cause = %d)", cause);
        break;
    }

    if (cause == SUPERVISOR_EXTERNAL_INTERRUPT) {
        ext_seip &= ~(1ULL << cause);
    } else {
        mip &= ~(1ULL << cause);
    }
}


// -----------------------------------------------------------------------------
//
// Performance monitoring
//
// -----------------------------------------------------------------------------

void Hart::notify_pmu_minion_event(uint8_t event)
{
    // The first four counters count Minion-related events
    for (int i = 0; i < 4; i++) {
        if (chip->neigh_pmu_events[neigh_index(*this)][i][index_in_core(*this)] == event) {
            ++chip->neigh_pmu_counters[neigh_index(*this)][i][index_in_core(*this)];
        }
    }
}


// -----------------------------------------------------------------------------
//
// Manage hart state
//
// -----------------------------------------------------------------------------

void Hart::become_nonexistent()
{
    if (index_in_core(*this) == 0) {
        core->tload_a[0].clear();
        core->tload_a[1].clear();
        core->tload_b.clear();
        core->tmul.state = TMul::State::idle;
        core->tquant.state = TQuant::State::idle;
        core->reduce.state = TReduce::State::idle;
        core->reduce.hart = this;
        core->tqueue.clear();
    }
    waits = Waiting::none;
    state = State::nonexistent;
    links.unlink();
}


void Hart::become_unavailable()
{
    if (is_active() || is_sleeping()) {
        LOG_HART(DEBUG, *this, "%s", "Become unavailable");
    }
    if (index_in_core(*this) == 0) {
        if (has_active_coprocessor()) {
            WARN_HART(tensors, *this, "%s",
                     "Stopping a hart with an active coprocessor!");
        }
        core->tload_a[0].clear();
        core->tload_a[1].clear();
        core->tload_b.clear();
        core->tmul.state = TMul::State::idle;
        core->tquant.state = TQuant::State::idle;
        core->reduce.state = TReduce::State::idle;
        core->reduce.hart = this;
        core->tqueue.clear();
    }
    waits = Waiting::none;
    state = State::unavailable;
    links.unlink();
}


void Hart::start_running()
{
    if (is_running()) {
        return;
    }
    LOG_HART(DEBUG, *this, "%s", "Start running");
    clear_hastatus0(*this, hastatus0_halted);
    set_hastatus0(*this, hastatus0_running);
    // If we are resuming from debug mode, update PC and privilege
    if (debug_mode) {
        set_hastatus0(*this, hastatus0_resumeack);
        pc = dpc;
        set_prv(get_privilege(dcsr));
    }
    // FIXME(cabul): If the hart was executing a tensor_wait while being halted,
    // we should resume execution of the tensor_wait when it is resumed.
    debug_mode = false;
    if (is_active() || is_sleeping()) {
        return;
    }
    if (!is_waiting() || has_active_coprocessor()) {
        chip->awaking.push_back(*this);
        state = State::active;
    } else {
        chip->sleeping.push_back(*this);
        state = State::sleeping;
    }
}


void Hart::enter_debug_mode(Debug_entry::Cause cause)
{
    if (is_halted()) {
        return;
    }
    LOG_HART(DEBUG, *this, "Halt execution: %s", debug_cause(cause));
    debug_mode = true;
    dpc = npc;
    dcsr = (static_cast<uint32_t>(prv))
         | (static_cast<uint32_t>(cause) << 6)
         | (0x4 << 28); // xdebugver: tied to 0x4
    set_hastatus0(*this, hastatus0_halted);
    clear_hastatus0(*this, hastatus0_running);
    prv = Privilege::M; // NB: This does not activate breakpoints
    reset_progbuf();
    // FIXME(cabul): We should also unblock pending tensor_waits here
    stop_waiting(Waiting::interrupt); // halt is an interrupt
    if (is_active() || is_sleeping()) {
        return;
    }
    if (!is_waiting() || has_active_coprocessor()) {
        chip->awaking.push_back(*this);
        state = State::active;
    } else {
        chip->sleeping.push_back(*this);
        state = State::sleeping;
    }
}


void Hart::start_waiting(Waiting what)
{
    assert(is_active() || is_sleeping());
    if ((what == Waiting::none) || is_waiting(what)) {
        return;
    }
    switch (what) {
    case Waiting::none:
        /* do nothing */
        break;
    case Waiting::tload_0:
        LOG_HART(DEBUG, *this, "%s", "\tStart waiting for TensorLoad(id=0)");
        break;
    case Waiting::tload_1:
        LOG_HART(DEBUG, *this, "%s", "\tStart waiting for TensorLoad(id=1)");
        break;
    case Waiting::tload_L2_0:
        LOG_HART(DEBUG, *this, "%s", "\tStart waiting for TensorLoadL2(id=0)");
        break;
    case Waiting::tload_L2_1:
        LOG_HART(DEBUG, *this, "%s", "\tStart waiting for TensorLoadL2(id=1)");
        break;
    case Waiting::prefetch_0:
        LOG_HART(DEBUG, *this, "%s", "\tStart waiting for Prefetch(id=0)");
        break;
    case Waiting::prefetch_1:
        LOG_HART(DEBUG, *this, "%s", "\tStart waiting for Prefetch(id=1)");
        break;
    case Waiting::cacheop:
        LOG_HART(DEBUG, *this, "%s", "\tStart waiting for CacheOp");
        break;
    case Waiting::tfma:
        LOG_HART(DEBUG, *this, "%s", "\tStart waiting for TensorFMA");
        break;
    case Waiting::tstore:
        LOG_HART(DEBUG, *this, "%s", "\tStart waiting for TensorStore");
        break;
    case Waiting::reduce:
        LOG_HART(DEBUG, *this, "%s", "\tStart waiting for tensor reduce");
        break;
    case Waiting::tquant:
        LOG_HART(DEBUG, *this, "%s", "\tStart waiting for TensorQuant");
        break;
    case Waiting::interrupt:
        LOG_HART(DEBUG, *this, "%s", "\tStart waiting for interrupt");
        break;
    case Waiting::message:
        LOG_HART(DEBUG, *this, "%s", "\tStart waiting for message");
        break;
    case Waiting::credit0:
        LOG_HART(DEBUG, *this, "%s", "\tStart waiting for FCC0");
        break;
    case Waiting::credit1:
        LOG_HART(DEBUG, *this, "%s", "\tStart waiting for FCC1");
        break;
    case Waiting::tload_tenb:
        LOG_HART(DEBUG, *this, "%s", "\tStart waiting for TensorLoad to TenB");
        break;
    }
    waits |= what;
    maybe_sleep();
}


void Hart::stop_waiting(Waiting what)
{
    if ((what == Waiting::none) || !is_waiting(what)) {
        return;
    }
    switch (what) {
    case Waiting::none:
        /* do nothing */
        break;
    case Waiting::tload_0:
        LOG_HART(DEBUG, *this, "%s", "\tStop waiting for TensorLoad(id=0)");
        break;
    case Waiting::tload_1:
        LOG_HART(DEBUG, *this, "%s", "\tStop waiting for TensorLoad(id=1)");
        break;
    case Waiting::tload_L2_0:
        LOG_HART(DEBUG, *this, "%s", "\tStop waiting for TensorLoadL2(id=0)");
        break;
    case Waiting::tload_L2_1:
        LOG_HART(DEBUG, *this, "%s", "\tStop waiting for TensorLoadL2(id=1)");
        break;
    case Waiting::prefetch_0:
        LOG_HART(DEBUG, *this, "%s", "\tStop waiting for Prefetch(id=0)");
        break;
    case Waiting::prefetch_1:
        LOG_HART(DEBUG, *this, "%s", "\tStop waiting for Prefetch(id=1)");
        break;
    case Waiting::cacheop:
        LOG_HART(DEBUG, *this, "%s", "\tStop waiting for CacheOp");
        break;
    case Waiting::tfma:
        LOG_HART(DEBUG, *this, "%s", "\tStop waiting for TensorFMA");
        break;
    case Waiting::tstore:
        LOG_HART(DEBUG, *this, "%s", "\tStop waiting for TensorStore");
        break;
    case Waiting::reduce:
        LOG_HART(DEBUG, *this, "%s", "\tStop waiting for tensor reduce");
        break;
    case Waiting::tquant:
        LOG_HART(DEBUG, *this, "%s", "\tStop waiting for TensorQuant");
        break;
    case Waiting::interrupt:
        LOG_HART(DEBUG, *this, "%s", "\tStop waiting for interrupt");
        break;
    case Waiting::message:
        LOG_HART(DEBUG, *this, "%s", "\tStop waiting for message");
        break;
    case Waiting::credit0:
        LOG_HART(DEBUG, *this, "%s", "\tStop waiting for FCC0");
        break;
    case Waiting::credit1:
        LOG_HART(DEBUG, *this, "%s", "\tStop waiting for FCC1");
        break;
    case Waiting::tload_tenb:
        LOG_HART(DEBUG, *this, "%s", "\tStop waiting for TensorLoad to TenB");
        break;
    }

    // Finish executing any outstanding TensorWait
    if ((twait & what) != Waiting::none) {
        tensor_wait_execute(*this, what);
    }

    waits &= ~what;
    maybe_wakeup();
}


void Hart::maybe_sleep()
{
    if (!is_sleeping() && is_waiting() && !has_active_coprocessor()) {
        assert(is_active());
        chip->sleeping.push_back(*this);
        state = State::sleeping;
        LOG_HART(DEBUG, *this, "%s", "Going to sleep");
    }
}


void Hart::maybe_wakeup()
{
    if (!is_active() && (!is_waiting() || has_active_coprocessor())) {
        chip->awaking.push_back(*this);
        state = State::active;
        LOG_HART(DEBUG, *this, "%s", "Waking up");
    }
}


void Hart::debug_reset()
{
    // Exit and clear the program buffer
    if (in_progbuf()) {
        exit_progbuf(Progbuf::error);
    }
    reset_progbuf();
}


void Hart::warm_reset()
{
    // Become unavailable
    if (state != State::nonexistent) {
        state = State::unavailable;
        waits = Hart::Waiting::none;
        links.unlink();
    }

    // Check if in program buffer
    if (in_progbuf()) {
        exit_progbuf(Progbuf::error);
    }

    // Register files
    xregs[x0] = 0;

    // PC
    pc = chip->neigh_esrs[neigh_index(*this)].minion_boot;
    npc = pc;

    // Currently executing instruction
    inst = Instruction { 0, 0 };

    // Fetch buffer
    fetch_pc = -1;

    // RISCV control and status registers
    scounteren = 0;
    mstatus = 0x0000000A00001800ULL; // mpp=11, sxl=uxl=10
    medeleg = 0;
    mideleg = 0;
    mie = 0;
    mcounteren = 0;

    mcause = 0;
    mip = 0;
    tdata1 = 0x20C0000000000000ULL;
    dcsr = 0x40000003ULL;

    // Esperanto control and status registers
    ddata0 = 0;
    minstmask = 0;
    minstmatch = 0;
    // TODO: amofence_ctrl <= ...
    gsc_progress = 0;
    fcc[0] = 0;
    fcc[1] = 0;

    // PMU events
    System::neigh_pmu_events_t& events = chip->neigh_pmu_events[neigh_index(*this)];
    for (int counter = 0; counter < 6; ++counter) {
        events[counter][index_in_neigh(*this)] = PMU_MINION_EVENT_NONE;
    }

    // Port control
    for (unsigned p = 0; p < portctrl.size(); ++p) {
        configure_port(*this, p, 0);
    }

    // Other hart internal (microarchitectural or hidden) state
    prv = Privilege::M;
    debug_mode = false;
    ext_seip = 0;

    // Pre-computed state to improve simulation speed
    break_on_load = false;
    break_on_store = false;
    break_on_fetch = false;

    // Reset core-shared state
    if (index_in_core(*this) == 0) {
        core->matp = 0;
        core->menable_shadows = 0;
        core->excl_mode = 0;
        core->mcache_control = 0;
        core->ucache_control = 0x200;
        for (auto& set : core->scp_lock) {
            set.fill(false);
        }
        core->tload_a[0].clear();
        core->tload_a[1].clear();
        core->tload_b.clear();
        core->tmul.state = TMul::State::idle;
        core->tquant.state = TQuant::State::idle;
        core->reduce.state = TReduce::State::idle;
        core->reduce.hart = this;
        core->tqueue.clear();
    }

    clear_hastatus0(*this, hastatus0_halted);
    clear_hastatus0(*this, hastatus0_running);
    set_hastatus0(*this, hastatus0_havereset);
}


void Hart::reset_progbuf()
{
    static constexpr uint32_t ebreak = 0x100073;
    std::fill(progbuf.begin(), progbuf.end(), ebreak);
}


bool Hart::in_progbuf() const
{
    return pc >= PROGBUF_START && pc < PROGBUF_END;
}


void Hart::fetch_progbuf()
{
    assert(in_progbuf());
    inst.bits  = progbuf[(pc - PROGBUF_START) / 4];
    inst.flags = 0;
}


void Hart::advance_progbuf()
{
    if (!in_progbuf()) {
        return;
    }
    advance_pc();
    if (!in_progbuf()) {
        exit_progbuf(Progbuf::ok);
    }
}


void Hart::enter_progbuf()
{
    LOG_HART(DEBUG, *this, "%s", "Enter program buffer");
    pc = PROGBUF_START;
    chip->neigh_esrs[neigh_index(*this)].hastatus1 |= (1ull << index_in_neigh(*this));
}


void Hart::exit_progbuf(Progbuf status)
{
    assert(in_progbuf());
    pc = PROGBUF_END;
    npc = dpc;
    const uint64_t mask = 1ull << index_in_neigh(*this);
    auto& hastatus1 = chip->neigh_esrs[neigh_index(*this)].hastatus1;
    switch (status) {
    case Progbuf::ok:
        LOG_HART(DEBUG, *this, "%s", "Exit program buffer");
        break;
    case Progbuf::exception:
        LOG_HART(DEBUG, *this, "%s", "Exit program buffer (exception)");
        hastatus1 |= mask << 16;
        break;
    case Progbuf::error:
        LOG_HART(DEBUG, *this, "%s", "Exit program buffer (error)");
        hastatus1 |= mask << 32;
        break;
    default:
        assert(0 && "Unreachable");
    }
    chip->neigh_esrs[neigh_index(*this)].hastatus1 &= ~mask;
}


void Hart::write_progbuf(uint64_t addr, uint64_t value)
{
    assert(!in_progbuf());
    switch (addr) {
    case ESR_ABSCMD:
        progbuf[0] = value & 0xFFFFFFFF;
        progbuf[1] = value >> 32;
        break;
    case ESR_NXPROGBUF0:
    case ESR_AXPROGBUF0:
        progbuf[2] = value;
        break;
    case ESR_NXPROGBUF1:
    case ESR_AXPROGBUF1:
        progbuf[3] = value;
        break;
    default:
        assert(0 && "Invalid ESR for program buffer");
    }
}


} // namespace bemu
