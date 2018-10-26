#ifndef _EMU_H
#define _EMU_H

#include <list>

#include "emu_defines.h"
#include "instruction.h"
#include "testLog.h"

#ifdef IPC
#undef IPC
#define IPC(a) { a }
#define DISASM 1
#else
#define IPC(a)
#endif

#ifdef DISASM
#undef DISASM
#define DISASM(a) do { a } while (0)
#else
#define DISASM(a) do { } while (0)
#endif

// Used to access different threads transparently
#define XREGS xregs[current_thread]
#define FREGS fregs[current_thread]
#define MREGS mregs[current_thread]

// Processor state
extern xdata xregs[EMU_NUM_THREADS][32];
extern fdata fregs[EMU_NUM_THREADS][32];
extern mdata mregs[EMU_NUM_THREADS][8];

// Processor configuration
extern uint8_t in_sysemu;
extern uint32_t current_thread;
extern void set_core_type(et_core_t core);
extern et_core_t get_core_type();

// Configure the emulation environment
extern void init_emu(int debug, int fakesam, enum logLevel log_level);
extern void log_only_minion(int32_t m);

// Helpers
extern void print_comment(const char *comm);
extern void init_stack();
extern void initcsr(uint32_t thread);           // init all CSRs
extern uint64_t xget(uint64_t src1);
extern void init(xreg dst, uint64_t val);       // init general purpose register
extern void fpinit(freg dst, uint64_t val[2]);  // init vector register
extern void minit(mreg dst, uint64_t val);      // init mask register

// Processor state manipulation
extern void set_pc(uint64_t pc);
extern instruction * get_inst();
extern void set_thread(uint32_t thread);
extern uint32_t get_thread();
extern uint32_t get_mask(unsigned maskNr);

// Main memory accessors
extern void set_memory_funcs(void * func_memread8_, void * func_memread16_,
                             void * func_memread32_, void * func_memread64_,
                             void * func_memwrite8_, void * func_memwrite16_,
                             void * func_memwrite32_, void * func_memwrite64_);

// Traps
extern void take_trap(const trap_t& t);

// Illegal instruction encodings will execute this
extern void unknown(const char* comm = 0);

// Instruction encodings that match minstmatch/minstmask will execute this
extern void check_minst_match(uint32_t bits);

// ----- RV64I emulation -------------------------------------------------------

extern void beq  (xreg src1, xreg src2, int imm, const char* comm = 0);
extern void bne  (xreg src1, xreg src2, int imm, const char* comm = 0);
extern void blt  (xreg src1, xreg src2, int imm, const char* comm = 0);
extern void bge  (xreg src1, xreg src2, int imm, const char* comm = 0);
extern void bltu (xreg src1, xreg src2, int imm, const char* comm = 0);
extern void bgeu (xreg src1, xreg src2, int imm, const char* comm = 0);

extern void jalr (xreg dst, xreg src1, int imm, const char* comm = 0);
extern void jal  (xreg dst, int imm, const char* comm = 0);

extern void lui   (xreg dst, int imm, const char* comm = 0);
extern void auipc (xreg dst, int imm, const char* comm = 0);

extern void addi  (xreg dst, xreg src1, int imm, const char* comm = 0);
extern void slli  (xreg dst, xreg src1, int imm, const char* comm = 0);
extern void slti  (xreg dst, xreg src1, int imm, const char* comm = 0);
extern void sltiu (xreg dst, xreg src1, int imm, const char* comm = 0);
extern void xori  (xreg dst, xreg src1, int imm, const char* comm = 0);
extern void srli  (xreg dst, xreg src1, int imm, const char* comm = 0);
extern void srai  (xreg dst, xreg src1, int imm, const char* comm = 0);
extern void ori   (xreg dst, xreg src1, int imm, const char* comm = 0);
extern void andi  (xreg dst, xreg src1, int imm, const char* comm = 0);

extern void add  (xreg dst, xreg src1, xreg src2, const char* comm = 0);
extern void sub  (xreg dst, xreg src1, xreg src2, const char* comm = 0);
extern void sll  (xreg dst, xreg src1, xreg src2, const char* comm = 0);
extern void slt  (xreg dst, xreg src1, xreg src2, const char* comm = 0);
extern void sltu (xreg dst, xreg src1, xreg src2, const char* comm = 0);
extern void xor_ (xreg dst, xreg src1, xreg src2, const char* comm = 0);
extern void srl  (xreg dst, xreg src1, xreg src2, const char* comm = 0);
extern void sra  (xreg dst, xreg src1, xreg src2, const char* comm = 0);
extern void or_  (xreg dst, xreg src1, xreg src2, const char* comm = 0);
extern void and_ (xreg dst, xreg src1, xreg src2, const char* comm = 0);

