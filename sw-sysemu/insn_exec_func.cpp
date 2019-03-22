#include "insn.h"
#include "insn_exec_func.h"
#include "emu.h"

// ----- Helper templates ----------------------------------

#define UNIMPL(name) \
    void name(insn_t inst __attribute__((unused))) { \
        unknown(); \
    }

#define FMT_NONE(name, exec) \
    void name(insn_t inst __attribute__((unused))) { \
        (exec) (); \
    }

#define FMT_PRED_SUCC(name, exec) \
    void name(insn_t inst __attribute__((unused))) { \
        (exec) (); \
    }

#define FMT_RD(name, exec) \
    void name(insn_t inst) { \
        (exec) (inst.rd()); \
    }

#define FMT_RD_CSR_RS1(name, exec) \
    void name(insn_t inst) { \
        (exec) (inst.rd(), inst.csrimm(), inst.rs1()); \
    }

#define FMT_RD_CSR_UIMM5(name, exec) \
    void name(insn_t inst) { \
        (exec) (inst.rd(), inst.csrimm(), inst.uimm5()); \
    }

#define FMT_RD_JIMM(name, exec) \
    void name(insn_t inst) { \
        (exec) (inst.rd(), inst.j_imm()); \
    }

#define FMT_RD_RS1_IIMM(name, exec) \
    void name(insn_t inst) { \
        (exec) (inst.rd(), inst.rs1(), inst.i_imm()); \
    }

#define FMT_RD_RS1_RS2(name, exec) \
    void name(insn_t inst) { \
        (exec) (inst.rd(), inst.rs1(), inst.rs2()); \
    }

#define FMT_RD_RS1_RS2_AQ_RL(name, exec) \
    void name(insn_t inst) { \
        (exec) (inst.rd(), inst.rs1(), inst.rs2()/*, inst.aqrl()*/); \
    }

#define FMT_RD_RS1_SHAMT5(name, exec) \
    void name(insn_t inst) { \
        (exec) (inst.rd(), inst.rs1(), inst.shamt5()); \
    }

#define FMT_RD_RS1_SHAMT6(name, exec) \
    void name(insn_t inst) { \
        (exec) (inst.rd(), inst.rs1(), inst.shamt6()); \
    }

#define FMT_RD_UIMM(name, exec) \
    void name(insn_t inst) { \
        (exec) (inst.rd(), inst.u_imm()); \
    }

#define FMT_RS1(name, exec) \
    void name(insn_t inst) { \
        (exec) (inst.rs1()); \
    }

#define FMT_RS1_RS2(name, exec) \
    void name(insn_t inst) { \
        (exec) (inst.rs1(), inst.rs2()); \
    }

#define FMT_RS1_RS2_BIMM(name, exec) \
    void name(insn_t inst) { \
        (exec) (inst.rs1(), inst.rs2(), inst.b_imm()); \
    }

#define FMT_RS2_RS1_SIMM(name, exec) \
    void name(insn_t inst) { \
        (exec) (inst.rs2(), inst.rs1(), inst.s_imm()); \
    }

#define FMT_RS2_RS1(name, exec) \
    void name(insn_t inst) { \
        (exec) (inst.rs2(), inst.rs1()); \
    }

#define FMT_MD_FS1(name, exec) \
    void name(insn_t inst) { \
        (exec) (inst.md(), inst.fs1()); \
    }

#define FMT_MD_FS1_FS2(name, exec) \
    void name(insn_t inst) { \
        (exec) (inst.md(), inst.fs1(), inst.fs2()); \
    }

#define FMT_MD_MS1(name, exec) \
    void name(insn_t inst) { \
        (exec) (inst.md(), inst.ms1()); \
    }

#define FMT_MD_MS1_MS2(name, exec) \
    void name(insn_t inst) { \
        (exec) (inst.md(), inst.ms1(), inst.ms2()); \
    }

#define FMT_MD_RS1_UIMM8(name, exec) \
    void name(insn_t inst) { \
        (exec) (inst.md(), inst.rs1(), inst.uimm8()); \
    }

#define FMT_RD_MS1(name, exec) \
    void name(insn_t inst) { \
        (exec) (inst.rd(), inst.ms1()); \
    }

#define FMT_RD_MS1_MS2_UMSK4(name, exec) \
    void name(insn_t inst) { \
        (exec) (inst.rd(), inst.ms1(), inst.ms2(), inst.umsk4()); \
    }

#define FMT_FD_F32IMM(name, exec) \
    void name(insn_t inst) { \
        (exec) (inst.fd(), inst.f32imm()); \
    }

#define FMT_FD_FS1(name, exec) \
    void name(insn_t inst) { \
        (exec) (inst.fd(), inst.fs1()); \
    }

#define FMT_FD_FS1_FS2(name, exec) \
    void name(insn_t inst) { \
        (exec) (inst.fd(), inst.fs1(), inst.fs2()); \
    }

#define FMT_FD_FS1_FS2_FS3(name, exec) \
    void name(insn_t inst) { \
        (exec) (inst.fd(), inst.fs1(), inst.fs2(), inst.fs3()); \
    }

