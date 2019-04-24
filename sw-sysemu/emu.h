#ifndef _EMU_H
#define _EMU_H

#include <queue>
#include <iomanip>

#include "emu_defines.h"
#include "testLog.h"

#include "emu_casts.h"
#include "emu_gio.h"
#include "emu_memop.h"

#include "fpu/fpu.h"
#include "fpu/fpu_casts.h"
#include "tbox_emu.h"
#include "rbox.h"

namespace TBOX { class TBOXEmu; }
namespace RBOX { class RBOXEmu; }

// Accelerators
#if (EMU_TBOXES_PER_SHIRE > 1)
extern TBOX::TBOXEmu tbox[EMU_NUM_COMPUTE_SHIRES][EMU_TBOXES_PER_SHIRE];
#define GET_TBOX(shire_id, rbox_id) tbox[shire_id][rbox_id]
#else
extern TBOX::TBOXEmu tbox[EMU_NUM_COMPUTE_SHIRES];
#define GET_TBOX(shire_id, tbox_id) tbox[shire_id]
#endif

#if (EMU_RBOXES_PER_SHIRE > 1)
extern RBOX::RBOXEmu rbox[EMU_NUM_COMPUTE_SHIRES][EMU_RBOXES_PER_SHIRE];
#define GET_RBOX(shire_id, rbox_id) rbox[shire_id][rbox_id]
#else
extern RBOX::RBOXEmu rbox[EMU_NUM_COMPUTE_SHIRES];
#define GET_RBOX(shire_id, rbox_id) rbox[shire_id]
#endif

// Processor configuration
extern uint8_t in_sysemu;
extern uint32_t current_thread;

// Configure the emulation environment
extern void init_emu();

// Helpers
extern bool emu_done();
extern std::string dump_xregs(uint32_t thread_id);
extern std::string dump_fregs(uint32_t thread_id);
extern void init_stack();
extern void initcsr(uint32_t thread);             // init all CSRs
extern uint64_t get_csr(uint32_t thread, uint16_t cnum);
extern uint64_t xget(uint64_t src1);
extern void init(xreg dst, uint64_t val);         // init general purpose register
extern void fpinit(freg dst, uint64_t val[VL/2]); // init vector register
extern void minit(mreg dst, uint64_t val);        // init mask register

// Processor state manipulation
extern void set_pc(uint64_t pc);
extern void set_thread(uint32_t thread);
extern uint32_t get_thread();
extern uint32_t get_mask(unsigned maskNr);

// Main memory accessors
extern void set_memory_funcs(uint8_t  (*func_memread8_ ) (uint64_t),
                             uint16_t (*func_memread16_) (uint64_t),
                             uint32_t (*func_memread32_) (uint64_t),
                             uint64_t (*func_memread64_) (uint64_t),
                             void (*func_memwrite8_ ) (uint64_t, uint8_t ),
                             void (*func_memwrite16_) (uint64_t, uint16_t),
                             void (*func_memwrite32_) (uint64_t, uint32_t),
                             void (*func_memwrite64_) (uint64_t, uint64_t));
extern void set_msg_funcs(void (*func_msg_to_thread) (int));

// Traps
extern void take_trap(const trap_t& t);

// Interrupts
extern void check_pending_interrupts();

// Illegal instruction encodings will execute this
extern void unknown(const char* comm = 0);

// Instruction encodings that match minstmatch/minstmask will execute this
extern void check_minst_match(uint32_t bits);

// ----- RV64I emulation -------------------------------------------------------

extern void beq  (xreg rs1, xreg rs2, int64_t b_imm, const char* comm = 0);
extern void bne  (xreg rs1, xreg rs2, int64_t b_imm, const char* comm = 0);
extern void blt  (xreg rs1, xreg rs2, int64_t b_imm, const char* comm = 0);
extern void bge  (xreg rs1, xreg rs2, int64_t b_imm, const char* comm = 0);
extern void bltu (xreg rs1, xreg rs2, int64_t b_imm, const char* comm = 0);
extern void bgeu (xreg rs1, xreg rs2, int64_t b_imm, const char* comm = 0);