extern void addiw (xreg dst, xreg src1, int imm, const char* comm = 0);
extern void slliw (xreg dst, xreg src1, int imm, const char* comm = 0);
extern void srliw (xreg dst, xreg src1, int imm, const char* comm = 0);
extern void sraiw (xreg dst, xreg src1, int imm, const char* comm = 0);

extern void addw (xreg dst, xreg src1, xreg src2, const char* comm = 0);
extern void subw (xreg dst, xreg src1, xreg src2, const char* comm = 0);
extern void sllw (xreg dst, xreg src1, xreg src2, const char* comm = 0);
extern void srlw (xreg dst, xreg src1, xreg src2, const char* comm = 0);
extern void sraw (xreg dst, xreg src1, xreg src2, const char* comm = 0);

extern void lb  (xreg dst, int off, xreg base, const char* comm = 0);
extern void lh  (xreg dst, int off, xreg base, const char* comm = 0);
extern void lw  (xreg dst, int off, xreg base, const char* comm = 0);
extern void ld  (xreg dst, int off, xreg base, const char* comm = 0);
extern void lbu (xreg dst, int off, xreg base, const char* comm = 0);
extern void lhu (xreg dst, int off, xreg base, const char* comm = 0);
extern void lwu (xreg dst, int off, xreg base, const char* comm = 0);

extern void sd (xreg src1, int off, xreg base, const char* comm = 0);
extern void sw (xreg src1, int off, xreg base, const char* comm = 0);
extern void sh (xreg src1, int off, xreg base, const char* comm = 0);
extern void sb (xreg src1, int off, xreg base, const char* comm = 0);

extern void fence   (const char* comm = 0);
extern void fence_i (const char* comm = 0);

extern void sfence_vma(const char* comm = 0);

// ----- RV64M emulation -------------------------------------------------------

extern void mul    (xreg dst, xreg src1, xreg src2, const char* comm = 0);
extern void mulh   (xreg dst, xreg src1, xreg src2, const char* comm = 0);
extern void mulhsu (xreg dst, xreg src1, xreg src2, const char* comm = 0);
extern void mulhu  (xreg dst, xreg src1, xreg src2, const char* comm = 0);
extern void div_   (xreg dst, xreg src1, xreg src2, const char* comm = 0);
extern void divu   (xreg dst, xreg src1, xreg src2, const char* comm = 0);
extern void rem    (xreg dst, xreg src1, xreg src2, const char* comm = 0);
extern void remu   (xreg dst, xreg src1, xreg src2, const char* comm = 0);

extern void mulw   (xreg dst, xreg src1, xreg src2, const char* comm = 0);
extern void divw   (xreg dst, xreg src1, xreg src2, const char* comm = 0);
extern void divuw  (xreg dst, xreg src1, xreg src2, const char* comm = 0);
extern void remw   (xreg dst, xreg src1, xreg src2, const char* comm = 0);
extern void remuw  (xreg dst, xreg src1, xreg src2, const char* comm = 0);

// ----- RV64A emulation -------------------------------------------------------

extern void amoadd_w  (xreg dst, xreg src1, xreg src2, const char* comm = 0);
extern void amoxor_w  (xreg dst, xreg src1, xreg src2, const char* comm = 0);
extern void amoor_w   (xreg dst, xreg src1, xreg src2, const char* comm = 0);
extern void amoand_w  (xreg dst, xreg src1, xreg src2, const char* comm = 0);
extern void amomin_w  (xreg dst, xreg src1, xreg src2, const char* comm = 0);
extern void amomax_w  (xreg dst, xreg src1, xreg src2, const char* comm = 0);
extern void amominu_w (xreg dst, xreg src1, xreg src2, const char* comm = 0);
extern void amomaxu_w (xreg dst, xreg src1, xreg src2, const char* comm = 0);
extern void amoswap_w (xreg dst, xreg src1, xreg src2, const char* comm = 0);
//extern void lr_w(...)
//extern void sc_w(...)