#define FMT_FD_FS1_FS2_FS3_RM(name, exec) \
    void name(insn_t inst) { \
        (exec) (inst.fd(), inst.fs1(), inst.fs2(), inst.fs3(), inst.rm()); \
    }

#define FMT_FD_FS1_FS2_RM(name, exec) \
    void name(insn_t inst) { \
        (exec) (inst.fd(), inst.fs1(), inst.fs2(), inst.rm()); \
    }

#define FMT_FD_FS1_RM(name, exec) \
    void name(insn_t inst) { \
        (exec) (inst.fd(), inst.fs1(), inst.rm()); \
    }

#define FMT_FD_FS1_RS2(name, exec) \
    void name(insn_t inst) { \
        (exec) (inst.fd(), inst.fs1(), inst.rs2()); \
    }

#define FMT_FD_FS1_SHAMT5(name, exec) \
    void name(insn_t inst) { \
        (exec) (inst.fd(), inst.fs1(), inst.shamt5()); \
    }

#define FMT_FD_FS1_UIMM8(name, exec) \
    void name(insn_t inst) { \
        (exec) (inst.fd(), inst.fs1(), inst.uimm8()); \
    }

#define FMT_FD_FS1_VIMM(name, exec) \
    void name(insn_t inst) { \
        (exec) (inst.fd(), inst.fs1(), inst.v_imm()); \
    }

#define FMT_FD_I32IMM(name, exec) \
    void name(insn_t inst) { \
        (exec) (inst.fd(), inst.i32imm()); \
    }

#define FMT_FD_RS1(name, exec) \
    void name(insn_t inst) { \
        (exec) (inst.fd(), inst.rs1()); \
    }

#define FMT_FD_RS1_IIMM(name, exec) \
    void name(insn_t inst) { \
        (exec) (inst.fd(), inst.rs1(), inst.i_imm()); \
    }

#define FMT_FD_RS1_RM(name, exec) \
    void name(insn_t inst) { \
        (exec) (inst.fd(), inst.rs1(), inst.rm()); \
    }

#define FMT_FD_RS1_RS2(name, exec) \
    void name(insn_t inst) { \
        (exec) (inst.fd(), inst.rs1(), inst.rs2()); \
    }

#define FMT_FS2_RS1(name, exec) \
    void name(insn_t inst) { \
        (exec) (inst.fs2(), inst.rs1()); \
    }

#define FMT_FS2_RS1_SIMM(name, exec) \
    void name(insn_t inst) { \
        (exec) (inst.fs2(), inst.rs1(), inst.s_imm()); \
    }

#define FMT_RD_FS1(name, exec) \
    void name(insn_t inst) { \
        (exec) (inst.rd(), inst.fs1()); \
    }

#define FMT_RD_FS1_FS2(name, exec) \
    void name(insn_t inst) { \
        (exec) (inst.rd(), inst.fs1(), inst.fs2()); \
    }

#define FMT_RD_FS1_RM(name, exec) \
    void name(insn_t inst) { \
        (exec) (inst.rd(), inst.fs1(), inst.rm()); \
    }

#define FMT_RD_FS1_UIMM3(name, exec) \
    void name(insn_t inst) { \
        (exec) (inst.rd(), inst.fs1(), inst.uimm3()); \
    }


// ----- Unknown -------------------------------------------
UNIMPL (insn_unknown)

// ----- RV64I ---------------------------------------------
// computational
FMT_RD_RS1_RS2    (insn_add,    add)
FMT_RD_RS1_IIMM   (insn_addi,   addi)
FMT_RD_RS1_IIMM   (insn_addiw,  addiw)
FMT_RD_RS1_RS2    (insn_addw,   addw)
FMT_RD_RS1_RS2    (insn_and,    and_)
FMT_RD_RS1_IIMM   (insn_andi,   andi)
FMT_RD_UIMM       (insn_auipc,  auipc)
FMT_RD_UIMM       (insn_lui,    lui)
FMT_RD_RS1_RS2    (insn_or,     or_)
FMT_RD_RS1_IIMM   (insn_ori,    ori)
FMT_RD_RS1_RS2    (insn_sll,    sll)
FMT_RD_RS1_SHAMT6 (insn_slli,   slli)
FMT_RD_RS1_SHAMT5 (insn_slliw,  slliw)
FMT_RD_RS1_RS2    (insn_sllw,   sllw)
FMT_RD_RS1_RS2    (insn_slt,    slt)
FMT_RD_RS1_IIMM   (insn_slti,   slti)
FMT_RD_RS1_IIMM   (insn_sltiu,  sltiu)
FMT_RD_RS1_RS2    (insn_sltu,   sltu)
FMT_RD_RS1_RS2    (insn_sra,    sra)
FMT_RD_RS1_SHAMT6 (insn_srai,   srai)
FMT_RD_RS1_SHAMT5 (insn_sraiw,  sraiw)
FMT_RD_RS1_RS2    (insn_sraw,   sraw)
FMT_RD_RS1_RS2    (insn_srl,    srl)
FMT_RD_RS1_SHAMT6 (insn_srli,   srli)
FMT_RD_RS1_SHAMT5 (insn_srliw,  srliw)
FMT_RD_RS1_RS2    (insn_srlw,   srlw)
FMT_RD_RS1_RS2    (insn_sub,    sub)
FMT_RD_RS1_RS2    (insn_subw,   subw)
FMT_RD_RS1_RS2    (insn_xor,    xor_)
FMT_RD_RS1_IIMM   (insn_xori,   xori)
// control
FMT_RD_JIMM       (insn_jal,    jal)
FMT_RD_RS1_IIMM   (insn_jalr,   jalr)
FMT_RS1_RS2_BIMM  (insn_beq,    beq)
FMT_RS1_RS2_BIMM  (insn_bge,    bge)
FMT_RS1_RS2_BIMM  (insn_bgeu,   bgeu)
FMT_RS1_RS2_BIMM  (insn_blt,    blt)
FMT_RS1_RS2_BIMM  (insn_bltu,   bltu)
FMT_RS1_RS2_BIMM  (insn_bne,    bne)
// loads/stores
FMT_RD_RS1_IIMM   (insn_lb,     lb)
FMT_RD_RS1_IIMM   (insn_lbu,    lbu)
FMT_RD_RS1_IIMM   (insn_ld,     ld)
FMT_RD_RS1_IIMM   (insn_lh,     lh)
FMT_RD_RS1_IIMM   (insn_lhu,    lhu)
FMT_RD_RS1_IIMM   (insn_lw,     lw)
FMT_RD_RS1_IIMM   (insn_lwu,    lwu)
FMT_RS2_RS1_SIMM  (insn_sb,     sb)
FMT_RS2_RS1_SIMM  (insn_sd,     sd)
FMT_RS2_RS1_SIMM  (insn_sh,     sh)
FMT_RS2_RS1_SIMM  (insn_sw,     sw)
// memory ordering
FMT_PRED_SUCC     (insn_fence,  fence)