extern void jalr (xreg rd, xreg rs1, int64_t i_imm, const char* comm = 0);
extern void jal  (xreg rd, int64_t j_imm, const char* comm = 0);

extern void lui   (xreg rd, int64_t u_imm, const char* comm = 0);
extern void auipc (xreg rd, int64_t u_imm, const char* comm = 0);

extern void addi  (xreg rd, xreg rs1, int64_t i_imm, const char* comm = 0);
extern void slli  (xreg rd, xreg rs1, unsigned shamt6, const char* comm = 0);
extern void slti  (xreg rd, xreg rs1, int64_t i_imm, const char* comm = 0);
extern void sltiu (xreg rd, xreg rs1, int64_t i_imm, const char* comm = 0);
extern void xori  (xreg rd, xreg rs1, int64_t i_imm, const char* comm = 0);
extern void srli  (xreg rd, xreg rs1, unsigned shamt6, const char* comm = 0);
extern void srai  (xreg rd, xreg rs1, unsigned shamt6, const char* comm = 0);
extern void ori   (xreg rd, xreg rs1, int64_t i_imm, const char* comm = 0);
extern void andi  (xreg rd, xreg rs1, int64_t i_imm, const char* comm = 0);

extern void add  (xreg rd, xreg rs1, xreg rs2, const char* comm = 0);
extern void sub  (xreg rd, xreg rs1, xreg rs2, const char* comm = 0);
extern void sll  (xreg rd, xreg rs1, xreg rs2, const char* comm = 0);
extern void slt  (xreg rd, xreg rs1, xreg rs2, const char* comm = 0);
extern void sltu (xreg rd, xreg rs1, xreg rs2, const char* comm = 0);
extern void xor_ (xreg rd, xreg rs1, xreg rs2, const char* comm = 0);
extern void srl  (xreg rd, xreg rs1, xreg rs2, const char* comm = 0);
extern void sra  (xreg rd, xreg rs1, xreg rs2, const char* comm = 0);
extern void or_  (xreg rd, xreg rs1, xreg rs2, const char* comm = 0);
extern void and_ (xreg rd, xreg rs1, xreg rs2, const char* comm = 0);

extern void addiw (xreg rd, xreg rs1, int64_t i_imm, const char* comm = 0);
extern void slliw (xreg rd, xreg rs1, unsigned shamt5, const char* comm = 0);
extern void srliw (xreg rd, xreg rs1, unsigned shamt5, const char* comm = 0);
extern void sraiw (xreg rd, xreg rs1, unsigned shamt5, const char* comm = 0);

extern void addw (xreg rd, xreg rs1, xreg rs2, const char* comm = 0);
extern void subw (xreg rd, xreg rs1, xreg rs2, const char* comm = 0);
extern void sllw (xreg rd, xreg rs1, xreg rs2, const char* comm = 0);
extern void srlw (xreg rd, xreg rs1, xreg rs2, const char* comm = 0);
extern void sraw (xreg rd, xreg rs1, xreg rs2, const char* comm = 0);

extern void lb  (xreg dst, xreg base, int64_t off, const char* comm = 0);
extern void lh  (xreg dst, xreg base, int64_t off, const char* comm = 0);
extern void lw  (xreg dst, xreg base, int64_t off, const char* comm = 0);
extern void ld  (xreg dst, xreg base, int64_t off, const char* comm = 0);
extern void lbu (xreg dst, xreg base, int64_t off, const char* comm = 0);
extern void lhu (xreg dst, xreg base, int64_t off, const char* comm = 0);
extern void lwu (xreg dst, xreg base, int64_t off, const char* comm = 0);

extern void sd (xreg src1, xreg base, int64_t off, const char* comm = 0);
extern void sw (xreg src1, xreg base, int64_t off, const char* comm = 0);
extern void sh (xreg src1, xreg base, int64_t off, const char* comm = 0);
extern void sb (xreg src1, xreg base, int64_t off, const char* comm = 0);

extern void fence   (const char* comm = 0);
extern void fence_i (const char* comm = 0);

// ----- RV64M emulation -------------------------------------------------------

