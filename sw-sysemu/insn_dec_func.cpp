#include "insn.h"
#include "insn_exec_func.h"
#include "emu_gio.h"

// program state
extern uint32_t current_inst;

// memory emulation
extern uint64_t (*vmemtranslate) (uint64_t, mem_access_type);
extern uint16_t (*pmemfetch16) (uint64_t);

// re-define the type
typedef void (*insn_exec_funct_t)(insn_t);

typedef insn_exec_funct_t (*insn_decode_func_t)(uint32_t, uint32_t&);


// -----------------------------------------------------------------------------
//
// 32-bit instruction decoding
//
// -----------------------------------------------------------------------------

static insn_exec_funct_t dec_load(uint32_t bits, uint32_t& flags)
{
    static const insn_exec_funct_t functab[8] = {
        /* 000 */ insn_lb,
        /* 001 */ insn_lh,
        /* 010 */ insn_lw,
        /* 011 */ insn_ld,
        /* 100 */ insn_lbu,
        /* 101 */ insn_lhu,
        /* 110 */ insn_lwu,
        /* 111 */ insn_unknown
    };
    unsigned funct3 = (bits >> 12) & 7;
    flags |= insn_t::flag_LOAD;
    return functab[funct3];
}


static insn_exec_funct_t dec_load_fp(uint32_t bits, uint32_t& flags)
{
    unsigned funct3 = (bits >> 12) & 7;
    switch (funct3) {
    case 0x2: flags |= insn_t::flag_LOAD; return insn_flw;
    case 0x5: flags |= insn_t::flag_LOAD; return insn_flq2;
    default : return insn_unknown;
    }
}


static insn_exec_funct_t dec_custom0(uint32_t bits, uint32_t& flags __attribute__((unused)))
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
              default  : return insn_unknown;
              }
    case 0x2: return insn_flw_ps;
    case 0x3: return insn_fbcx_ps;
    case 0x4: switch (funct7) {
              case 0x03: return insn_famoaddl_pi;
              case 0x07: return insn_famoswapl_pi;
              case 0x0b: return insn_famoandl_pi;
              case 0x0f: return insn_famoorl_pi;
              case 0x13: return insn_famoxorl_pi;
              case 0x14: return insn_famominl_ps;
              case 0x17: return insn_famominl_pi;
              case 0x18: return insn_famomaxl_ps;
              case 0x1b: return insn_famomaxl_pi;
              case 0x1f: return insn_famominul_pi;
              case 0x23: return insn_famomaxul_pi;
              case 0x43: return insn_famoaddg_pi;
              case 0x47: return insn_famoswapg_pi;
              case 0x4b: return insn_famoandg_pi;
              case 0x4f: return insn_famoorg_pi;
              case 0x53: return insn_famoxorg_pi;
              case 0x54: return insn_famoming_ps;
              case 0x57: return insn_famoming_pi;
              case 0x58: return insn_famomaxg_ps;
              case 0x5b: return insn_famomaxg_pi;
              case 0x5f: return insn_famominug_pi;
              case 0x63: return insn_famomaxug_pi;
              default  : return insn_unknown;
              }
    case 0x6: return insn_fsw_ps;
    case 0x7: switch (funct7) {
              case 0x08: return (bits & 0x01f00000) ? insn_unknown : insn_flwl_ps; // rs2==0
              case 0x09: return (bits & 0x01f00000) ? insn_unknown : insn_flwg_ps; // rs2==0
              case 0x28: return (bits & 0x01f00000) ? insn_unknown : insn_fswl_ps; // rs2==0
              case 0x29: return (bits & 0x01f00000) ? insn_unknown : insn_fswg_ps; // rs2==0
              case 0x48: return insn_fgwl_ps;
              case 0x49: return insn_fgwg_ps;
              case 0x68: return insn_fscwl_ps;
              case 0x69: return insn_fscwg_ps;
              default  : return insn_unknown;
              }
    default : return insn_unknown;
    }
}


static insn_exec_funct_t dec_misc_mem(uint32_t bits, uint32_t& flags __attribute__((unused)))
{
    unsigned funct3 = (bits >> 12) & 7;
    unsigned fm = (bits >> 28);
    switch (funct3) {
    case 0x0: return (fm == 8) ? insn_fence_tso : insn_fence;
    case 0x1: return insn_fence_i;
    default : return insn_unknown;
    }
}


static insn_exec_funct_t dec_op_imm(uint32_t bits, uint32_t& flags __attribute__((unused)))
{
    unsigned funct3 = (bits >> 12) & 7;
    unsigned funct6 = (bits >> 26);
    switch (funct3) {
    case 0x0: return insn_addi;
    case 0x1: return (funct6 == 0) ? insn_slli : insn_unknown;
    case 0x2: return insn_slti;
    case 0x3: return insn_sltiu;
    case 0x4: return insn_xori;
    case 0x5: switch (funct6) {
              case 0x00: return insn_srli;
              case 0x10: return insn_srai;
              default  : return insn_unknown;
              }
    case 0x6: return insn_ori;
    case 0x7: return insn_andi;
    default : return insn_unknown;
    }
}