// ----- Zifencei ------------------------------------------
FMT_NONE (insn_fence_i, fence_i)

// ----- Zifencetso ----------------------------------------
UNIMPL (insn_fence_tso)

// ----- RV64M ---------------------------------------------
FMT_RD_RS1_RS2 (insn_div,     div_)
FMT_RD_RS1_RS2 (insn_divu,    divu)
FMT_RD_RS1_RS2 (insn_divuw,   divuw)
FMT_RD_RS1_RS2 (insn_divw,    divw)
FMT_RD_RS1_RS2 (insn_mul,     mul)
FMT_RD_RS1_RS2 (insn_mulh,    mulh)
FMT_RD_RS1_RS2 (insn_mulhsu,  mulhsu)
FMT_RD_RS1_RS2 (insn_mulhu,   mulhu)
FMT_RD_RS1_RS2 (insn_mulw,    mulw)
FMT_RD_RS1_RS2 (insn_rem,     rem)
FMT_RD_RS1_RS2 (insn_remu,    remu)
FMT_RD_RS1_RS2 (insn_remuw,   remuw)
FMT_RD_RS1_RS2 (insn_remw,    remw)

// ----- RV64F ---------------------------------------------
// loads
FMT_FD_RS1_IIMM       (insn_flw,        flw)
// stores
FMT_FS2_RS1_SIMM      (insn_fsw,        fsw)
// computational
FMT_FD_FS1_FS2_RM     (insn_fadd_s,     fadd_s)
FMT_FD_FS1_FS2_RM     (insn_fdiv_s,     fdiv_s)
FMT_FD_FS1_FS2_FS3_RM (insn_fmadd_s,    fmadd_s)
FMT_FD_FS1_FS2        (insn_fmax_s,     fmax_s)
FMT_FD_FS1_FS2        (insn_fmin_s,     fmin_s)
FMT_FD_FS1_FS2_FS3_RM (insn_fmsub_s,    fmsub_s)
FMT_FD_FS1_FS2_RM     (insn_fmul_s,     fmul_s)
FMT_FD_FS1_FS2_FS3_RM (insn_fnmadd_s,   fnmadd_s)
FMT_FD_FS1_FS2_FS3_RM (insn_fnmsub_s,   fnmsub_s)
FMT_FD_FS1_RM         (insn_fsqrt_s,    fsqrt_s)
FMT_FD_FS1_FS2_RM     (insn_fsub_s,     fsub_s)
// conversions
FMT_RD_FS1_RM         (insn_fcvt_l_s,   fcvt_l_s)
FMT_RD_FS1_RM         (insn_fcvt_lu_s,  fcvt_lu_s)
FMT_RD_FS1_RM         (insn_fcvt_w_s,   fcvt_w_s)
FMT_RD_FS1_RM         (insn_fcvt_wu_s,  fcvt_wu_s)
FMT_FD_RS1_RM         (insn_fcvt_s_l,   fcvt_s_l)
FMT_FD_RS1_RM         (insn_fcvt_s_lu,  fcvt_s_lu)
FMT_FD_RS1_RM         (insn_fcvt_s_w,   fcvt_s_w)
FMT_FD_RS1_RM         (insn_fcvt_s_wu,  fcvt_s_wu)
// moves
FMT_FD_FS1_FS2        (insn_fsgnj_s,    fsgnj_s)
FMT_FD_FS1_FS2        (insn_fsgnjn_s,   fsgnjn_s)
FMT_FD_FS1_FS2        (insn_fsgnjx_s,   fsgnjx_s)
FMT_FD_RS1            (insn_fmv_w_x,    fmv_w_x)
FMT_RD_FS1            (insn_fmv_x_w,    fmv_x_w)
// comparisons
FMT_RD_FS1_FS2        (insn_feq_s,      feq_s)
FMT_RD_FS1_FS2        (insn_fle_s,      fle_s)
FMT_RD_FS1_FS2        (insn_flt_s,      flt_s)
// classify
FMT_RD_FS1            (insn_fclass_s,   fclass_s)