extern void mul    (xreg rd, xreg rs1, xreg rs2, const char* comm = 0);
extern void mulh   (xreg rd, xreg rs1, xreg rs2, const char* comm = 0);
extern void mulhsu (xreg rd, xreg rs1, xreg rs2, const char* comm = 0);
extern void mulhu  (xreg rd, xreg rs1, xreg rs2, const char* comm = 0);
extern void div_   (xreg rd, xreg rs1, xreg rs2, const char* comm = 0);
extern void divu   (xreg rd, xreg rs1, xreg rs2, const char* comm = 0);
extern void rem    (xreg rd, xreg rs1, xreg rs2, const char* comm = 0);
extern void remu   (xreg rd, xreg rs1, xreg rs2, const char* comm = 0);

extern void mulw   (xreg rd, xreg rs1, xreg rs2, const char* comm = 0);
extern void divw   (xreg rd, xreg rs1, xreg rs2, const char* comm = 0);
extern void divuw  (xreg rd, xreg rs1, xreg rs2, const char* comm = 0);
extern void remw   (xreg rd, xreg rs1, xreg rs2, const char* comm = 0);
extern void remuw  (xreg rd, xreg rs1, xreg rs2, const char* comm = 0);

// ----- RV64A emulation -------------------------------------------------------
#if 0
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
#endif

// ----- SYSTEM emulation ------------------------------------------------------

extern void ecall(const char* comm = 0);
extern void ebreak(const char* comm = 0);
extern void sret(const char* comm = 0);
extern void mret(const char* comm = 0);
extern void wfi(const char* comm = 0);
extern void sfence_vma(xreg src1, xreg src2, const char* comm = 0);
extern void csrrw(xreg dst, uint16_t src1, xreg src2, const char* comm = 0);
extern void csrrs(xreg dst, uint16_t src1, xreg src2, const char* comm = 0);
extern void csrrc(xreg dst, uint16_t src1, xreg src2, const char* comm = 0);
extern void csrrwi(xreg dst, uint16_t src1, uint64_t imm, const char* comm = 0);
extern void csrrsi(xreg dst, uint16_t src1, uint64_t imm, const char* comm = 0);
extern void csrrci(xreg dst, uint16_t src1, uint64_t imm, const char* comm = 0);

// ----- RV64C emulation -------------------------------------------------------

extern void c_jalr (xreg rd, xreg rs1, int64_t i_imm, const char* comm = 0);
extern void c_jal  (xreg rd, int64_t j_imm, const char* comm = 0);

// ----- RV64F emulation -------------------------------------------------------

extern void fadd_s   (freg fd, freg fs1, freg fs2, rounding_mode rm, const char* comm = 0);
extern void fsub_s   (freg fd, freg fs1, freg fs2, rounding_mode rm, const char* comm = 0);
extern void fmul_s   (freg fd, freg fs1, freg fs2, rounding_mode rm, const char* comm = 0);
extern void fdiv_s   (freg fd, freg fs1, freg fs2, rounding_mode rm, const char* comm = 0);
extern void fsgnj_s  (freg fd, freg fs1, freg fs2, const char* comm = 0);
extern void fsgnjn_s (freg fd, freg fs1, freg fs2, const char* comm = 0);
extern void fsgnjx_s (freg fd, freg fs1, freg fs2, const char* comm = 0);
extern void fmin_s   (freg fd, freg fs1, freg fs2, const char* comm = 0);
extern void fmax_s   (freg fd, freg fs1, freg fs2, const char* comm = 0);
extern void fsqrt_s  (freg fd, freg fs1, rounding_mode rm, const char* comm = 0);

extern void feq_s (xreg rd, freg fs1, freg fs2, const char* comm = 0);
extern void fle_s (xreg rd, freg fs1, freg fs2, const char* comm = 0);
extern void flt_s (xreg rd, freg fs1, freg fs2, const char* comm = 0);

