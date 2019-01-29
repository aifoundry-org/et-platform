 #include "insn.h"
#include "insn_exec_func.h"
#include "emu.h"

// ----- Helper functions ----------------------------------

static csr imm2csr(uint16_t imm)
{
    switch (imm) {
    /* RISCV user registers */
    //case 0x000 : return csr_ustatus;
    case 0x001 : return csr_fflags;
    case 0x002 : return csr_frm;
    case 0x003 : return csr_fcsr;
    //case 0x004 : return csr_uie;
    //case 0x005 : return csr_utvec;
    //case 0x040 : return csr_uscratch;
    //case 0x041 : return csr_uepc;
    //case 0x042 : return csr_ucause;
    //case 0x043 : return csr_utval;
    //case 0x044 : return csr_uip;
    case 0xc00 : return csr_cycle;
    //case 0xc01 : return csr_time;
    case 0xc02 : return csr_instret;
    //case 0xc03 : return csr_hpmcounter3;
    //case 0xc04 : return csr_hpmcounter4;
    //case 0xc05 : return csr_hpmcounter5;
    //case 0xc06 : return csr_hpmcounter6;
    //case 0xc07 : return csr_hpmcounter7;
    //case 0xc08 : return csr_hpmcounter8;
    //case 0xc09 : return csr_hpmcounter9;
    //case 0xc0a : return csr_hpmcounter10;
    //case 0xc0b : return csr_hpmcounter11;
    //case 0xc0c : return csr_hpmcounter12;
    //case 0xc0d : return csr_hpmcounter13;
    //case 0xc0e : return csr_hpmcounter14;
    //case 0xc0f : return csr_hpmcounter15;
    //case 0xc10 : return csr_hpmcounter16;
    //case 0xc11 : return csr_hpmcounter17;
    //case 0xc12 : return csr_hpmcounter18;
    //case 0xc13 : return csr_hpmcounter19;
    //case 0xc14 : return csr_hpmcounter20;
    //case 0xc15 : return csr_hpmcounter21;
    //case 0xc16 : return csr_hpmcounter22;
    //case 0xc17 : return csr_hpmcounter23;
    //case 0xc18 : return csr_hpmcounter24;
    //case 0xc19 : return csr_hpmcounter25;
    //case 0xc1a : return csr_hpmcounter26;
    //case 0xc1b : return csr_hpmcounter27;
    //case 0xc1c : return csr_hpmcounter28;
    //case 0xc1d : return csr_hpmcounter29;
    //case 0xc1e : return csr_hpmcounter30;
    //case 0xc1f : return csr_hpmcounter31;
    //case 0xc80 : return csr_cycleh;
    //case 0xc81 : return csr_timeh;
    //case 0xc82 : return csr_instreth;
    //case 0xc83 : return csr_hpmcounter3h;
    //case 0xc84 : return csr_hpmcounter4h;
    //case 0xc85 : return csr_hpmcounter5h;
    //case 0xc86 : return csr_hpmcounter6h;
    //case 0xc87 : return csr_hpmcounter7h;
    //case 0xc88 : return csr_hpmcounter8h;
    //case 0xc89 : return csr_hpmcounter9h;
    //case 0xc8a : return csr_hpmcounter10h;
    //case 0xc8b : return csr_hpmcounter11h;
    //case 0xc8c : return csr_hpmcounter12h;
    //case 0xc8d : return csr_hpmcounter13h;
    //case 0xc8e : return csr_hpmcounter14h;
    //case 0xc8f : return csr_hpmcounter15h;
    //case 0xc90 : return csr_hpmcounter16h;
    //case 0xc91 : return csr_hpmcounter17h;
    //case 0xc92 : return csr_hpmcounter18h;
    //case 0xc93 : return csr_hpmcounter19h;
    //case 0xc94 : return csr_hpmcounter20h;
    //case 0xc95 : return csr_hpmcounter21h;
    //case 0xc96 : return csr_hpmcounter22h;
    //case 0xc97 : return csr_hpmcounter23h;
    //case 0xc98 : return csr_hpmcounter24h;
    //case 0xc99 : return csr_hpmcounter25h;
    //case 0xc9a : return csr_hpmcounter26h;
    //case 0xc9b : return csr_hpmcounter27h;
    //case 0xc9c : return csr_hpmcounter28h;
    //case 0xc9d : return csr_hpmcounter29h;
    //case 0xc9e : return csr_hpmcounter30h;
    //case 0xc9f : return csr_hpmcounter31h;
    /* RISCV supervisor registers */
    case 0x100 : return csr_sstatus;
    //case 0x102 : return csr_sedeleg;
    //case 0x103 : return csr_sideleg;
    case 0x104 : return csr_sie;
    case 0x105 : return csr_stvec;
    case 0x106 : return csr_scounteren;
    case 0x140 : return csr_sscratch;
    case 0x141 : return csr_sepc;
    case 0x142 : return csr_scause;
    case 0x143 : return csr_stval;
    case 0x144 : return csr_sip;
    case 0x180 : return csr_satp;
    /* RISCV machine registers */
    case 0xf11 : return csr_mvendorid;
    case 0xf12 : return csr_marchid;
    case 0xf13 : return csr_mimpid;
    case 0xf14 : return csr_mhartid;
    case 0x300 : return csr_mstatus;
    case 0x301 : return csr_misa;
    case 0x302 : return csr_medeleg;
    case 0x303 : return csr_mideleg;
    case 0x304 : return csr_mie;
    case 0x305 : return csr_mtvec;
    case 0x306 : return csr_mcounteren;
    case 0x340 : return csr_mscratch;
    case 0x341 : return csr_mepc;
    case 0x342 : return csr_mcause;
    case 0x343 : return csr_mtval;
    case 0x344 : return csr_mip;
    //case 0x3a0 : return csr_pmpcfg0;
    //case 0x3a1 : return csr_pmpcfg1;
    //case 0x3a2 : return csr_pmpcfg2;
    //case 0x3a3 : return csr_pmpcfg3;
    //case 0x3b0 : return csr_pmpaddr0;
    //case 0x3b1 : return csr_pmpaddr1;
    //case 0x3b2 : return csr_pmpaddr2;
    //case 0x3b3 : return csr_pmpaddr3;
    //case 0x3b4 : return csr_pmpaddr4;
    //case 0x3b5 : return csr_pmpaddr5;
    //case 0x3b6 : return csr_pmpaddr6;
    //case 0x3b7 : return csr_pmpaddr7;
    //case 0x3b8 : return csr_pmpaddr8;
    //case 0x3b9 : return csr_pmpaddr9;
    //case 0x3ba : return csr_pmpaddr10;
    //case 0x3bb : return csr_pmpaddr11;
    //case 0x3bc : return csr_pmpaddr12;
    //case 0x3bd : return csr_pmpaddr13;
    //case 0x3be : return csr_pmpaddr14;
    //case 0x3bf : return csr_pmpaddr15;
    case 0xb00 : return csr_mcycle;
    case 0xb02 : return csr_minstret;
    //case 0xb03 : return csr_mhpmcounter3;
    //case 0xb04 : return csr_mhpmcounter4;
    //case 0xb05 : return csr_mhpmcounter5;
    //case 0xb06 : return csr_mhpmcounter6;
    //case 0xb07 : return csr_mhpmcounter7;
    //case 0xb08 : return csr_mhpmcounter8;
    //case 0xb09 : return csr_mhpmcounter9;
    //case 0xb0a : return csr_mhpmcounter10;
    //case 0xb0b : return csr_mhpmcounter11;
    //case 0xb0c : return csr_mhpmcounter12;
    //case 0xb0d : return csr_mhpmcounter13;
    //case 0xb0e : return csr_mhpmcounter14;
    //case 0xb0f : return csr_mhpmcounter15;
    //case 0xb10 : return csr_mhpmcounter16;
    //case 0xb11 : return csr_mhpmcounter17;
    //case 0xb12 : return csr_mhpmcounter18;
    //case 0xb13 : return csr_mhpmcounter19;
    //case 0xb14 : return csr_mhpmcounter20;
    //case 0xb15 : return csr_mhpmcounter21;
    //case 0xb16 : return csr_mhpmcounter22;
    //case 0xb17 : return csr_mhpmcounter23;
    //case 0xb18 : return csr_mhpmcounter24;
    //case 0xb19 : return csr_mhpmcounter25;
    //case 0xb1a : return csr_mhpmcounter26;
    //case 0xb1b : return csr_mhpmcounter27;
    //case 0xb1c : return csr_mhpmcounter28;
    //case 0xb1d : return csr_mhpmcounter29;
    //case 0xb1e : return csr_mhpmcounter30;
    //case 0xb1f : return csr_mhpmcounter31;
    //case 0xb80 : return csr_mcycleh;
    //case 0xb82 : return csr_minstreth;
    //case 0xb83 : return csr_mhpmcounter3h;
    //case 0xb84 : return csr_mhpmcounter4h;
    //case 0xb85 : return csr_mhpmcounter5h;
    //case 0xb86 : return csr_mhpmcounter6h;
    //case 0xb87 : return csr_mhpmcounter7h;
    //case 0xb88 : return csr_mhpmcounter8h;
    //case 0xb89 : return csr_mhpmcounter9h;
    //case 0xb8a : return csr_mhpmcounter10h;
    //case 0xb8b : return csr_mhpmcounter11h;
    //case 0xb8c : return csr_mhpmcounter12h;
    //case 0xb8d : return csr_mhpmcounter13h;
    //case 0xb8e : return csr_mhpmcounter14h;
    //case 0xb8f : return csr_mhpmcounter15h;
    //case 0xb90 : return csr_mhpmcounter16h;
    //case 0xb91 : return csr_mhpmcounter17h;
    //case 0xb92 : return csr_mhpmcounter18h;
    //case 0xb93 : return csr_mhpmcounter19h;
    //case 0xb94 : return csr_mhpmcounter20h;
    //case 0xb95 : return csr_mhpmcounter21h;
    //case 0xb96 : return csr_mhpmcounter22h;
    //case 0xb97 : return csr_mhpmcounter23h;
    //case 0xb98 : return csr_mhpmcounter24h;
    //case 0xb99 : return csr_mhpmcounter25h;
    //case 0xb9a : return csr_mhpmcounter26h;
    //case 0xb9b : return csr_mhpmcounter27h;
    //case 0xb9c : return csr_mhpmcounter28h;
    //case 0xb9d : return csr_mhpmcounter29h;
    //case 0xb9e : return csr_mhpmcounter30h;
    //case 0xb9f : return csr_mhpmcounter31h;
    //case 0x323 : return csr_mhpmevent3;
    //case 0x324 : return csr_mhpmevent4;
    //case 0x325 : return csr_mhpmevent5;
    //case 0x326 : return csr_mhpmevent6;
    //case 0x327 : return csr_mhpmevent7;
    //case 0x328 : return csr_mhpmevent8;
    //case 0x329 : return csr_mhpmevent9;
    //case 0x32a : return csr_mhpmevent10;
    //case 0x32b : return csr_mhpmevent11;
    //case 0x32c : return csr_mhpmevent12;
    //case 0x32d : return csr_mhpmevent13;
    //case 0x32e : return csr_mhpmevent14;
    //case 0x32f : return csr_mhpmevent15;
    //case 0x330 : return csr_mhpmevent16;
    //case 0x331 : return csr_mhpmevent17;
    //case 0x332 : return csr_mhpmevent18;
    //case 0x333 : return csr_mhpmevent19;
    //case 0x334 : return csr_mhpmevent20;
    //case 0x335 : return csr_mhpmevent21;
    //case 0x336 : return csr_mhpmevent22;
    //case 0x337 : return csr_mhpmevent23;
    //case 0x338 : return csr_mhpmevent24;
    //case 0x339 : return csr_mhpmevent25;
    //case 0x33a : return csr_mhpmevent26;
    //case 0x33b : return csr_mhpmevent27;
    //case 0x33c : return csr_mhpmevent28;
    //case 0x33d : return csr_mhpmevent29;
    //case 0x33e : return csr_mhpmevent30;
    //case 0x33f : return csr_mhpmevent31;
    /* RISCV debug registers */
    //case 0x7a0 : return csr_tselect;
    //case 0x7a1 : return csr_tdata1;
    //case 0x7a2 : return csr_tdata2;
    //case 0x7a3 : return csr_tdata3;
    //case 0x7b0 : return csr_dcsr;
    //case 0x7b1 : return csr_dpc;
    //case 0x7b2 : return csr_dscratch;
    /* Esperanto user CSRs */
    case 0x800 : return csr_tensor_reduce;
    case 0x801 : return csr_tensor_fma;
    case 0x802 : return csr_tensor_conv_size;
    case 0x803 : return csr_tensor_conv_ctrl;
    case 0x804 : return csr_tensor_coop;
    case 0x805 : return csr_tensor_mask;
    case 0x806 : return csr_tensor_quant;   /* TODO: add to binutils */
    case 0x807 : return csr_tex_send;
    case 0x808 : return csr_tensor_error;
    case 0x810 : return csr_scratchpad_ctrl;
#if 1 /* TODO: uncomment, usr_cache_op is replaced by individual CacheOp CSRs */
    case 0x81f : return csr_prefetch_va;
#else
    case 0x81f : return csr_usr_cache_op;
#endif
    case 0x820 : return csr_flb0;
    case 0x821 : return csr_fcc;            /* TODO: add to binutils */
    case 0x822 : return csr_stall;          /* TODO: add to binutils */
    case 0x830 : return csr_tensor_wait;
    case 0x83f : return csr_tensor_load;
    // TODO: case 0x840 : return csr_gsc_progress;   /* TODO: add to binutils */
    case 0x85f : return csr_tensor_load_l2;
    case 0x87f : return csr_tensor_store;
    case 0x89f : return csr_evict_va;
    case 0x8bf : return csr_flush_va;
    case 0x8cc : return csr_umsg_port0;     /* TODO: remove once everything is migrated to new port spec */
    case 0x8cd : return csr_umsg_port1;     /* TODO: remove once everything is migrated to new port spec */
    case 0x8ce : return csr_umsg_port2;     /* TODO: remove once everything is migrated to new port spec */
    case 0x8cf : return csr_umsg_port3;     /* TODO: remove once everything is migrated to new port spec */
    case 0x8d0 : return csr_validation0;
    case 0x8d1 : return csr_validation1;
    case 0x8d2 : return csr_validation2;
    case 0x8d3 : return csr_validation3;
    case 0x8d5 : return csr_sleep_txfma_27; /* TODO: add to binutils */
    case 0x8df : return csr_lock_va;
    case 0x8ff : return csr_unlock_va;
    case 0x7fd : return csr_lock_sw;
    case 0x7ff : return csr_unlock_sw;
    case 0xcc8 : return csr_porthead0;
    case 0xcc9 : return csr_porthead1;
    case 0xcca : return csr_porthead2;
    case 0xccb : return csr_porthead3;
    case 0xccc : return csr_portheadnb0;
    case 0xccd : return csr_portheadnb1;
    case 0xcce : return csr_portheadnb2;
    case 0xccf : return csr_portheadnb3;
    case 0xcd0 : return csr_hartid;         /* TODO: add to binutils */
    /* Esperanto supervisor CSRs */
    case 0x51f : return csr_sys_cache_op;   /* TODO: remove, it is replaced by individual CacheOp CSRs */
    case 0x7e0 : return csr_mcache_control;   /* TODO: remove, it is replaced by individual CacheOp CSRs */
    case 0x7f9 : return csr_evict_sw;
    case 0x7fb : return csr_flush_sw;
    case 0x9cc : return csr_portctrl0;
    case 0x9cd : return csr_portctrl1;
    case 0x9ce : return csr_portctrl2;
    case 0x9cf : return csr_portctrl3;
    /* Esperanto machine CSRs */
    //case 0x7cb : return csr_icache_ctrl;
    //case 0x7cc : return csr_write_ctrl;
    case 0x7cd : return csr_minstmask;
    case 0x7ce : return csr_minstmatch;
    //case 0x7cf : return csr_amofence_ctrl;
    case 0x7d0 : return csr_flush_icache;
    case 0x7d1 : return csr_msleep_txfma_27; /* TODO: rename in binutils */
    case 0x7d2 : return csr_menable_shadows; /* TODO: add to binutils */
    case 0x7d3 : return csr_excl_mode;       /* TODO: add to binutils */
    /* Unimplemented register */
    default    : return csr_unknown;
    }
}