// ----- SYSTEM --------------------------------------------
// control and status registers
FMT_RD_CSR_RS1   (insn_csrrc,      csrrc)
FMT_RD_CSR_RS1   (insn_csrrs,      csrrs)
FMT_RD_CSR_RS1   (insn_csrrw,      csrrw)
FMT_RD_CSR_UIMM5 (insn_csrrci,     csrrci)
FMT_RD_CSR_UIMM5 (insn_csrrsi,     csrrsi)
FMT_RD_CSR_UIMM5 (insn_csrrwi,     csrrwi)
// environment call & breakpoints
FMT_NONE         (insn_ebreak,     ebreak)
FMT_NONE         (insn_ecall,      ecall)
// environment return
FMT_NONE         (insn_sret,       sret)
FMT_NONE         (insn_mret,       mret)
// wait for interrupt
FMT_NONE         (insn_wfi,        wfi)
// sfence.vma
FMT_RS1_RS2      (insn_sfence_vma, sfence_vma)

// ----- ET mask -------------------------------------------
FMT_MD_MS1_MS2       (insn_maskand,       maskand)
FMT_MD_MS1_MS2       (insn_maskor,        maskor)
FMT_MD_MS1_MS2       (insn_maskxor,       maskxor)
FMT_MD_MS1           (insn_masknot,       masknot)
FMT_RD               (insn_mova_x_m,      mova_x_m)
FMT_RS1              (insn_mova_m_x,      mova_m_x)
FMT_MD_RS1_UIMM8     (insn_mov_m_x,       mov_m_x)
FMT_RD_MS1           (insn_maskpopc,      maskpopc)
FMT_RD_MS1           (insn_maskpopcz,     maskpopcz)
FMT_RD_MS1_MS2_UMSK4 (insn_maskpopc_rast, maskpopc_rast)