extern void fcvt_w_s  (xreg rd, freg fs1, rounding_mode rm, const char* comm = 0);
extern void fcvt_wu_s (xreg rd, freg fs1, rounding_mode rm, const char* comm = 0);
extern void fcvt_l_s  (xreg rd, freg fs1, rounding_mode rm, const char* comm = 0);
extern void fcvt_lu_s (xreg rd, freg fs1, rounding_mode rm, const char* comm = 0);
extern void fmv_x_w   (xreg rd, freg fs1, const char* comm = 0);
extern void fclass_s  (xreg rd, freg fs1, const char* comm = 0);

extern void fcvt_s_w  (freg fd, xreg rs1, rounding_mode rm, const char* comm = 0);
extern void fcvt_s_wu (freg fd, xreg rs1, rounding_mode rm, const char* comm = 0);
extern void fcvt_s_l  (freg fd, xreg rs1, rounding_mode rm, const char* comm = 0);
extern void fcvt_s_lu (freg fd, xreg rs1, rounding_mode rm, const char* comm = 0);
extern void fmv_w_x   (freg fd, xreg rs1, const char* comm = 0);

extern void flw (freg dst, xreg base, int64_t off, const char* comm = 0);

extern void fsw (freg src1, xreg base, int64_t off, const char* comm = 0);

extern void fmadd_s  (freg fd, freg fs1, freg fs2, freg fs3, rounding_mode rm, const char* comm = 0);
extern void fmsub_s  (freg fd, freg fs1, freg fs2, freg fs3, rounding_mode rm, const char* comm = 0);
extern void fnmsub_s (freg fd, freg fs1, freg fs2, freg fs3, rounding_mode rm, const char* comm = 0);
extern void fnmadd_s (freg fd, freg fs1, freg fs2, freg fs3, rounding_mode rm, const char* comm = 0);

// ----- Esperanto mask extension ----------------------------------------------

extern void maskand (mreg md, mreg ms1, mreg ms2, const char* comm = 0);
extern void maskor  (mreg md, mreg ms1, mreg ms2, const char* comm = 0);
extern void maskxor (mreg md, mreg ms1, mreg ms2, const char* comm = 0);

extern void masknot (mreg md, mreg ms1, const char* comm = 0);

extern void mova_x_m (xreg rd, const char* comm = 0);
extern void mova_m_x (xreg rs1, const char* comm = 0);

extern void mov_m_x  (mreg md, xreg rs1, unsigned uimm8, const char* comm = 0);

extern void maskpopc  (xreg rd, mreg ms1, const char* comm = 0);
extern void maskpopcz (xreg rd, mreg ms1, const char* comm = 0);

extern void maskpopc_rast (xreg rd, mreg ms1, mreg ms2, unsigned umsk4, const char* comm = 0);

// ----- Esperanto packed-single extension -------------------------------------

// Load and store

extern void flq2    (freg dst, xreg base, int64_t off, const char* comm = 0);
extern void flw_ps  (freg dst, xreg base, int64_t off, const char* comm = 0);
extern void flwl_ps (freg dst, xreg base, const char* comm = 0);
extern void flwg_ps (freg dst, xreg base, const char* comm = 0);

extern void fsq2    (freg src, xreg base, int64_t off, const char* comm = 0);
extern void fsw_ps  (freg src, xreg base, int64_t off, const char* comm = 0);
extern void fswl_ps (freg dst, xreg base, const char* comm = 0);
extern void fswg_ps (freg src, xreg base, const char* comm = 0);

// Broadcast

extern void fbc_ps  (freg dst, xreg base, int64_t off, const char* comm = 0);
extern void fbci_ps (freg fd, uint32_t f32imm, const char* comm = 0);
extern void fbcx_ps (freg fd, xreg rs1, const char* comm = 0);

// Gather and scatter

extern void fgb_ps    (freg dst, freg src1, xreg base, const char* comm = 0);
extern void fgh_ps    (freg dst, freg src1, xreg base, const char* comm = 0);
extern void fgw_ps    (freg dst, freg src1, xreg base, const char* comm = 0);
extern void fgwl_ps   (freg dst, freg src1, xreg base, const char* comm = 0);
extern void fghl_ps   (freg dst, freg src1, xreg base, const char* comm = 0);
extern void fgbl_ps   (freg dst, freg src1, xreg base, const char* comm = 0);
extern void fgwg_ps   (freg dst, freg src1, xreg base, const char* comm = 0);
extern void fghg_ps   (freg dst, freg src1, xreg base, const char* comm = 0);
extern void fgbg_ps   (freg dst, freg src1, xreg base, const char* comm = 0);