extern void amoadd_d  (xreg dst, xreg src1, xreg src2, const char* comm = 0);
extern void amoxor_d  (xreg dst, xreg src1, xreg src2, const char* comm = 0);
extern void amoor_d   (xreg dst, xreg src1, xreg src2, const char* comm = 0);
extern void amoand_d  (xreg dst, xreg src1, xreg src2, const char* comm = 0);
extern void amomin_d  (xreg dst, xreg src1, xreg src2, const char* comm = 0);
extern void amomax_d  (xreg dst, xreg src1, xreg src2, const char* comm = 0);
extern void amominu_d (xreg dst, xreg src1, xreg src2, const char* comm = 0);
extern void amomaxu_d (xreg dst, xreg src1, xreg src2, const char* comm = 0);
extern void amoswap_d (xreg dst, xreg src1, xreg src2, const char* comm = 0);
//extern void lr_d(...)
//extern void sc_d(...)

// ----- SYSTEM emulation ------------------------------------------------------

extern void ecall(const char* comm = 0);
extern void ebreak(const char* comm = 0);
extern void sret(const char* comm = 0);
extern void mret(const char* comm = 0);
extern void wfi(const char* comm = 0);
extern void csrrw(xreg dst, csr src1, xreg src2, const char* comm = 0);
extern void csrrs(xreg dst, csr src1, xreg src2, const char* comm = 0);
extern void csrrc(xreg dst, csr src1, xreg src2, const char* comm = 0);
extern void csrrwi(xreg dst, csr src1, uint64_t imm, const char* comm = 0);
extern void csrrsi(xreg dst, csr src1, uint64_t imm, const char* comm = 0);
extern void csrrci(xreg dst, csr src1, uint64_t imm, const char* comm = 0);

// ----- RV64C emulation -------------------------------------------------------

extern void c_jalr (xreg dst, xreg src1, int imm, const char* comm = 0);
extern void c_jal  (xreg dst, int imm, const char* comm = 0);

// ----- RV64F emulation -------------------------------------------------------

extern void fadd_s   (freg dst, freg src1, freg src2, rounding_mode rm, const char* comm = 0);
extern void fsub_s   (freg dst, freg src1, freg src2, rounding_mode rm, const char* comm = 0);
extern void fmul_s   (freg dst, freg src1, freg src2, rounding_mode rm, const char* comm = 0);
extern void fdiv_s   (freg dst, freg src1, freg src2, rounding_mode rm, const char* comm = 0);
extern void fsgnj_s  (freg dst, freg src1, freg src2, const char* comm = 0);
extern void fsgnjn_s (freg dst, freg src1, freg src2, const char* comm = 0);
extern void fsgnjx_s (freg dst, freg src1, freg src2, const char* comm = 0);
extern void fmin_s   (freg dst, freg src1, freg src2, const char* comm = 0);
extern void fmax_s   (freg dst, freg src1, freg src2, const char* comm = 0);
extern void fsqrt_s  (freg dst, freg src1, rounding_mode rm, const char* comm = 0);

extern void feq_s (xreg dst, freg src1, freg src2, const char* comm = 0);
extern void fle_s (xreg dst, freg src1, freg src2, const char* comm = 0);
extern void flt_s (xreg dst, freg src1, freg src2, const char* comm = 0);

extern void fcvt_w_s  (xreg dst, freg src1, rounding_mode rm, const char* comm = 0);
extern void fcvt_wu_s (xreg dst, freg src1, rounding_mode rm, const char* comm = 0);
extern void fcvt_l_s  (xreg dst, freg src1, rounding_mode rm, const char* comm = 0);
extern void fcvt_lu_s (xreg dst, freg src1, rounding_mode rm, const char* comm = 0);
extern void fmv_x_w   (xreg dst, freg src1, const char* comm = 0);
extern void fclass_s  (xreg dst, freg src1, const char* comm = 0);

extern void fcvt_s_w  (freg dst, xreg src1, rounding_mode rm, const char* comm = 0);
extern void fcvt_s_wu (freg dst, xreg src1, rounding_mode rm, const char* comm = 0);
extern void fcvt_s_l  (freg dst, xreg src1, rounding_mode rm, const char* comm = 0);
extern void fcvt_s_lu (freg dst, xreg src1, rounding_mode rm, const char* comm = 0);
extern void fmv_w_x   (freg dst, xreg src1, const char* comm = 0);

extern void flw (freg dst, int off, xreg base, const char* comm = 0);

extern void fsw (freg src1, int off, xreg base, const char* comm = 0);

extern void fmadd_s  (freg dst, freg src1, freg src2, freg src3, rounding_mode rm, const char* comm = 0);
extern void fmsub_s  (freg dst, freg src1, freg src2, freg src3, rounding_mode rm, const char* comm = 0);
extern void fnmsub_s (freg dst, freg src1, freg src2, freg src3, rounding_mode rm, const char* comm = 0);
extern void fnmadd_s (freg dst, freg src1, freg src2, freg src3, rounding_mode rm, const char* comm = 0);