// ----- ET packed single ----------------------------------
// loads
FMT_FD_RS1_IIMM       (insn_flq2,           flq2)
FMT_FD_RS1_IIMM       (insn_flw_ps,         flw_ps)
FMT_FD_RS1            (insn_flwg_ps,        flwg_ps)
FMT_FD_RS1            (insn_flwl_ps,        flwl_ps)
// stores
FMT_FS2_RS1_SIMM      (insn_fsq2,           fsq2)
FMT_FS2_RS1_SIMM      (insn_fsw_ps,         fsw_ps)
FMT_FD_RS1            (insn_fswg_ps,        fswg_ps)
FMT_FD_RS1            (insn_fswl_ps,        fswl_ps)
// broadcast
FMT_FD_RS1_IIMM       (insn_fbc_ps,         fbc_ps)
FMT_FD_F32IMM         (insn_fbci_ps,        fbci_ps)
FMT_FD_RS1            (insn_fbcx_ps,        fbcx_ps)
// gather
FMT_FD_FS1_RS2        (insn_fgb_ps,         fgb_ps)
FMT_FD_FS1_RS2        (insn_fgh_ps,         fgh_ps)
FMT_FD_FS1_RS2        (insn_fgw_ps,         fgw_ps)
FMT_FD_FS1_RS2        (insn_fgwg_ps,        fgwg_ps)
FMT_FD_FS1_RS2        (insn_fghg_ps,        fghg_ps)
FMT_FD_FS1_RS2        (insn_fgbg_ps,        fgbg_ps)
FMT_FD_FS1_RS2        (insn_fgwl_ps,        fgwl_ps)
FMT_FD_FS1_RS2        (insn_fghl_ps,        fghl_ps)
FMT_FD_FS1_RS2        (insn_fgbl_ps,        fgbl_ps)
FMT_FD_RS1_RS2        (insn_fg32b_ps,       fg32b_ps)
FMT_FD_RS1_RS2        (insn_fg32h_ps,       fg32h_ps)
FMT_FD_RS1_RS2        (insn_fg32w_ps,       fg32w_ps)
// scatter
FMT_FD_FS1_RS2        (insn_fscb_ps,        fscb_ps)
FMT_FD_FS1_RS2        (insn_fsch_ps,        fsch_ps)
FMT_FD_FS1_RS2        (insn_fscw_ps,        fscw_ps)
FMT_FD_FS1_RS2        (insn_fscwg_ps,       fscwg_ps)
FMT_FD_FS1_RS2        (insn_fschg_ps,       fschg_ps)
FMT_FD_FS1_RS2        (insn_fscbg_ps,       fscbg_ps)
FMT_FD_FS1_RS2        (insn_fscwl_ps,       fscwl_ps)
FMT_FD_FS1_RS2        (insn_fschl_ps,       fschl_ps)
FMT_FD_FS1_RS2        (insn_fscbl_ps,       fscbl_ps)
FMT_FD_RS1_RS2        (insn_fsc32b_ps,      fsc32b_ps)
FMT_FD_RS1_RS2        (insn_fsc32h_ps,      fsc32h_ps)
FMT_FD_RS1_RS2        (insn_fsc32w_ps,      fsc32w_ps)
// computational
FMT_FD_FS1_FS2_RM     (insn_fadd_ps,        fadd_ps)
FMT_FD_FS1_FS2_RM     (insn_fdiv_ps,        fdiv_ps)
FMT_FD_FS1_FS2_FS3_RM (insn_fmadd_ps,       fmadd_ps)
FMT_FD_FS1_FS2        (insn_fmax_ps,        fmax_ps)
FMT_FD_FS1_FS2        (insn_fmin_ps,        fmin_ps)
FMT_FD_FS1_FS2_FS3_RM (insn_fmsub_ps,       fmsub_ps)
FMT_FD_FS1_FS2_RM     (insn_fmul_ps,        fmul_ps)
FMT_FD_FS1_FS2_FS3_RM (insn_fnmadd_ps,      fnmadd_ps)
FMT_FD_FS1_FS2_FS3_RM (insn_fnmsub_ps,      fnmsub_ps)
FMT_FD_FS1_RM         (insn_fsqrt_ps,       fsqrt_ps)
FMT_FD_FS1_FS2_RM     (insn_fsub_ps,        fsub_ps)
// conversions
FMT_FD_FS1_RM         (insn_fcvt_ps_pw,     fcvt_ps_pw)
FMT_FD_FS1_RM         (insn_fcvt_ps_pwu,    fcvt_ps_pwu)
FMT_FD_FS1_RM         (insn_fcvt_pw_ps,     fcvt_pw_ps)
FMT_FD_FS1_RM         (insn_fcvt_pwu_ps,    fcvt_pwu_ps)
// moves
FMT_FD_FS1_FS2_FS3    (insn_fcmov_ps,       fcmov_ps)
FMT_FD_FS1_FS2        (insn_fcmovm_ps,      fcmovm_ps)
FMT_RD_FS1_UIMM3      (insn_fmvs_x_ps,      fmvs_x_ps)
FMT_RD_FS1_UIMM3      (insn_fmvz_x_ps,      fmvz_x_ps)
FMT_FD_FS1_FS2        (insn_fsgnj_ps,       fsgnj_ps)
FMT_FD_FS1_FS2        (insn_fsgnjn_ps,      fsgnjn_ps)
FMT_FD_FS1_FS2        (insn_fsgnjx_ps,      fsgnjx_ps)
FMT_FD_FS1_UIMM8      (insn_fswizz_ps,      fswizz_ps)
// comparisons to freg
FMT_FD_FS1_FS2        (insn_feq_ps,         feq_ps)
FMT_FD_FS1_FS2        (insn_fle_ps,         fle_ps)
FMT_FD_FS1_FS2        (insn_flt_ps,         flt_ps)
// comparisons to mask
FMT_MD_FS1_FS2        (insn_feqm_ps,        feqm_ps)
FMT_MD_FS1_FS2        (insn_flem_ps,        flem_ps)
FMT_MD_FS1_FS2        (insn_fltm_ps,        fltm_ps)
// classify
FMT_FD_FS1            (insn_fclass_ps,      fclass_ps)
// graphics upconverts
FMT_FD_FS1            (insn_fcvt_ps_f10,    fcvt_ps_f10)
FMT_FD_FS1            (insn_fcvt_ps_f11,    fcvt_ps_f11)
FMT_FD_FS1            (insn_fcvt_ps_f16,    fcvt_ps_f16)
FMT_FD_FS1            (insn_fcvt_ps_sn8,    fcvt_ps_sn8)
FMT_FD_FS1            (insn_fcvt_ps_sn16,   fcvt_ps_sn16)
FMT_FD_FS1            (insn_fcvt_ps_un2,    fcvt_ps_un2)
FMT_FD_FS1            (insn_fcvt_ps_un8,    fcvt_ps_un8)
FMT_FD_FS1            (insn_fcvt_ps_un10,   fcvt_ps_un10)
FMT_FD_FS1            (insn_fcvt_ps_un16,   fcvt_ps_un16)
FMT_FD_FS1            (insn_fcvt_ps_un24,   fcvt_ps_un24)
// graphics downconverts
FMT_FD_FS1            (insn_fcvt_f10_ps,    fcvt_f10_ps)
FMT_FD_FS1            (insn_fcvt_f11_ps,    fcvt_f11_ps)
FMT_FD_FS1            (insn_fcvt_f16_ps,    fcvt_f16_ps)
FMT_FD_FS1            (insn_fcvt_sn8_ps,    fcvt_sn8_ps)
FMT_FD_FS1            (insn_fcvt_sn16_ps,   fcvt_sn16_ps)
FMT_FD_FS1            (insn_fcvt_un2_ps,    fcvt_un2_ps)
FMT_FD_FS1            (insn_fcvt_un8_ps,    fcvt_un8_ps)
FMT_FD_FS1            (insn_fcvt_un10_ps,   fcvt_un10_ps)
FMT_FD_FS1            (insn_fcvt_un16_ps,   fcvt_un16_ps)
FMT_FD_FS1            (insn_fcvt_un24_ps,   fcvt_un24_ps)
// graphics additional
FMT_FD_FS1            (insn_fcvt_rast_ps,   fcvt_rast_ps)
FMT_FD_FS1            (insn_fexp_ps,        fexp_ps)
FMT_FD_FS1            (insn_flog_ps,        flog_ps)
FMT_FD_FS1            (insn_frcp_ps,        frcp_ps)
FMT_FD_FS1            (insn_frsq_ps,        frsq_ps)
FMT_FD_FS1            (insn_fsin_ps,        fsin_ps)
FMT_FD_FS1_FS2        (insn_cubeface_ps,    cubeface_ps)
FMT_FD_FS1_FS2        (insn_cubefaceidx_ps, cubefaceidx_ps)
FMT_FD_FS1_FS2        (insn_cubesgnsc_ps,   cubesgnsc_ps)
FMT_FD_FS1_FS2        (insn_cubesgntc_ps,   cubesgntc_ps)
FMT_FD_FS1_FS2        (insn_frcp_fix_rast,  frcp_fix_rast)
FMT_FD_FS1_RM         (insn_fcvt_ps_rast,   fcvt_ps_rast)
FMT_FD_FS1            (insn_ffrc_ps,        ffrc_ps)
FMT_FD_FS1_RM         (insn_fround_ps,      fround_ps)