extern void fg32b_ps  (freg dst, xreg src1, xreg src2, const char* comm = 0);
extern void fg32h_ps  (freg dst, xreg src1, xreg src2, const char* comm = 0);
extern void fg32w_ps  (freg dst, xreg src1, xreg src2, const char* comm = 0);

extern void fscb_ps   (freg src, freg src1, xreg base, const char* comm = 0);
extern void fsch_ps   (freg src, freg src1, xreg base, const char* comm = 0);
extern void fscw_ps   (freg src, freg src1, xreg base, const char* comm = 0);
extern void fscwl_ps  (freg src, freg src1, xreg base, const char* comm = 0);
extern void fschl_ps  (freg src, freg src1, xreg base, const char* comm = 0);
extern void fscbl_ps  (freg src, freg src1, xreg base, const char* comm = 0);
extern void fscwg_ps  (freg src, freg src1, xreg base, const char* comm = 0);
extern void fschg_ps  (freg src, freg src1, xreg base, const char* comm = 0);
extern void fscbg_ps  (freg src, freg src1, xreg base, const char* comm = 0);

extern void fsc32b_ps (freg src, xreg src1, xreg src2, const char* comm = 0);
extern void fsc32h_ps (freg src, xreg src1, xreg src2, const char* comm = 0);
extern void fsc32w_ps (freg src, xreg src1, xreg src2, const char* comm = 0);

// Computational (follows RV64F)

extern void fadd_ps   (freg fd, freg fs1, freg fs2, rounding_mode rm, const char* comm = 0);
extern void fsub_ps   (freg fd, freg fs1, freg fs2, rounding_mode rm, const char* comm = 0);
extern void fmul_ps   (freg fd, freg fs1, freg fs2, rounding_mode rm, const char* comm = 0);
extern void fdiv_ps   (freg fd, freg fs1, freg fs2, rounding_mode rm, const char* comm = 0);
extern void fsgnj_ps  (freg fd, freg fs1, freg fs2, const char* comm = 0);
extern void fsgnjn_ps (freg fd, freg fs1, freg fs2, const char* comm = 0);
extern void fsgnjx_ps (freg fd, freg fs1, freg fs2, const char* comm = 0);
extern void fmin_ps   (freg fd, freg fs1, freg fs2, const char* comm = 0);
extern void fmax_ps   (freg fd, freg fs1, freg fs2, const char* comm = 0);
extern void fsqrt_ps  (freg fd, freg fs1, rounding_mode rm, const char* comm = 0);

extern void feq_ps (freg fd, freg fs1, freg fs2, const char* comm = 0);
extern void fle_ps (freg fd, freg fs1, freg fs2, const char* comm = 0);
extern void flt_ps (freg fd, freg fs1, freg fs2, const char* comm = 0);

extern void feqm_ps (mreg md, freg fs1, freg fs2, const char* comm = 0);
extern void flem_ps (mreg md, freg fs1, freg fs2, const char* comm = 0);
extern void fltm_ps (mreg md, freg fs1, freg fs2, const char* comm = 0);

extern void fsetm_pi (mreg md, freg fs1, const char* comm = 0);

extern void fcmov_ps  (freg fd, freg fs1, freg fs2, freg fs3, const char* comm = 0);
extern void fcmovm_ps (freg fd, freg fs1, freg fs2, const char* comm = 0);
extern void fmvz_x_ps (xreg rd, freg fs1, unsigned uimm3, const char* comm = 0);
extern void fmvs_x_ps (xreg rd, freg fs1, unsigned uimm3, const char* comm = 0);
extern void fswizz_ps (freg fd, freg fs1, unsigned uimm8, const char* comm = 0);