static insn_exec_funct_t dec_auipc(uint32_t bits __attribute__((unused)),
                                   uint32_t& flags __attribute__((unused)))
{
    return insn_auipc;
}


static insn_exec_funct_t dec_op_imm_32(uint32_t bits, uint32_t& flags __attribute__((unused)))
{
    unsigned funct3 = (bits >> 12) & 7;
    unsigned funct7 = (bits >> 25);
    switch (funct3) {
    case 0x0: return insn_addiw;
    case 0x1: return (funct7 == 0) ? insn_slliw : insn_unknown;
    case 0x5: switch (funct7) {
              case 0x00: return insn_srliw;
              case 0x20: return insn_sraiw;
              default  : return insn_unknown;
              }
    default : return insn_unknown;
    }
}


static insn_exec_funct_t dec_insn_48b_0(uint32_t bits __attribute__((unused)),
                                        uint32_t& flags __attribute__((unused)))
{
    return insn_fbci_ps;
}


static insn_exec_funct_t dec_store(uint32_t bits, uint32_t& flags __attribute__((unused)))
{
    static const insn_exec_funct_t functab[8] = {
        /* 000 */ insn_sb,
        /* 001 */ insn_sh,
        /* 010 */ insn_sw,
        /* 011 */ insn_sd,
        /* 100 */ insn_unknown,
        /* 101 */ insn_unknown,
        /* 110 */ insn_unknown,
        /* 111 */ insn_unknown
    };
    unsigned funct3 = (bits >> 12) & 7;
    return functab[funct3];
}


static insn_exec_funct_t dec_store_fp(uint32_t bits, uint32_t& flags __attribute__((unused)))
{
    unsigned funct3 = (bits >> 12) & 7;
    switch (funct3) {
    case 0x2: return insn_fsw;
    case 0x5: return insn_fsq2;
    default : return insn_unknown;
    }
}


static insn_exec_funct_t dec_custom1(uint32_t bits __attribute__((unused)),
                                     uint32_t& flags __attribute__((unused)))
{
    return insn_unknown;
}


static insn_exec_funct_t dec_amo(uint32_t bits, uint32_t& flags)
{
#if 0
    unsigned funct3 = (bits >> 12) & 7;
    unsigned funct5 = (bits >> 27);
    switch (funct3) {
    case 0x2: switch (funct5) {
              case 0x00: flags |= insn_t::flag_AMO; return insn_amoadd_w;
              case 0x01: flags |= insn_t::flag_AMO; return insn_amoswap_w;
              case 0x02: flags |= insn_t::flag_AMO; return insn_lr_w;
              case 0x03: flags |= insn_t::flag_AMO; return insn_sc_w;
              case 0x04: flags |= insn_t::flag_AMO; return insn_amoxor_w;
              case 0x08: flags |= insn_t::flag_AMO; return insn_amoor_w;
              case 0x0c: flags |= insn_t::flag_AMO; return insn_amoand_w;
              case 0x10: flags |= insn_t::flag_AMO; return insn_amomin_w;
              case 0x14: flags |= insn_t::flag_AMO; return insn_amomax_w;
              case 0x18: flags |= insn_t::flag_AMO; return insn_amominu_w;
              case 0x1c: flags |= insn_t::flag_AMO; return insn_amomaxu_w;
              default  : return insn_unknown;
              }
    case 0x3: switch (funct5) {
              case 0x00: flags |= insn_t::flag_AMO; return insn_amoadd_d;
              case 0x01: flags |= insn_t::flag_AMO; return insn_amoswap_d;
              case 0x02: flags |= insn_t::flag_AMO; return insn_lr_d;
              case 0x03: flags |= insn_t::flag_AMO; return insn_sc_d;
              case 0x04: flags |= insn_t::flag_AMO; return insn_amoxor_d;
              case 0x08: flags |= insn_t::flag_AMO; return insn_amoor_d;
              case 0x0c: flags |= insn_t::flag_AMO; return insn_amoand_d;
              case 0x10: flags |= insn_t::flag_AMO; return insn_amomin_d;
              case 0x14: flags |= insn_t::flag_AMO; return insn_amomax_d;
              case 0x18: flags |= insn_t::flag_AMO; return insn_amominu_d;
              case 0x1c: flags |= insn_t::flag_AMO; return insn_amomaxu_d;
              default  : return insn_unknown;
              }
    default : return insn_unknown;
    }
#else
    return insn_unknown;
#endif
}


static insn_exec_funct_t dec_op(uint32_t bits, uint32_t& flags __attribute__((unused)))
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
        /* 001 */ insn_unknown,
        /* 010 */ insn_unknown,
        /* 011 */ insn_unknown,
        /* 100 */ insn_unknown,
        /* 101 */ insn_sra,
        /* 110 */ insn_unknown,
        /* 111 */ insn_unknown
    };
    unsigned funct3 = (bits >> 12) & 7;
    unsigned funct7 = (bits >> 25);
    switch (funct7) {
    case 0x00: return functab00[funct3];
    case 0x01: return functab01[funct3];
    case 0x20: return functab20[funct3];
    default  : return insn_unknown;
    }
}