// ----- ET packed integer ---------------------------------
// broadcast
FMT_FD_I32IMM      (insn_fbci_pi,       fbci_pi)
// computational
FMT_FD_FS1_FS2     (insn_fadd_pi,       fadd_pi)
FMT_FD_FS1_VIMM    (insn_faddi_pi,      faddi_pi)
FMT_FD_FS1_FS2     (insn_fand_pi,       fand_pi)
FMT_FD_FS1_VIMM    (insn_fandi_pi,      fandi_pi)
FMT_FD_FS1_FS2     (insn_fdiv_pi,       fdiv_pi)
FMT_FD_FS1_FS2     (insn_fdivu_pi,      fdivu_pi)
FMT_FD_FS1_FS2     (insn_fmax_pi,       fmax_pi)
FMT_FD_FS1_FS2     (insn_fmaxu_pi,      fmaxu_pi)
FMT_FD_FS1_FS2     (insn_fmin_pi,       fmin_pi)
FMT_FD_FS1_FS2     (insn_fminu_pi,      fminu_pi)
FMT_FD_FS1_FS2     (insn_fmul_pi,       fmul_pi)
FMT_FD_FS1_FS2     (insn_fmulh_pi,      fmulh_pi)
FMT_FD_FS1_FS2     (insn_fmulhu_pi,     fmulhu_pi)
FMT_FD_FS1         (insn_fnot_pi,       fnot_pi)
FMT_FD_FS1_FS2     (insn_for_pi,        for_pi)
FMT_FD_FS1         (insn_fpackrepb_pi,  fpackrepb_pi)
FMT_FD_FS1         (insn_fpackreph_pi,  fpackreph_pi)
FMT_FD_FS1_FS2     (insn_frem_pi,       frem_pi)
FMT_FD_FS1_FS2     (insn_fremu_pi,      fremu_pi)
FMT_FD_FS1         (insn_fsat8_pi,      fsat8_pi)
FMT_FD_FS1         (insn_fsatu8_pi,     fsatu8_pi)
FMT_FD_FS1_FS2     (insn_fsll_pi,       fsll_pi)
FMT_FD_FS1_SHAMT5  (insn_fslli_pi,      fslli_pi)
FMT_FD_FS1_FS2     (insn_fsra_pi,       fsra_pi)
FMT_FD_FS1_SHAMT5  (insn_fsrai_pi,      fsrai_pi)
FMT_FD_FS1_FS2     (insn_fsrl_pi,       fsrl_pi)
FMT_FD_FS1_SHAMT5  (insn_fsrli_pi,      fsrli_pi)
FMT_FD_FS1_FS2     (insn_fsub_pi,       fsub_pi)
FMT_FD_FS1_FS2     (insn_fxor_pi,       fxor_pi)
// comparisons
FMT_FD_FS1_FS2     (insn_feq_pi,        feq_pi)
FMT_FD_FS1_FS2     (insn_fle_pi,        fle_pi)
FMT_FD_FS1_FS2     (insn_flt_pi,        flt_pi)
FMT_MD_FS1_FS2     (insn_fltm_pi,       fltm_pi)
FMT_FD_FS1_FS2     (insn_fltu_pi,       fltu_pi)
FMT_MD_FS1         (insn_fsetm_pi,      fsetm_pi)

// ----- ET graphics scalar --------------------------------
FMT_RD_RS1_RS2  (insn_packb,    packb)
FMT_RD_RS1_RS2  (insn_bitmixb,  bitmixb)