// ----- Esperanto mask extension ----------------------------------------------

extern void maskand (mreg dst, mreg src1, mreg src2, const char* comm = 0);
extern void maskor  (mreg dst, mreg src1, mreg src2, const char* comm = 0);
extern void maskxor (mreg dst, mreg src1, mreg src2, const char* comm = 0);

extern void masknot (mreg dst, mreg src1, const char* comm = 0);

extern void mova_x_m (xreg dst, const char* comm = 0);
extern void mova_m_x (xreg src1, const char* comm = 0);

extern void mov_m_x  (mreg dst, xreg src1, uint32_t imm, const char* comm = 0);

extern void maskpopc  (xreg dst, mreg src1, const char* comm = 0);
extern void maskpopcz (xreg dst, mreg src1, const char* comm = 0);

extern void maskpopc_rast (xreg dst, mreg src1, mreg src2, uint32_t imm, const char* comm = 0);

// ----- Esperanto packed-single extension -------------------------------------

// Load and store

extern void flw_ps (freg dst, int off, xreg base, const char* comm = 0);
extern void flq2   (freg dst, int off, xreg base, const char* comm = 0);

extern void fsw_ps (freg src1, int off, xreg base, const char* comm = 0);
extern void fsq2   (freg src1, int off, xreg base, const char* comm = 0);
extern void fswg_ps (freg src1, xreg base, const char* comm = 0);

// Broadcast

extern void fbc_ps  (freg dst, int off, xreg base, const char* comm = 0);
extern void fbci_ps (freg dst, uint32_t imm, const char* comm = 0);
extern void fbcx_ps (freg dst, xreg src, const char* comm = 0);

// Gather and scatter

extern void fgb_ps    (freg dst, freg src1, xreg base, const char* comm = 0);
extern void fgh_ps    (freg dst, freg src1, xreg base, const char* comm = 0);
extern void fgw_ps    (freg dst, freg src1, xreg base, const char* comm = 0);

extern void fscb_ps   (freg src1, freg src2, xreg base, const char* comm = 0);
extern void fsch_ps   (freg src1, freg src2, xreg base, const char* comm = 0);
extern void fscw_ps   (freg src1, freg src2, xreg base, const char* comm = 0);

extern void fg32b_ps  (freg dst, xreg src1, xreg src2, const char* comm = 0);
extern void fg32h_ps  (freg dst, xreg src1, xreg src2, const char* comm = 0);
extern void fg32w_ps  (freg dst, xreg src1, xreg src2, const char* comm = 0);

extern void fsc32b_ps (freg src3, xreg src1, xreg src2, const char* comm = 0);
extern void fsc32h_ps (freg src3, xreg src1, xreg src2, const char* comm = 0);
extern void fsc32w_ps (freg src3, xreg src1, xreg src2, const char* comm = 0);

// Computational (follows RV64F)

extern void fadd_ps   (freg dst, freg src1, freg src2, rounding_mode rm, const char* comm = 0);
extern void fsub_ps   (freg dst, freg src1, freg src2, rounding_mode rm, const char* comm = 0);
extern void fmul_ps   (freg dst, freg src1, freg src2, rounding_mode rm, const char* comm = 0);
extern void fdiv_ps   (freg dst, freg src1, freg src2, rounding_mode rm, const char* comm = 0);
extern void fsgnj_ps  (freg dst, freg src1, freg src2, const char* comm = 0);
extern void fsgnjn_ps (freg dst, freg src1, freg src2, const char* comm = 0);
extern void fsgnjx_ps (freg dst, freg src1, freg src2, const char* comm = 0);
extern void fmin_ps   (freg dst, freg src1, freg src2, const char* comm = 0);
extern void fmax_ps   (freg dst, freg src1, freg src2, const char* comm = 0);
extern void fsqrt_ps  (freg dst, freg src1, rounding_mode rm, const char* comm = 0);

extern void feq_ps (freg dst, freg src1, freg src2, const char* comm = 0);
extern void fle_ps (freg dst, freg src1, freg src2, const char* comm = 0);
extern void flt_ps (freg dst, freg src1, freg src2, const char* comm = 0);

extern void feqm_ps (mreg dst, freg src1, freg src2, const char* comm = 0);
extern void flem_ps (mreg dst, freg src1, freg src2, const char* comm = 0);
extern void fltm_ps (mreg dst, freg src1, freg src2, const char* comm = 0);

extern void fsetm_ps (mreg dst, freg src1, const char* comm = 0);