static insn_exec_funct_t dec_lui(uint32_t bits __attribute__((unused)),
                                 uint32_t& flags __attribute__((unused)))
{
    return insn_lui;
}


static insn_exec_funct_t dec_op_32(uint32_t bits, uint32_t& flags __attribute__((unused)))
{
#if 0
    static const insn_exec_funct_t functab00[8] = {
        /* 000 */ insn_addw,
        /* 001 */ insn_sllw,
        /* 010 */ insn_unknown,
        /* 011 */ insn_unknown,
        /* 100 */ insn_unknown,
        /* 101 */ insn_srlw,
        /* 110 */ insn_unknown,
        /* 111 */ insn_unknown
    };
    static const insn_exec_funct_t functab01[8] = {
        /* 000 */ insn_mulw,
        /* 001 */ insn_unknown,
        /* 010 */ insn_unknown,
        /* 011 */ insn_unknown,
        /* 100 */ insn_divw,
        /* 101 */ insn_divuw,
        /* 110 */ insn_remw,
        /* 111 */ insn_remuw
    };
    static const insn_exec_funct_t functab20[8] = {
        /* 000 */ insn_subw,
        /* 001 */ insn_unknown,
        /* 010 */ insn_unknown,
        /* 011 */ insn_unknown,
        /* 100 */ insn_unknown,
        /* 101 */ insn_sraw,
        /* 110 */ insn_unknown,
        /* 111 */ insn_unknown
    };
    unsigned funct3 = (bits >> 12) & 7;
    unsigned funct7 = (bits >> 25);
    switch (funct7) {
    case 0x00: return functab00[funct3];
    case 0x01: return functab01[funct3];
    case 0x20: return functab20[funct3];
    default  : return insn_unknown;
    }
#else
    unsigned funct3 = (bits >> 12) & 7;
    unsigned funct7 = (bits >> 25);
    switch (funct3) {
    case 0x0: switch (funct7) {
              case 0x00: return insn_addw;
              case 0x01: return insn_mulw;
              case 0x20: return insn_subw;
              default  : return insn_unknown;
              }
    case 0x1: return (funct7 == 0) ? insn_sllw : insn_unknown;
    case 0x2: switch (funct7) {
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
              default  : return insn_unknown;
              }
    case 0x3: switch (funct7) {
              case 0x00: return insn_amoaddl_d;
              case 0x01: return insn_amoaddg_d;
              case 0x04: return insn_amoswapl_d;
              case 0x05: return insn_amoswapg_d;
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
              default  : return insn_unknown;
              }
    case 0x4: return (funct7 == 0x01) ? insn_divw : insn_unknown;
    case 0x5: switch (funct7) {
              case 0x00: return insn_srlw;
              case 0x01: return insn_divuw;
              case 0x20: return insn_sraw;
              default  : return insn_unknown;
              }
    case 0x6: switch (funct7) {
              case 0x01: return insn_remw;
              case 0x40: return insn_packb;
              default  : return insn_unknown;
              }
    case 0x7: switch (funct7) {
              case 0x01: return insn_remuw;
              case 0x40: return insn_bitmixb;
              default  : return insn_unknown;
              }
    default : return insn_unknown;
    }
#endif
}


static insn_exec_funct_t dec_insn_64b(uint32_t bits, uint32_t& flags __attribute__((unused)))
{
    static const insn_exec_funct_t functab0[4] = {
        /* 00 */ insn_unknown,
        /* 01 */ insn_unknown,
        /* 10 */ insn_faddi_pi,
        /* 11 */ insn_unknown
    };
    static const insn_exec_funct_t functab1[4] = {
        /* 00 */ insn_unknown,
        /* 01 */ insn_unknown,
        /* 10 */ insn_fandi_pi,
        /* 11 */ insn_unknown
    };
    static const insn_exec_funct_t functab2[4] = {
        /* 00 */ insn_unknown,
        /* 01 */ insn_unknown,
        /* 10 */ insn_fcmov_ps,
        /* 11 */ insn_unknown
    };
    unsigned funct3 = (bits >> 12) & 7;
    unsigned fmt = (bits >> 25) & 3;
    switch (funct3) {
    case 0x0: return functab0[fmt];
    case 0x1: return functab1[fmt];
    case 0x2: return functab2[fmt];
    default : return insn_unknown;
    }
}


static insn_exec_funct_t dec_madd(uint32_t bits, uint32_t& flags __attribute__((unused)))
{
    unsigned funct2 = (bits >> 25) & 2;
    return (funct2 == 0) ? insn_fmadd_s : insn_unknown;
}


static insn_exec_funct_t dec_msub(uint32_t bits, uint32_t& flags __attribute__((unused)))
{
    unsigned funct2 = (bits >> 25) & 2;
    return (funct2 == 0) ? insn_fmsub_s : insn_unknown;
}


static insn_exec_funct_t dec_nmsub(uint32_t bits, uint32_t& flags __attribute__((unused)))
{
    unsigned funct2 = (bits >> 25) & 2;
    return (funct2 == 0) ? insn_fnmsub_s : insn_unknown;
}