// ----- ET atomic -----------------------------------------
// scalar
FMT_RD_RS1_RS2  (insn_amoaddg_d,      amoaddg_d)
FMT_RD_RS1_RS2  (insn_amoaddg_w,      amoaddg_w)
FMT_RD_RS1_RS2  (insn_amoaddl_d,      amoaddl_d)
FMT_RD_RS1_RS2  (insn_amoaddl_w,      amoaddl_w)
FMT_RD_RS1_RS2  (insn_amoandg_d,      amoandg_d)
FMT_RD_RS1_RS2  (insn_amoandg_w,      amoandg_w)
FMT_RD_RS1_RS2  (insn_amoandl_d,      amoandl_d)
FMT_RD_RS1_RS2  (insn_amoandl_w,      amoandl_w)
FMT_RD_RS1_RS2  (insn_amomaxg_d,      amomaxg_d)
FMT_RD_RS1_RS2  (insn_amomaxg_w,      amomaxg_w)
FMT_RD_RS1_RS2  (insn_amomaxl_d,      amomaxl_d)
FMT_RD_RS1_RS2  (insn_amomaxl_w,      amomaxl_w)
FMT_RD_RS1_RS2  (insn_amomaxug_d,     amomaxug_d)
FMT_RD_RS1_RS2  (insn_amomaxug_w,     amomaxug_w)
FMT_RD_RS1_RS2  (insn_amomaxul_d,     amomaxul_d)
FMT_RD_RS1_RS2  (insn_amomaxul_w,     amomaxul_w)
FMT_RD_RS1_RS2  (insn_amoming_d,      amoming_d)
FMT_RD_RS1_RS2  (insn_amoming_w,      amoming_w)
FMT_RD_RS1_RS2  (insn_amominl_d,      amominl_d)
FMT_RD_RS1_RS2  (insn_amominl_w,      amominl_w)
FMT_RD_RS1_RS2  (insn_amominug_d,     amominug_d)
FMT_RD_RS1_RS2  (insn_amominug_w,     amominug_w)
FMT_RD_RS1_RS2  (insn_amominul_d,     amominul_d)
FMT_RD_RS1_RS2  (insn_amominul_w,     amominul_w)
FMT_RD_RS1_RS2  (insn_amoorg_d,       amoorg_d)
FMT_RD_RS1_RS2  (insn_amoorg_w,       amoorg_w)
FMT_RD_RS1_RS2  (insn_amoorl_d,       amoorl_d)
FMT_RD_RS1_RS2  (insn_amoorl_w,       amoorl_w)
FMT_RD_RS1_RS2  (insn_amoswapg_d,     amoswapg_d)
FMT_RD_RS1_RS2  (insn_amoswapg_w,     amoswapg_w)
FMT_RD_RS1_RS2  (insn_amoswapl_d,     amoswapl_d)
FMT_RD_RS1_RS2  (insn_amoswapl_w,     amoswapl_w)
FMT_RD_RS1_RS2  (insn_amoxorg_d,      amoxorg_d)
FMT_RD_RS1_RS2  (insn_amoxorg_w,      amoxorg_w)
FMT_RD_RS1_RS2  (insn_amoxorl_d,      amoxorl_d)
FMT_RD_RS1_RS2  (insn_amoxorl_w,      amoxorl_w)
FMT_RS2_RS1 (insn_sbl,            sbl)
FMT_RS2_RS1 (insn_sbg,            sbg)
FMT_RS2_RS1 (insn_shl,            shl)
FMT_RS2_RS1 (insn_shg,            shg)

// packed
FMT_FD_FS1_RS2  (insn_famoaddg_pi,    famoaddg_pi)
FMT_FD_FS1_RS2  (insn_famoaddl_pi,    famoaddl_pi)
FMT_FD_FS1_RS2  (insn_famoandg_pi,    famoandg_pi)
FMT_FD_FS1_RS2  (insn_famoandl_pi,    famoandl_pi)
FMT_FD_FS1_RS2  (insn_famomaxg_pi,    famomaxg_pi)
FMT_FD_FS1_RS2  (insn_famomaxg_ps,    famomaxg_ps)
FMT_FD_FS1_RS2  (insn_famomaxl_pi,    famomaxl_pi)
FMT_FD_FS1_RS2  (insn_famomaxl_ps,    famomaxl_ps)
FMT_FD_FS1_RS2  (insn_famomaxug_pi,   famomaxug_pi)
FMT_FD_FS1_RS2  (insn_famomaxul_pi,   famomaxul_pi)
FMT_FD_FS1_RS2  (insn_famoming_pi,    famoming_pi)
FMT_FD_FS1_RS2  (insn_famoming_ps,    famoming_ps)
FMT_FD_FS1_RS2  (insn_famominl_pi,    famominl_pi)
FMT_FD_FS1_RS2  (insn_famominl_ps,    famominl_ps)
FMT_FD_FS1_RS2  (insn_famominug_pi,   famominug_pi)
FMT_FD_FS1_RS2  (insn_famominul_pi,   famominul_pi)
FMT_FD_FS1_RS2  (insn_famoorg_pi,     famoorg_pi)
FMT_FD_FS1_RS2  (insn_famoorl_pi,     famoorl_pi)
FMT_FD_FS1_RS2  (insn_famoswapg_pi,   famoswapg_pi)
FMT_FD_FS1_RS2  (insn_famoswapl_pi,   famoswapl_pi)
FMT_FD_FS1_RS2  (insn_famoxorg_pi,    famoxorg_pi)
FMT_FD_FS1_RS2  (insn_famoxorl_pi,    famoxorl_pi)