extern void fcvt_pw_ps  (freg fd, freg fs1, rounding_mode rm, const char* comm = 0);
extern void fcvt_pwu_ps (freg fd, freg fs1, rounding_mode rm, const char* comm = 0);
extern void fclass_ps   (freg fd, freg fs1, const char* comm = 0);
extern void fcvt_ps_pw  (freg fd, freg fs1, rounding_mode rm, const char* comm = 0);
extern void fcvt_ps_pwu (freg fd, freg fs1, rounding_mode rm, const char* comm = 0);

extern void fmadd_ps  (freg fd, freg fs1, freg fs2, freg fs3, rounding_mode rm, const char* comm = 0);
extern void fmsub_ps  (freg fd, freg fs1, freg fs2, freg fs3, rounding_mode rm, const char* comm = 0);
extern void fnmsub_ps (freg fd, freg fs1, freg fs2, freg fs3, rounding_mode rm, const char* comm = 0);
extern void fnmadd_ps (freg fd, freg fs1, freg fs2, freg fs3, rounding_mode rm, const char* comm = 0);

// Graphics upconvert

extern void fcvt_ps_f16  (freg fd, freg fs1, const char* comm = 0);
extern void fcvt_ps_f11  (freg fd, freg fs1, const char* comm = 0);
extern void fcvt_ps_f10  (freg fd, freg fs1, const char* comm = 0);

extern void fcvt_ps_un24 (freg fd, freg fs1, const char* comm = 0);
extern void fcvt_ps_un16 (freg fd, freg fs1, const char* comm = 0);
extern void fcvt_ps_un10 (freg fd, freg fs1, const char* comm = 0);
extern void fcvt_ps_un8  (freg fd, freg fs1, const char* comm = 0);
extern void fcvt_ps_un2  (freg fd, freg fs1, const char* comm = 0);

extern void fcvt_ps_sn16 (freg fd, freg fs1, const char* comm = 0);
extern void fcvt_ps_sn8  (freg fd, freg fs1, const char* comm = 0);

// Graphics downconvert

extern void fcvt_f16_ps  (freg fd, freg fs1, const char* comm = 0);
extern void fcvt_f11_ps  (freg fd, freg fs1, const char* comm = 0);
extern void fcvt_f10_ps  (freg fd, freg fs1, const char* comm = 0);

extern void fcvt_un24_ps (freg fd, freg fs1, const char* comm = 0);
extern void fcvt_un16_ps (freg fd, freg fs1, const char* comm = 0);
extern void fcvt_un10_ps (freg fd, freg fs1, const char* comm = 0);
extern void fcvt_un8_ps  (freg fd, freg fs1, const char* comm = 0);
extern void fcvt_un2_ps  (freg fd, freg fs1, const char* comm = 0);

extern void fcvt_sn16_ps (freg fd, freg fs1, const char* comm = 0);
extern void fcvt_sn8_ps  (freg fd, freg fs1, const char* comm = 0);

// Graphics additional

extern void fsin_ps   (freg fd, freg fs1, const char* comm = 0);
extern void fexp_ps   (freg fd, freg fs1, const char* comm = 0);
extern void flog_ps   (freg fd, freg fs1, const char* comm = 0);
extern void ffrc_ps   (freg fd, freg fs1, const char* comm = 0);
extern void fround_ps (freg fd, freg fs1, rounding_mode rm, const char* comm = 0);
extern void frcp_ps   (freg fd, freg fs1, const char* comm = 0);
extern void frsq_ps   (freg fd, freg fs1, const char* comm = 0);

extern void cubeface_ps    (freg fd, freg fs1, freg fs2, const char* comm = 0);
extern void cubefaceidx_ps (freg fd, freg fs1, freg fs2, const char* comm = 0);
extern void cubesgnsc_ps   (freg fd, freg fs1, freg fs2, const char* comm = 0);
extern void cubesgntc_ps   (freg fd, freg fs1, freg fs2, const char* comm = 0);

extern void fcvt_ps_rast  (freg fd, freg fs1, rounding_mode rm, const char* comm = 0);
extern void fcvt_rast_ps  (freg fd, freg fs1, const char* comm = 0);
extern void frcp_fix_rast (freg fd, freg fs1, freg fs2, const char* comm = 0);