static insn_exec_funct_t dec_nmadd(uint32_t bits, uint32_t& flags __attribute__((unused)))
{
    unsigned funct2 = (bits >> 25) & 2;
    return (funct2 == 0) ? insn_fnmadd_s : insn_unknown;
}


static insn_exec_funct_t dec_op_fp(uint32_t bits, uint32_t& flags __attribute__((unused)))
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
               default : return insn_unknown;
               }
    case 0x14: switch (funct3) {
               case 0x0: return insn_fmin_s;
               case 0x1: return insn_fmax_s;
               default : return insn_unknown;
               }
    case 0x2c: return (rs2 == 0) ? insn_fsqrt_s : insn_unknown;
    case 0x50: switch (funct3) {
               case 0x0: return insn_fle_s;
               case 0x1: return insn_flt_s;
               case 0x2: return insn_feq_s;
               default : return insn_unknown;
               }
    case 0x60: switch (rs2) {
               case 0x00: return insn_fcvt_w_s;
               case 0x01: return insn_fcvt_wu_s;
               case 0x02: return insn_fcvt_l_s;
               case 0x03: return insn_fcvt_lu_s;
               default  : return insn_unknown;
               }
    case 0x68: switch (rs2) {
               case 0x00: return insn_fcvt_s_w;
               case 0x01: return insn_fcvt_s_wu;
               case 0x02: return insn_fcvt_s_l;
               case 0x03: return insn_fcvt_s_lu;
               default  : return insn_unknown;
               }
    case 0x70: switch (funct3) {
               case 0x0: return (rs2 == 0) ? insn_fmv_x_w  : insn_unknown;
               case 0x1: return (rs2 == 0) ? insn_fclass_s : insn_unknown;
               default : return insn_unknown;
               }
    case 0x78: return (rs2 | funct3) ? insn_unknown : insn_fmv_w_x;
    default  : return insn_unknown;
    }
}


static insn_exec_funct_t dec_reserved0(uint32_t bits __attribute__((unused)),
                                       uint32_t& flags __attribute__((unused)))
{
    return insn_unknown;
}


static insn_exec_funct_t dec_custom2(uint32_t bits, uint32_t& flags __attribute__((unused)))
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
                                        uint32_t& flags __attribute__((unused)))
{
    return insn_fbci_pi;
}


static insn_exec_funct_t dec_branch(uint32_t bits, uint32_t& flags __attribute__((unused)))
{
    static const insn_exec_funct_t functab[8] = {
        /* 000 */ insn_beq,
        /* 001 */ insn_bne,
        /* 010 */ insn_unknown,
        /* 011 */ insn_unknown,
        /* 100 */ insn_blt,
        /* 101 */ insn_bge,
        /* 110 */ insn_bltu,
        /* 111 */ insn_bgeu
    };
    unsigned funct3 = (bits >> 12) & 7;
    return functab[funct3];
}


static insn_exec_funct_t dec_jalr(uint32_t bits __attribute__((unused)),
                                  uint32_t& flags __attribute__((unused)))
{
    return insn_jalr;
}


static insn_exec_funct_t dec_reserved1(uint32_t bits __attribute__((unused)),
                                       uint32_t& flags __attribute__((unused)))
{
    return insn_unknown;
}


static insn_exec_funct_t dec_jal(uint32_t bits __attribute__((unused)),
                                 uint32_t& flags __attribute__((unused)))
{
    return insn_jal;
}


static insn_exec_funct_t dec_system(uint32_t bits, uint32_t& flags __attribute__((unused)))
{
    static const insn_exec_funct_t functab[8] = {
        /* 000 */ nullptr,
        /* 001 */ insn_csrrw,
        /* 010 */ insn_csrrs,
        /* 011 */ insn_csrrc,
        /* 100 */ insn_unknown,
        /* 101 */ insn_csrrwi,
        /* 110 */ insn_csrrsi,
        /* 111 */ insn_csrrci
    };
    unsigned funct3 = (bits >> 12) & 7;
    uint16_t imm12 = (bits >> 20) & 0xfff;
    if (funct3 == 0) {
        uint16_t rd = (bits >> 7) & 31;
        uint16_t rs1 = (bits >> 15) & 31;
        switch (imm12) {
        case 0x000: return (rd | rs1) ? insn_unknown : insn_ecall;
        case 0x001: return (rd | rs1) ? insn_unknown : insn_ebreak;
        //case 0x002: return (rd | rs1) ? insn_unknown : insn_uret;
        case 0x102: return (rd | rs1) ? insn_unknown : insn_sret;
        case 0x302: return (rd | rs1) ? insn_unknown : insn_mret;
        case 0x105: return (rd | rs1) ? insn_unknown : insn_wfi;
        }
        if ((imm12 >= 0x120) && (imm12 <= 0x13f) && (rd == 0)) {
            return insn_sfence_vma;
        }
        return insn_unknown;
    }
    // csrr{w,s,c,wi,si,ci} instructions
    flags |= insn_t::flag_CSR_READ; // FIXME: csrrw with rd=x0 does not read the CSR!
    switch (imm12) {
    case 0x800 : flags |= insn_t::flag_REDUCE; break;
    case 0x801 : flags |= insn_t::flag_TENSOR_FMA; break;
    case 0x806 : flags |= insn_t::flag_TENSOR_QUANT; break;
    case 0x820 : flags |= insn_t::flag_FLB; break;
    case 0x821 : flags |= insn_t::flag_FCC; break;
    case 0x822 : flags |= insn_t::flag_STALL; break;
    case 0x83f : flags |= insn_t::flag_TENSOR_LOAD; break;
    }
    return functab[funct3];
}


