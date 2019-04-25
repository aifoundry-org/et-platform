#include "insn.h"
#include "insn_func.h"
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

// ----- Zifencetso ----------------------------------------
UNIMPL (insn_fence_tso)

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

// ----- RV64C ---------------------------------------------

// defined illegal instruction

void insn_c_illegal(insn_t inst __attribute__((unused))) {
    unknown();
}

// breakpoint instruction

void insn_c_ebreak(insn_t inst __attribute__((unused))) {
    ebreak();
}