// ----- Esperanto packed-integer extension ------------------------------------

// Broadcast

extern void fbci_pi (freg fd, int32_t i32imm, const char* comm = 0);

// Computational (follows RV64I/RV64F + RV64M + min/max)

extern void feq_pi  (freg fd, freg fs1, freg fs2, const char* comm = 0);     // RV64I: beq
//extern void fne_pi  (freg fd, freg fs1, freg fs2, const char* comm = 0);   // RV64I: bne
extern void fle_pi  (freg fd, freg fs1, freg fs2, const char* comm = 0);     // RV64I: bge
extern void flt_pi  (freg fd, freg fs1, freg fs2, const char* comm = 0);     // RV64I: blt
//extern void fleu_pi (freg fd, freg fs1, freg fs2, const char* comm = 0);   // RV64I: bgeu
extern void fltu_pi (freg fd, freg fs1, freg fs2, const char* comm = 0);     // RV64I: bltu

//extern void feqm_pi  (mreg md, freg fs1, freg fs2, const char* comm = 0);
//extern void fnem_pi  (mreg md, freg fs1, freg fs2, const char* comm = 0);
//extern void flem_pi  (mreg md, freg fs1, freg fs2, const char* comm = 0);
extern void fltm_pi  (mreg md, freg fs1, freg fs2, const char* comm = 0);
//extern void fleum_pi (mreg md, freg fs1, freg fs2, const char* comm = 0);
//extern void fltum_pi (mreg md, freg fs1, freg fs2, const char* comm = 0);

extern void faddi_pi  (freg fd, freg fs1, int32_t v_imm, const char* comm = 0);
extern void fslli_pi  (freg fd, freg fs1, int32_t v_imm, const char* comm = 0);
//extern void fslti_pi  (freg fd, freg fs1, int32_t v_imm, const char* comm = 0);
//extern void fsltiu_pi (freg fd, freg fs1, int32_t v_imm, const char* comm = 0);
//extern void fxori_pi  (freg fd, freg fs1, int32_t v_imm, const char* comm = 0);
extern void fsrli_pi  (freg fd, freg fs1, int32_t v_imm, const char* comm = 0);
extern void fsrai_pi  (freg fd, freg fs1, int32_t v_imm, const char* comm = 0);
//extern void fori_pi   (freg fd, freg fs1, int32_t v_imm, const char* comm = 0);
extern void fandi_pi  (freg fd, freg fs1, int32_t v_imm, const char* comm = 0);

extern void fadd_pi  (freg fd, freg fs1, freg fs2, const char* comm = 0);
extern void fsub_pi  (freg fd, freg fs1, freg fs2, const char* comm = 0);
//extern void fslt_pi  (freg fd, freg fs1, freg fs2, const char* comm = 0);
//extern void fsltu_pi (freg fd, freg fs1, freg fs2, const char* comm = 0);
extern void fsll_pi  (freg fd, freg fs1, freg fs2, const char* comm = 0);
extern void fxor_pi  (freg fd, freg fs1, freg fs2, const char* comm = 0);
extern void fsrl_pi  (freg fd, freg fs1, freg fs2, const char* comm = 0);
extern void fsra_pi  (freg fd, freg fs1, freg fs2, const char* comm = 0);
extern void for_pi   (freg fd, freg fs1, freg fs2, const char* comm = 0);
extern void fand_pi  (freg fd, freg fs1, freg fs2, const char* comm = 0);

extern void fnot_pi  (freg fd, freg fs1, const char* comm = 0);
extern void fsat8_pi (freg fd, freg fs1, const char* comm = 0);
extern void fsatu8_pi (freg fd, freg fs1, const char* comm = 0);

extern void fpackreph_pi (freg fd, freg fs1, const char* comm = 0);
extern void fpackrepb_pi (freg fd, freg fs1, const char* comm = 0);