static insn_exec_funct_t dec_reserved2(uint32_t bits, uint32_t& flags __attribute__((unused)))
{
    unsigned funct3 = (bits >> 12) & 7;
    unsigned funct7 = (bits >> 25);
    return (funct3 | funct7) ? insn_unknown : insn_fcmovm_ps;
}


static insn_exec_funct_t dec_custom3(uint32_t bits, uint32_t& flags)
{
    unsigned funct3 = (bits >> 12) & 7;
    unsigned funct7 = (bits >> 25);
    unsigned rs2 = (bits >> 20) & 31;
    switch (funct7) {
    case 0x00: return insn_fadd_ps;
    case 0x03: switch (funct3) {
               case 0x0: return insn_fadd_pi;
               case 0x1: return insn_fsll_pi;
               case 0x2: return (rs2 == 0) ? insn_fnot_pi : insn_unknown;
               case 0x3: switch (rs2) {
                         case 0x0: return insn_fsat8_pi;
                         case 0x1: return insn_fsatu8_pi;
                         default : return insn_unknown;
                         }
               case 0x4: return insn_fxor_pi;
               case 0x5: return insn_fsrl_pi;
               case 0x6: return insn_for_pi;
               case 0x7: return insn_fand_pi;
               default : return insn_unknown;
               }
    case 0x04: return insn_fsub_ps;
    case 0x07: switch (funct3) {
               case 0x0: return insn_fsub_pi;
               case 0x5: return insn_fsra_pi;
               default : return insn_unknown;
               }
    case 0x08: return insn_fmul_ps;
    case 0x0b: switch (funct3) {
               case 0x0: return insn_fmul_pi;
               case 0x1: return insn_fmulh_pi;
               case 0x2: return insn_fmulhu_pi;
               default : return insn_unknown;
               }
    case 0x0c: return insn_fdiv_ps;
    case 0x0f: switch (funct3) {
               case 0x0: return insn_fdiv_pi;
               case 0x1: return insn_fdivu_pi;
               case 0x2: return insn_frem_pi;
               case 0x3: return insn_fremu_pi;
               default : return insn_unknown;
               }
    case 0x10: switch (funct3) {
               case 0x0: return insn_fsgnj_ps;
               case 0x1: return insn_fsgnjn_ps;
               case 0x2: return insn_fsgnjx_ps;
               default : return insn_unknown;
               }
    case 0x13: switch (funct3) {
               case 0x0: return (rs2 == 0) ? insn_fpackrepb_pi : insn_unknown;
               case 0x1: return (rs2 == 0) ? insn_fpackreph_pi : insn_unknown;
               default : return insn_unknown;
               }
    case 0x14: switch (funct3) {
               case 0x0: return insn_fmin_ps;
               case 0x1: return insn_fmax_ps;
               default : return insn_unknown;
               }
    case 0x17: switch (funct3) {
               case 0x0: return insn_fmin_pi;
               case 0x1: return insn_fmax_pi;
               case 0x2: return insn_fminu_pi;
               case 0x3: return insn_fmaxu_pi;
               default : return insn_unknown;
               }
    case 0x18: return insn_frcp_fix_rast;
    case 0x1f: return (bits & 0x00007c00) ? insn_unknown : insn_fltm_pi; // funct3|rd[4:3] == 0
    case 0x27: switch (funct3) {
               case 0x1: return insn_fslli_pi;
               case 0x5: return insn_fsrli_pi;
               case 0x7: return insn_fsrai_pi;
               default : return insn_unknown;
               }
    case 0x29: return (bits & 0x01fc7000) ? insn_unknown : insn_maskpopc;  // rs2|rs1[4:3]|funct3
    case 0x2a: return (bits & 0x01fc7000) ? insn_unknown : insn_maskpopcz; // rs2|rs1[4:3]|funct3
    case 0x2b: return (bits & 0x00000c00) ? insn_unknown : insn_mov_m_x;   // rd[4:3] == 0
    case 0x2c: switch (rs2) {
               case 0x00: return (funct3 == 0) ? insn_fsqrt_ps : insn_unknown;
               case 0x01: return insn_fround_ps;
               case 0x02: return insn_ffrc_ps;
               case 0x03: flags |= insn_t::flag_1ULP; return (funct3 == 0) ? insn_flog_ps : insn_unknown;
               case 0x04: flags |= insn_t::flag_1ULP; return (funct3 == 0) ? insn_fexp_ps : insn_unknown;
               case 0x06: flags |= insn_t::flag_1ULP; return (funct3 == 0) ? insn_fsin_ps : insn_unknown;
               case 0x07: return (funct3 == 0) ? insn_frcp_ps : insn_unknown;
               case 0x08: flags |= insn_t::flag_1ULP; return (funct3 == 0) ? insn_frsq_ps : insn_unknown;
               default  : return insn_unknown;
               }
    case 0x2f: return (funct3 == 0) ? insn_maskpopc_rast : insn_unknown;
    case 0x33: switch (funct3) {
               case 0x2: return (bits & 0x01fc0c00) ? insn_unknown : insn_masknot; // rs2|rs1[4:3]|rd[4:3] == 0
               case 0x4: return (bits & 0x018c0c00) ? insn_unknown : insn_maskxor; // rs2[4:3]|rs1[4:3]|rd[4:3] == 0
               case 0x6: return (bits & 0x018c0c00) ? insn_unknown : insn_maskor;  // rs2[4:3]|rs1[4:3]|rd[4:3] == 0
               case 0x7: return (bits & 0x018c0c00) ? insn_unknown : insn_maskand; // rs2[4:3]|rs1[4:3]|rd[4:3] == 0
               default : return insn_unknown;
               }
    case 0x44: switch (funct3) {
               case 0x0: return insn_cubeface_ps;
               case 0x1: return insn_cubefaceidx_ps;
               case 0x2: return insn_cubesgnsc_ps;
               case 0x3: return insn_cubesgntc_ps;
               default : return insn_unknown;
               }
    case 0x50: switch (funct3) {
               case 0x0: return insn_fle_ps;
               case 0x1: return insn_flt_ps;
               case 0x2: return insn_feq_ps;
               case 0x4: return (bits & 0x00000c00) ? insn_unknown : insn_flem_ps; // rd[4:3] == 0
               case 0x5: return (bits & 0x00000c00) ? insn_unknown : insn_fltm_ps; // rd[4:3] == 0
               case 0x6: return (bits & 0x00000c00) ? insn_unknown : insn_feqm_ps; // rd[4:3] == 0
               default : return insn_unknown;
               }
    case 0x53: switch (funct3) {
               case 0x0: return insn_fle_pi;
               case 0x1: return insn_flt_pi;
               case 0x2: return insn_feq_pi;
               case 0x3: return insn_fltu_pi;
               default : return insn_unknown;
               }
    case 0x60: switch (rs2) {
               case 0x00: return insn_fcvt_pw_ps;
               case 0x01: return insn_fcvt_pwu_ps;
               case 0x02: return (funct3 == 0) ? insn_fcvt_rast_ps : insn_unknown;
               default  : return insn_unknown;
               }
    case 0x68: switch (rs2) {
               case 0x00: return insn_fcvt_ps_pw;
               case 0x01: return insn_fcvt_ps_pwu;
               case 0x02: return insn_fcvt_ps_rast;
               case 0x08: return (funct3 == 0) ? insn_fcvt_ps_f10  : insn_unknown;
               case 0x09: return (funct3 == 0) ? insn_fcvt_ps_f11  : insn_unknown;
               case 0x0a: return (funct3 == 0) ? insn_fcvt_ps_f16  : insn_unknown;
               case 0x10: return (funct3 == 0) ? insn_fcvt_ps_un24 : insn_unknown;
               case 0x11: return (funct3 == 0) ? insn_fcvt_ps_un16 : insn_unknown;
               case 0x12: return (funct3 == 0) ? insn_fcvt_ps_un10 : insn_unknown;
               case 0x13: return (funct3 == 0) ? insn_fcvt_ps_un8  : insn_unknown;
               case 0x17: return (funct3 == 0) ? insn_fcvt_ps_un2  : insn_unknown;
               case 0x19: return (funct3 == 0) ? insn_fcvt_ps_sn16 : insn_unknown;
               case 0x1b: return (funct3 == 0) ? insn_fcvt_ps_sn8  : insn_unknown;
               default  : return insn_unknown;
               }
    case 0x6b: if ((bits & 0x01fff000) == 0x00000000) return insn_mova_x_m; // rs2|rs1|funct3 == 0
               if ((bits & 0x01f07f80) == 0x00001000) return insn_mova_m_x; // rs2|rd == 0, funct3 == 1
               return insn_unknown;
    case 0x6c: switch (rs2) {
               case 0x08: return (funct3 == 0) ? insn_fcvt_f11_ps  : insn_unknown;
               case 0x09: return (funct3 == 0) ? insn_fcvt_f16_ps  : insn_unknown;
               case 0x0b: return (funct3 == 0) ? insn_fcvt_f10_ps  : insn_unknown;
               case 0x10: return (funct3 == 0) ? insn_fcvt_un24_ps : insn_unknown;
               case 0x11: return (funct3 == 0) ? insn_fcvt_un16_ps : insn_unknown;
               case 0x12: return (funct3 == 0) ? insn_fcvt_un10_ps : insn_unknown;
               case 0x13: return (funct3 == 0) ? insn_fcvt_un8_ps  : insn_unknown;
               case 0x17: return (funct3 == 0) ? insn_fcvt_un2_ps  : insn_unknown;
               case 0x19: return (funct3 == 0) ? insn_fcvt_sn16_ps : insn_unknown;
               case 0x1b: return (funct3 == 0) ? insn_fcvt_sn8_ps  : insn_unknown;
               default  : return insn_unknown;
               }
    case 0x70: switch (funct3) {
               case 0x0: return (bits & 0x01800000) ? insn_unknown : insn_fmvz_x_ps; // rs2[4:3] == 0
               case 0x1: return (bits & 0x01f00000) ? insn_unknown : insn_fclass_ps; // rs2 == 0
               case 0x2: return (bits & 0x01800000) ? insn_unknown : insn_fmvs_x_ps; // rs2[4:3] == 0
               case 0x5: return (bits & 0x01f00c00) ? insn_unknown : insn_flem_ps;   // rs2|rd[4:3] == 0
               default : return insn_unknown;
               }
    case 0x73: return insn_fswizz_ps;
    default  : return insn_unknown;
    }
}