extern void fcmov_ps  (freg dst, freg src1, freg src2, freg src3, const char* comm = 0);
extern void fcmovm_ps (freg dst, freg src1, freg src2, const char* comm = 0);
extern void fmvz_x_ps (xreg dst, freg src1, uint8_t index, const char* comm = 0);
extern void fmvs_x_ps (xreg dst, freg src1, uint8_t index, const char* comm = 0);
extern void fswizz_ps (freg dst, freg src1, uint8_t imm, const char* comm = 0);

extern void fcvt_pw_ps  (freg dst, freg src1, rounding_mode rm, const char* comm = 0);
extern void fcvt_pwu_ps (freg dst, freg src1, rounding_mode rm, const char* comm = 0);
//extern void fcvt_pl_ps(...)
//extern void fcvt_plu_ps(...)
extern void fclass_ps   (freg dst, freg src1, const char* comm = 0);

extern void fcvt_ps_pw  (freg dst, freg src1, rounding_mode rm, const char* comm = 0);
extern void fcvt_ps_pwu (freg dst, freg src1, rounding_mode rm, const char* comm = 0);
//extern void fcvt_ps_pl(...)
//extern void fcvt_ps_plu(...)

extern void fmadd_ps  (freg dst, freg src1, freg src2, freg src3, rounding_mode rm, const char* comm = 0);
extern void fmsub_ps  (freg dst, freg src1, freg src2, freg src3, rounding_mode rm, const char* comm = 0);
extern void fnmsub_ps (freg dst, freg src1, freg src2, freg src3, rounding_mode rm, const char* comm = 0);
extern void fnmadd_ps (freg dst, freg src1, freg src2, freg src3, rounding_mode rm, const char* comm = 0);

// Graphics upconvert

extern void fcvt_ps_f16  (freg dst, freg src1, const char* comm = 0);
extern void fcvt_ps_f11  (freg dst, freg src1, const char* comm = 0);
extern void fcvt_ps_f10  (freg dst, freg src1, const char* comm = 0);

extern void fcvt_ps_un24 (freg dst, freg src1, const char* comm = 0);
extern void fcvt_ps_un16 (freg dst, freg src1, const char* comm = 0);
extern void fcvt_ps_un10 (freg dst, freg src1, const char* comm = 0);
extern void fcvt_ps_un8  (freg dst, freg src1, const char* comm = 0);
extern void fcvt_ps_un2  (freg dst, freg src1, const char* comm = 0);

//extern void fcvt_ps_sn24 (freg dst, freg src1, const char* comm = 0);
extern void fcvt_ps_sn16 (freg dst, freg src1, const char* comm = 0);
//extern void fcvt_ps_sn10 (freg dst, freg src1, const char* comm = 0);
extern void fcvt_ps_sn8  (freg dst, freg src1, const char* comm = 0);
//extern void fcvt_ps_sn2  (freg dst, freg src1, const char* comm = 0);

// Graphics downconvert

extern void fcvt_f16_ps  (freg dst, freg src1, const char* comm = 0);
extern void fcvt_f11_ps  (freg dst, freg src1, const char* comm = 0);
extern void fcvt_f10_ps  (freg dst, freg src1, const char* comm = 0);

extern void fcvt_un24_ps (freg dst, freg src1, const char* comm = 0);
extern void fcvt_un16_ps (freg dst, freg src1, const char* comm = 0);
extern void fcvt_un10_ps (freg dst, freg src1, const char* comm = 0);
extern void fcvt_un8_ps  (freg dst, freg src1, const char* comm = 0);
extern void fcvt_un2_ps  (freg dst, freg src1, const char* comm = 0);

//extern void fcvt_sn24_ps (freg dst, freg src1, const char* comm = 0);
extern void fcvt_sn16_ps (freg dst, freg src1, const char* comm = 0);
//extern void fcvt_sn10_ps (freg dst, freg src1, const char* comm = 0);
extern void fcvt_sn8_ps  (freg dst, freg src1, const char* comm = 0);
//extern void fcvt_sn2_ps (freg dst, freg src1, const char* comm = 0);

// Graphics additional

extern void fsin_ps   (freg dst, freg src1, const char* comm = 0);
//extern void fcos_ps   (freg dst, freg src1, const char* comm = 0);
extern void fexp_ps   (freg dst, freg src1, const char* comm = 0);
extern void flog_ps   (freg dst, freg src1, const char* comm = 0);
extern void ffrc_ps   (freg dst, freg src1, rounding_mode rm, const char* comm = 0);
extern void fround_ps (freg dst, freg src1, rounding_mode rm, const char* comm = 0);
extern void frcp_ps   (freg dst, freg src1, const char* comm = 0);
extern void frsq_ps   (freg dst, freg src1, const char* comm = 0);
//extern void fltabs_ps (freg dst, freg src1, freg src2, rounding_mode rm, const char* comm = 0);