extern void fmul_pi   (freg fd, freg fs1, freg fs2, const char* comm = 0);
extern void fmulh_pi  (freg fd, freg fs1, freg fs2, const char* comm = 0);
// fmulhsu_pi will not be implemented
extern void fmulhu_pi (freg fd, freg fs1, freg fs2, const char* comm = 0);
extern void fdiv_pi   (freg fd, freg fs1, freg fs2, const char* comm = 0);
extern void fdivu_pi  (freg fd, freg fs1, freg fs2, const char* comm = 0);
extern void frem_pi   (freg fd, freg fs1, freg fs2, const char* comm = 0);
extern void fremu_pi  (freg fd, freg fs1, freg fs2, const char* comm = 0);

extern void fmin_pi  (freg fd, freg fs1, freg fs2, const char* comm = 0);
extern void fmax_pi  (freg fd, freg fs1, freg fs2, const char* comm = 0);
extern void fminu_pi (freg fd, freg fs1, freg fs2, const char* comm = 0);
extern void fmaxu_pi (freg fd, freg fs1, freg fs2, const char* comm = 0);

// ----- Esperanto scalar extension for graphics -------------------------------

extern void packb   (xreg rd, xreg rs1, xreg rs2, const char* comm = 0);
extern void bitmixb (xreg rd, xreg rs1, xreg rs2, const char* comm = 0);

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

extern void sbl (xreg dst, xreg base, const char* comm = 0);
extern void sbg (xreg dst, xreg base, const char* comm = 0);
extern void shl (xreg dst, xreg base, const char* comm = 0);
extern void shg (xreg dst, xreg base, const char* comm = 0);

// ----- Esperanto cache control extension -------------------------------------

// ----- Esperanto messaging extension -----------------------------------------

extern void set_delayed_msg_port_write(bool f);
extern bool get_msg_port_stall(uint32_t target_thread, uint32_t port_id);
extern void read_msg_port_data(uint32_t target_thread, uint32_t port_id, uint32_t *data, uint8_t* oob);
extern void write_msg_port_data(uint32_t target_thread, uint32_t port_id, uint32_t *data, uint8_t oob);
extern void write_msg_port_data_from_tbox(uint32_t target_thread, uint32_t port_id, uint32_t tbox_id, uint32_t *data, uint8_t oob);
extern void write_msg_port_data_from_rbox(uint32_t target_thread, uint32_t port_id, uint32_t rbox_id, uint32_t *data, uint8_t oob);
extern void commit_msg_port_data(uint32_t target_thread, uint32_t port_id, uint32_t source_thread);
extern void commit_msg_port_data_from_tbox(uint32_t target_thread, uint32_t port_id, uint32_t tbox_id);
extern void commit_msg_port_data_from_rbox(uint32_t target_thread, uint32_t port_id, uint32_t rbox_id);

// ----- Esperanto tensor extension --------------------------------------------

// TensorReduce

extern void tensor_reduce_decode(uint64_t value, unsigned* other_min, unsigned* action);

// Shire cooperative mode

extern void write_shire_coop_mode(unsigned shire, uint64_t val);
extern uint64_t read_shire_coop_mode(unsigned shire);

// ----- Esperanto fast local barrier extension --------------------------------

// ----- Esperanto fast credit counter extension --------------------------------

extern uint64_t get_fcc_cnt();
extern void fcc_inc(uint64_t thread, uint64_t shire, uint64_t minion_mask, uint64_t fcc_id);
std::queue<uint32_t> &get_minions_to_awake();

// ----- Esperanto IPI extension ------------------------------------------------

extern void raise_interrupt(int thread, int cause);
extern void raise_software_interrupt(int thread);
extern void clear_software_interrupt(int thread);

extern void raise_timer_interrupt(int thread);
extern void clear_timer_interrupt(int thread);

extern void raise_external_interrupt(int thread);
extern void clear_external_interrupt(int thread);

// ----- Esperanto code prefetching extension -----------------------------------

extern void write_icache_prefetch(int privilege, unsigned shire, uint64_t val);

extern uint64_t read_icache_prefetch(int privilege, unsigned shire);

extern void finish_icache_prefetch(unsigned shire);

#endif // _EMU_H