static insn_exec_funct_t dec_insn_80b(uint32_t bits __attribute__((unused)),
                                      uint32_t& flags __attribute__((unused)))
{
    return insn_unknown;
}


// -----------------------------------------------------------------------------
//
// 16-bit instruction decoding
//
// -----------------------------------------------------------------------------

// ----- Quadrant 0 -----

static insn_exec_funct_t dec_c_addi4spn(uint32_t bits, uint32_t& flags __attribute__((unused)))
{
    if (bits != 0) {
        uint16_t nzuimm = (bits >> 5) & 0xff;
        return (nzuimm != 0) ? insn_c_addi4spn : insn_unknown;
    }
    return insn_c_illegal;
}


static insn_exec_funct_t dec_c_fld(uint32_t bits __attribute__((unused)),
                                   uint32_t& flags __attribute__((unused)))
{
    return insn_unknown; // RV64D: insn_fld
}


static insn_exec_funct_t dec_c_lw(uint32_t bits __attribute__((unused)), uint32_t& flags)
{
    flags |= insn_t::flag_LOAD;
    return insn_c_lw;
}


static insn_exec_funct_t dec_c_ld(uint32_t bits __attribute__((unused)), uint32_t& flags)
{
    flags |= insn_t::flag_LOAD;
    return insn_c_ld;
}