// ----- RV64C ---------------------------------------------

// sp-based loads/stores

void insn_c_lwsp(insn_t inst) {
    lw(inst.rvc_rs1(), x2, inst.rvc_imm_lwsp());
}

void insn_c_ldsp(insn_t inst) {
    ld(inst.rvc_rs1(), x2, inst.rvc_imm_ldsp());
}

void insn_c_swsp(insn_t inst) {
    sw(inst.rvc_rs2(), x2, inst.rvc_imm_swsp());
}

void insn_c_sdsp(insn_t inst) {
    sd(inst.rvc_rs2(), x2, inst.rvc_imm_sdsp());
}

// reg-based loads/stores

void insn_c_lw(insn_t inst) {
    lw(inst.rvc_rs2p(), inst.rvc_rs1p(), inst.rvc_imm_lsw());
}

void insn_c_ld(insn_t inst) {
    ld(inst.rvc_rs2p(), inst.rvc_rs1p(), inst.rvc_imm_lsd());
}

void insn_c_sw(insn_t inst) {
    sw(inst.rvc_rs2p(), inst.rvc_rs1p(), inst.rvc_imm_lsw());
}

void insn_c_sd(insn_t inst) {
    sd(inst.rvc_rs2p(), inst.rvc_rs1p(), inst.rvc_imm_lsd());
}

// control transfer

void insn_c_j(insn_t inst) {
    c_jal(x0, inst.rvc_j_imm());
}

void insn_c_jalr(insn_t inst) {
    c_jalr(x1, inst.rvc_rs1(), 0);
}

void insn_c_jr(insn_t inst) {
    c_jalr(x0, inst.rvc_rs1(), 0);
}

void insn_c_beqz(insn_t inst) {
    beq(inst.rvc_rs1p(), x0, inst.rvc_b_imm());
}

void insn_c_bnez(insn_t inst) {
    bne(inst.rvc_rs1p(), x0, inst.rvc_b_imm());
}

// integer constant-generation

void insn_c_li(insn_t inst) {
    addi(inst.rvc_rs1(), x0, inst.rvc_imm6());
}

void insn_c_lui(insn_t inst) {
    lui(inst.rvc_rs1(), inst.rvc_nzimm_lui());
}

// integer reg-imm operations

void insn_c_addi(insn_t inst) {
    addi(inst.rvc_rs1(), inst.rvc_rs1(), inst.rvc_imm6());
}

void insn_c_addiw(insn_t inst) {
    addiw(inst.rvc_rs1(), inst.rvc_rs1(), inst.rvc_imm6());
}

void insn_c_addi16sp(insn_t inst) {
    addi(x2, x2, inst.rvc_nzimm_addi16sp());
}

void insn_c_addi4spn(insn_t inst) {
    addi(inst.rvc_rs2p(), x2, inst.rvc_nzuimm_addi4spn());
}

void insn_c_slli(insn_t inst) {
    slli(inst.rvc_rs1(), inst.rvc_rs1(), inst.rvc_shamt());
}

void insn_c_srai(insn_t inst) {
    srai(inst.rvc_rs1(), inst.rvc_rs1(), inst.rvc_shamt());
}

void insn_c_srli(insn_t inst) {
    srli(inst.rvc_rs1p(), inst.rvc_rs1p(), inst.rvc_shamt());
}

void insn_c_andi(insn_t inst) {
    andi(inst.rvc_rs1p(), inst.rvc_rs1p(), inst.rvc_imm6());
}

// integer reg-reg operations

void insn_c_mv(insn_t inst) {
    add(inst.rvc_rs1(), x0, inst.rvc_rs2());
}

void insn_c_add(insn_t inst) {
    add(inst.rvc_rs1(), inst.rvc_rs1(), inst.rvc_rs2());
}

void insn_c_and(insn_t inst) {
    and_(inst.rvc_rs1p(), inst.rvc_rs1p(), inst.rvc_rs2p());
}

void insn_c_or(insn_t inst) {
    or_(inst.rvc_rs1p(), inst.rvc_rs1p(), inst.rvc_rs2p());
}

void insn_c_xor(insn_t inst) {
    xor_(inst.rvc_rs1p(), inst.rvc_rs1p(), inst.rvc_rs2p());
}

void insn_c_sub(insn_t inst) {
    sub(inst.rvc_rs1p(), inst.rvc_rs1p(), inst.rvc_rs2p());
}

void insn_c_addw(insn_t inst) {
    addw(inst.rvc_rs1p(), inst.rvc_rs1p(), inst.rvc_rs2p());
}

void insn_c_subw(insn_t inst) {
    subw(inst.rvc_rs1p(), inst.rvc_rs1p(), inst.rvc_rs2p());
}

// defined illegal instruction

void insn_c_illegal(insn_t inst __attribute__((unused))) {
    unknown();
}

// breakpoint instruction

void insn_c_ebreak(insn_t inst __attribute__((unused))) {
    ebreak();
}