// FIXME: THIS INSTRUCTION IS OBSOLETE
extern void frcpfxp_ps (freg dst, freg src1, const char* comm = 0);

extern void cubeface_ps    (freg dst, freg src1, freg src2, const char* comm = 0);
extern void cubefaceidx_ps (freg dst, freg src1, freg src2, const char* comm = 0);
extern void cubesgnsc_ps   (freg dst, freg src1, freg src2, const char* comm = 0);
extern void cubesgntc_ps   (freg dst, freg src1, freg src2, const char* comm = 0);

extern void fcvt_ps_rast  (freg dst, freg src1, rounding_mode rm, const char* comm = 0);
extern void fcvt_rast_ps  (freg dst, freg src1, const char* comm = 0);
extern void frcp_fix_rast (freg dst, freg src1, freg src2, const char* comm = 0);

// Texture sampling
#include "txs.h"

// ----- Esperanto packed-integer extension ------------------------------------

// Broadcast

extern void fbci_pi (freg dst, uint32_t imm, const char* comm = 0);

// Computational (follows RV64I/RV64F + RV64M + min/max)

extern void feq_pi  (freg dst, freg src1, freg src2, const char* comm = 0);     // RV64I: beq
//extern void fne_pi  (freg dst, freg src1, freg src2, const char* comm = 0);   // RV64I: bne
extern void fle_pi  (freg dst, freg src1, freg src2, const char* comm = 0);     // RV64I: bge
extern void flt_pi  (freg dst, freg src1, freg src2, const char* comm = 0);     // RV64I: blt
//extern void fleu_pi (freg dst, freg src1, freg src2, const char* comm = 0);   // RV64I: bgeu
extern void fltu_pi (freg dst, freg src1, freg src2, const char* comm = 0);     // RV64I: bltu

//extern void feqm_pi  (mreg dst, freg src1, freg src2, const char* comm = 0);
//extern void fnem_pi  (mreg dst, freg src1, freg src2, const char* comm = 0);
//extern void flem_pi  (mreg dst, freg src1, freg src2, const char* comm = 0);
extern void fltm_pi  (mreg dst, freg src1, freg src2, const char* comm = 0);
//extern void fleum_pi (mreg dst, freg src1, freg src2, const char* comm = 0);
//extern void fltum_pi (mreg dst, freg src1, freg src2, const char* comm = 0);

extern void faddi_pi  (freg dst, freg src1, uint32_t imm, const char* comm = 0);
extern void fslli_pi  (freg dst, freg src1, uint32_t imm, const char* comm = 0);
//extern void fslti_pi  (freg dst, freg src1, freg src2, uint32_t imm, const char* comm = 0);
//extern void fsltiu_pi (freg dst, freg src1, freg src2, uint32_t imm, const char* comm = 0);
extern void fxori_pi  (freg dst, freg src1, uint32_t imm, const char* comm = 0);
extern void fsrli_pi  (freg dst, freg src1, uint32_t imm, const char* comm = 0);
extern void fsrai_pi  (freg dst, freg src1, uint32_t imm, const char* comm = 0);
extern void fori_pi   (freg dst, freg src1, uint32_t imm, const char* comm = 0);
extern void fandi_pi  (freg dst, freg src1, uint32_t imm, const char* comm = 0);

extern void fadd_pi  (freg dst, freg src1, freg src2, const char* comm = 0);
extern void fsub_pi  (freg dst, freg src1, freg src2, const char* comm = 0);
//extern void fslt_pi  (freg dst, freg src1, freg src2, const char* comm = 0);
//extern void fsltu_pi (freg dst, freg src1, freg src2, const char* comm = 0);
extern void fsll_pi  (freg dst, freg src1, freg src2, const char* comm = 0);
extern void fxor_pi  (freg dst, freg src1, freg src2, const char* comm = 0);
extern void fsrl_pi  (freg dst, freg src1, freg src2, const char* comm = 0);
extern void fsra_pi  (freg dst, freg src1, freg src2, const char* comm = 0);
extern void for_pi   (freg dst, freg src1, freg src2, const char* comm = 0);
extern void fand_pi  (freg dst, freg src1, freg src2, const char* comm = 0);