static insn_exec_funct_t dec_c_reserved(uint32_t bits __attribute__((unused)),
                                        uint32_t& flags __attribute__((unused)))
{
    return insn_unknown;
}


static insn_exec_funct_t dec_c_fsd(uint32_t bits __attribute__((unused)),
                                   uint32_t& flags __attribute__((unused)))
{
    return insn_unknown; // RV64D: insn_fsd
}


static insn_exec_funct_t dec_c_sw(uint32_t bits __attribute__((unused)),
                                  uint32_t& flags __attribute__((unused)))
{
    return insn_c_sw;
}


static insn_exec_funct_t dec_c_sd(uint32_t bits __attribute__((unused)),
                                  uint32_t& flags __attribute__((unused)))
{
    return insn_c_sd;
}


// ----- Quadrant 1 -----

static insn_exec_funct_t dec_c_addi(uint32_t bits, uint32_t& flags __attribute__((unused)))
{
    uint16_t rs1_rd = (bits >> 7) & 31;
    return (rs1_rd == 0) ? insn_c_nop : insn_c_addi;
}


static insn_exec_funct_t dec_c_addiw(uint32_t bits, uint32_t& flags __attribute__((unused)))
{
    uint16_t rs1_rd = (bits >> 7) & 31;
    return (rs1_rd == 0) ? insn_unknown : insn_c_addiw;
}


static insn_exec_funct_t dec_c_li(uint32_t bits __attribute__((unused)),
                                  uint32_t& flags __attribute__((unused)))
{
    return insn_c_li;
}


static insn_exec_funct_t dec_c_lui_addi16sp(uint32_t bits, uint32_t& flags __attribute__((unused)))
{
    uint16_t nzimm = ((bits >> 7) & 0x20) | ((bits >> 2) & 0x1f);
    if (nzimm != 0) {
        uint16_t rs1_rd = (bits >> 7) & 31;
        return (rs1_rd == 2) ? insn_c_addi16sp : insn_c_lui;
    }
    return insn_unknown;
}


static insn_exec_funct_t dec_c_misc_alu(uint32_t bits, uint32_t& flags __attribute__((unused)))
{
    // bits 12|11:10|6:5
    static const insn_exec_funct_t functab[32] = {
                   /*xxx.00*/   /*xxx.01*/   /*xxx.10*/    /*xxx.11*/
        /*000.xx*/ insn_c_srli, insn_c_srli, insn_c_srli,  insn_c_srli,
        /*001.xx*/ insn_c_srai, insn_c_srai, insn_c_srai,  insn_c_srai,
        /*010.xx*/ insn_c_andi, insn_c_andi, insn_c_andi,  insn_c_andi,
        /*011.xx*/ insn_c_sub,  insn_c_xor,  insn_c_or,    insn_c_and,
        /*100.xx*/ insn_c_srli, insn_c_srli, insn_c_srli,  insn_c_srli,
        /*101.xx*/ insn_c_srai, insn_c_srai, insn_c_srai,  insn_c_srai,
        /*110.xx*/ insn_c_andi, insn_c_andi, insn_c_andi,  insn_c_andi,
        /*111.xx*/ insn_c_subw, insn_c_addw, insn_unknown, insn_unknown
    };
    uint16_t idx = ((bits >> 10) & 0x1c) | ((bits >> 5) & 0x3);
    return functab[idx];
}