// NB: not reallly a helper function; get_csr_enum() is the external version
// of imm2csr() so that the latter can be inlined for speed
csr get_csr_enum(uint16_t imm) {
    return imm2csr(imm);
}


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
        (exec) (inst.rd(), imm2csr(inst.csrimm()), inst.rs1()); \
    }

#define FMT_RD_CSR_UIMM5(name, exec) \
    void name(insn_t inst) { \
        (exec) (inst.rd(), imm2csr(inst.csrimm()), inst.uimm5()); \
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

#define FMT_RD_RS1_SHAMT6(name, exec) \
    void name(insn_t inst) { \
        (exec) (inst.rd(), inst.rs1(), inst.shamt6()); \
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

//////////////////////////////////////////////////

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

/// ----- RV64F ---------------------------------------------
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
FMT_FD_FS1_RS2        (insn_fghg_ps,        fgwg_ps)
FMT_FD_FS1_RS2        (insn_fgbg_ps,        fgwg_ps)
FMT_FD_FS1_RS2        (insn_fgwl_ps,        fgwl_ps)
FMT_FD_FS1_RS2        (insn_fghl_ps,        fgwg_ps)
FMT_FD_FS1_RS2        (insn_fgbl_ps,        fgwg_ps)
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
FMT_FD_FS1            (insn_frcpfxp_ps,     frcpfxp_ps)
FMT_FD_FS1            (insn_frsq_ps,        frsq_ps)
FMT_FD_FS1            (insn_fsin_ps,        fsin_ps)
FMT_FD_FS1_FS2        (insn_cubeface_ps,    cubeface_ps)
FMT_FD_FS1_FS2        (insn_cubefaceidx_ps, cubefaceidx_ps)
FMT_FD_FS1_FS2        (insn_cubesgnsc_ps,   cubesgnsc_ps)
FMT_FD_FS1_FS2        (insn_cubesgntc_ps,   cubesgntc_ps)
FMT_FD_FS1_FS2        (insn_frcp_fix_rast,  frcp_fix_rast)
FMT_FD_FS1_RM         (insn_fcvt_ps_rast,   fcvt_ps_rast)
FMT_FD_FS1_RM         (insn_ffrc_ps,        ffrc_ps)
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