extern void fnot_pi  (freg dst, freg src1, const char* comm = 0);
extern void fsat8_pi (freg dst, freg src1, const char* comm = 0);
extern void fsatu8_pi (freg dst, freg src1, const char* comm = 0);

extern void fpackreph_pi (freg dst, freg src1, const char* comm = 0);
extern void fpackrepb_pi (freg dst, freg src1, const char* comm = 0);

extern void fmul_pi   (freg dst, freg src1, freg src2, const char* comm = 0);
extern void fmulh_pi  (freg dst, freg src1, freg src2, const char* comm = 0);
// fmulhsu_pi will not be implemented
extern void fmulhu_pi (freg dst, freg src1, freg src2, const char* comm = 0);
extern void fdiv_pi  (freg dst, freg src1, freg src2, const char* comm = 0);
extern void fdivu_pi (freg dst, freg src1, freg src2, const char* comm = 0);
extern void frem_pi  (freg st, freg src1, freg src2, const char* comm = 0);
extern void fremu_pi (freg st, freg src1, freg src2, const char* comm = 0);

extern void fmin_pi  (freg dst, freg src1, freg src2, const char* comm = 0);
extern void fmax_pi  (freg dst, freg src1, freg src2, const char* comm = 0);
extern void fminu_pi (freg dst, freg src1, freg src2, const char* comm = 0);
extern void fmaxu_pi (freg dst, freg src1, freg src2, const char* comm = 0);

// ----- Esperanto scalar extension for graphics -------------------------------

extern void packb(xreg dst, xreg src1, xreg src2, const char* comm = 0);
extern void bitmixb(xreg dst, xreg src1, xreg src2, const char* comm = 0);

// ----- Esperanto atomic extension --------------------------------------------
extern void amoswapl_w (xreg dst, xreg src1, xreg src2, const char* comm = 0);
extern void amoaddl_w  (xreg dst, xreg src1, xreg src2, const char* comm = 0);
extern void amoxorl_w  (xreg dst, xreg src1, xreg src2, const char* comm = 0);
extern void amoorl_w   (xreg dst, xreg src1, xreg src2, const char* comm = 0);
extern void amoandl_w  (xreg dst, xreg src1, xreg src2, const char* comm = 0);
extern void amominl_w  (xreg dst, xreg src1, xreg src2, const char* comm = 0);
extern void amomaxl_w  (xreg dst, xreg src1, xreg src2, const char* comm = 0);
extern void amominul_w (xreg dst, xreg src1, xreg src2, const char* comm = 0);
extern void amomaxul_w (xreg dst, xreg src1, xreg src2, const char* comm = 0);

extern void amoswapl_d (xreg dst, xreg src1, xreg src2, const char* comm = 0);
extern void amoaddl_d  (xreg dst, xreg src1, xreg src2, const char* comm = 0);
extern void amoxorl_d  (xreg dst, xreg src1, xreg src2, const char* comm = 0);
extern void amoorl_d   (xreg dst, xreg src1, xreg src2, const char* comm = 0);
extern void amoandl_d  (xreg dst, xreg src1, xreg src2, const char* comm = 0);
extern void amominl_d  (xreg dst, xreg src1, xreg src2, const char* comm = 0);
extern void amomaxl_d  (xreg dst, xreg src1, xreg src2, const char* comm = 0);
extern void amominul_d (xreg dst, xreg src1, xreg src2, const char* comm = 0);
extern void amomaxul_d (xreg dst, xreg src1, xreg src2, const char* comm = 0);

extern void famoswapl_pi (freg dst, freg src1, xreg src2, const char* comm = 0);
extern void famoaddl_pi  (freg dst, freg src1, xreg src2, const char* comm = 0);
extern void famoxorl_pi  (freg dst, freg src1, xreg src2, const char* comm = 0);
extern void famoorl_pi   (freg dst, freg src1, xreg src2, const char* comm = 0);
extern void famoandl_pi  (freg dst, freg src1, xreg src2, const char* comm = 0);
extern void famominl_pi  (freg dst, freg src1, xreg src2, const char* comm = 0);
extern void famomaxl_pi  (freg dst, freg src1, xreg src2, const char* comm = 0);
extern void famominul_pi (freg dst, freg src1, xreg src2, const char* comm = 0);
extern void famomaxul_pi (freg dst, freg src1, xreg src2, const char* comm = 0);
extern void famominl_ps  (freg dst, freg src1, xreg src2, const char* comm = 0);
extern void famomaxl_ps  (freg dst, freg src1, xreg src2, const char* comm = 0);