static insn_exec_funct_t dec_c_j(uint32_t bits __attribute__((unused)),
                                 uint32_t& flags __attribute__((unused)))
{
    return insn_c_j;
}


static insn_exec_funct_t dec_c_beqz(uint32_t bits __attribute__((unused)),
                                    uint32_t& flags __attribute__((unused)))
{
    return insn_c_beqz;
}


static insn_exec_funct_t dec_c_bnez(uint32_t bits __attribute__((unused)),
                                    uint32_t& flags __attribute__((unused)))
{
    return insn_c_bnez;
}


// ----- Quadrant 2 -----

static insn_exec_funct_t dec_c_slli(uint32_t bits __attribute__((unused)),
                                    uint32_t& flags __attribute__((unused)))
{
    return insn_c_slli;
}


static insn_exec_funct_t dec_c_fldsp(uint32_t bits __attribute__((unused)),
                                     uint32_t& flags __attribute__((unused)))
{
    return insn_unknown; // RV64D: insn_fldsp
}


static insn_exec_funct_t dec_c_lwsp(uint32_t bits, uint32_t& flags)
{
    uint16_t rs1_rd = (bits >> 7) & 31;
    flags |= insn_t::flag_LOAD;
    return (rs1_rd == 0) ? insn_unknown : insn_c_lwsp;
}


static insn_exec_funct_t dec_c_ldsp(uint32_t bits, uint32_t& flags)
{
    uint16_t rs1_rd = (bits >> 7) & 31;
    flags |= insn_t::flag_LOAD;
    return (rs1_rd == 0) ? insn_unknown : insn_c_ldsp;
}


static insn_exec_funct_t dec_c_jalr_mv_add(uint32_t bits, uint32_t& flags __attribute__((unused)))
{
    // idx[2] = bits[12]
    // idx[1] = (rs1_rd != 0)
    // idx[0] = (rs2 != 0)
    static const insn_exec_funct_t functab[8] = {
        /* 000 */ insn_unknown,
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
                                     uint32_t& flags __attribute__((unused)))
{
    return insn_unknown;
}


static insn_exec_funct_t dec_c_swsp(uint32_t bits __attribute__((unused)),
                                    uint32_t& flags __attribute__((unused)))
{
    return insn_c_swsp;
}


static insn_exec_funct_t dec_c_sdsp(uint32_t bits __attribute__((unused)),
                                    uint32_t& flags __attribute__((unused)))
{
    return insn_c_sdsp;
}


// -----------------------------------------------------------------------------
//
// Fetch and decode an instruction
//
// -----------------------------------------------------------------------------

// NB: placeholder for flushing any cached decoding results we may have
// to synchronize when fence_i() is executed
void flush_insn_cache()
{
}

void insn_t::fetch_and_decode(uint64_t vaddr)
{
    // Opcode map inst[1:0]=11b, indexed using inst[6:2]
    // See RV64
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

    // local copy of the class members
    uint32_t _bits = 0;
    uint32_t _flags = 0;
    insn_exec_funct_t _exec_fn = nullptr;

    // Fetch two 16b words; don't translate the address twice if both words
    // are in the same 4KiB area
    uint64_t paddr = vmemtranslate(vaddr, Mem_Access_Fetch);
    uint16_t low = pmemfetch16(paddr);
    if ((low & 0x3) != 0x3) {
        _bits = uint32_t(low);
        LOG(DEBUG, "Fetched compressed instruction from PC %" PRIx64 ": 0x%04x", vaddr, low);
    } else if ((paddr & 4095) <= 4092) {
        uint16_t high = pmemfetch16(paddr + 2);
        _bits = uint32_t(low) + (uint32_t(high) << 16);
        LOG(DEBUG, "Fetched instruction from PC %" PRIx64 ": 0x%08x", vaddr, _bits);
    } else {
        uint64_t paddr2 = vmemtranslate(vaddr + 2, Mem_Access_Fetch);
        uint16_t high = pmemfetch16(paddr2);
        _bits = uint32_t(low) + (uint32_t(high) << 16);
        LOG(DEBUG, "Fetched instruction from PC %" PRIx64 ": 0x%08x", vaddr, _bits);
    }
    current_inst = _bits;

    // Decode the fetched bits
    if ((_bits & 0x3) == 0x3) {
        int idx = ((_bits >> 2) & 0x1f);
        _exec_fn = functab32b[idx](_bits, _flags);
    } else {
        int idx = ((_bits >> 11) & 0x1c) | (_bits & 0x03);
        _exec_fn = functab16b[idx](_bits, _flags);
    }

    // overwrite const members
    ::new (this) insn_t(_bits, _flags, _exec_fn);
}