extern void amoswapg_w (xreg dst, xreg src1, xreg src2, const char* comm = 0);
extern void amoaddg_w  (xreg dst, xreg src1, xreg src2, const char* comm = 0);
extern void amoxorg_w  (xreg dst, xreg src1, xreg src2, const char* comm = 0);
extern void amoorg_w   (xreg dst, xreg src1, xreg src2, const char* comm = 0);
extern void amoandg_w  (xreg dst, xreg src1, xreg src2, const char* comm = 0);
extern void amoming_w  (xreg dst, xreg src1, xreg src2, const char* comm = 0);
extern void amomaxg_w  (xreg dst, xreg src1, xreg src2, const char* comm = 0);
extern void amominug_w (xreg dst, xreg src1, xreg src2, const char* comm = 0);
extern void amomaxug_w (xreg dst, xreg src1, xreg src2, const char* comm = 0);

extern void amoswapg_d (xreg dst, xreg src1, xreg src2, const char* comm = 0);
extern void amoaddg_d  (xreg dst, xreg src1, xreg src2, const char* comm = 0);
extern void amoxorg_d  (xreg dst, xreg src1, xreg src2, const char* comm = 0);
extern void amoorg_d   (xreg dst, xreg src1, xreg src2, const char* comm = 0);
extern void amoandg_d  (xreg dst, xreg src1, xreg src2, const char* comm = 0);
extern void amoming_d  (xreg dst, xreg src1, xreg src2, const char* comm = 0);
extern void amomaxg_d  (xreg dst, xreg src1, xreg src2, const char* comm = 0);
extern void amominug_d (xreg dst, xreg src1, xreg src2, const char* comm = 0);
extern void amomaxug_d (xreg dst, xreg src1, xreg src2, const char* comm = 0);

extern void famoswapg_pi (freg dst, freg src1, xreg src2, const char* comm = 0);
extern void famoaddg_pi  (freg dst, freg src1, xreg src2, const char* comm = 0);
extern void famoxorg_pi  (freg dst, freg src1, xreg src2, const char* comm = 0);
extern void famoorg_pi   (freg dst, freg src1, xreg src2, const char* comm = 0);
extern void famoandg_pi  (freg dst, freg src1, xreg src2, const char* comm = 0);
extern void famoming_pi  (freg dst, freg src1, xreg src2, const char* comm = 0);
extern void famomaxg_pi  (freg dst, freg src1, xreg src2, const char* comm = 0);
extern void famominug_pi (freg dst, freg src1, xreg src2, const char* comm = 0);
extern void famomaxug_pi (freg dst, freg src1, xreg src2, const char* comm = 0);
extern void famoming_ps  (freg dst, freg src1, xreg src2, const char* comm = 0);
extern void famomaxg_ps  (freg dst, freg src1, xreg src2, const char* comm = 0);

// extern void flwl_ps      (freg dst, xreg src1, const char* comm = 0);
// extern void fswl_ps      (freg dst, xreg src1, const char* comm = 0);
// extern void fgwl_ps      (freg dst, xreg src1, const char* comm = 0);
// extern void fscwl_ps     (freg dst, xreg src1, const char* comm = 0);
// extern void flwg_ps      (freg dst, xreg src1, const char* comm = 0);
// extern void fswg_ps      (freg dst, xreg src1, const char* comm = 0);
// extern void fgwg_ps      (freg dst, xreg src1, const char* comm = 0);
// extern void fscwg_ps     (freg dst, xreg src1, const char* comm = 0);

// ----- Esperanto cache control extension -------------------------------------

// ----- Esperanto messaging extension -----------------------------------------

extern void set_msg_port_data_funcs(void* getdata, void *hasdata, void *reqdata);
extern bool get_msg_port_stall(uint32_t thread, uint32_t id);
extern void write_msg_port_data(uint32_t thread, uint32_t id, uint32_t *data, uint8_t oob);
extern void update_msg_port_data();

// ----- Esperanto tensor extension --------------------------------------------

// Scratchpad

extern uint64_t get_scratchpad_value(int entry, int block, int * last_entry, int * size);
extern void get_scratchpad_conv_list(std::list<bool> * list);

// TensorFMA

extern uint32_t get_tensorfma_value(int entry, int pass, int block, int * size, int * passes, bool * conv_skip);

// TensorReduce

extern void get_reduce_info(uint64_t value, uint64_t * other_min, uint64_t * action);
extern uint64_t get_reduce_value(int entry, int block, int * size, int * start_entry);

// ----- Esperanto fast local barrier extension --------------------------------

#endif // _EMU_H
