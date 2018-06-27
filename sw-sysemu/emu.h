#ifndef _EMU_H
#define _EMU_H

#include <list>

#include "emu_defines.h"

#define DEBUG_EMU   1
#define DEBUG_MASK  1
#define DISASM      1

#define MASK2BYTE(_MR) (_MR.b[7]<<7|_MR.b[6]<<6|_MR.b[5]<<5|_MR.b[4]<<4|_MR.b[3]<<3|_MR.b[2]<<2|_MR.b[1]<<1|_MR.b[0])

#ifdef DEBUG_EMU
#undef DEBUG_EMU
#define DEBUG_EMU(a) if ( print_debug ) { a }
extern int print_debug;
#else
#define DEBUG_EMU(a)
#endif

#ifdef DEBUG_MASK
#undef DEBUG_MASK
#define DEBUG_MASK(_MR) DEBUG_EMU(gprintf("\tmask = 0x%02x\n",MASK2BYTE(_MR));)
#else
#define DEBUG_MASK(a)
#endif

#ifdef IPC
#undef IPC
#define IPC(a) { a }
#define DISASM 1
#else
#define IPC(a)
#endif

#ifdef DISASM
#undef DISASM
#define DISASM(a) { a }
#else
#define DISASM(a)
#endif

// Used to access different threads transparently
#define XREGS xregs[current_thread]
#define FREGS fregs[current_thread]
#define MREGS mregs[current_thread]
#define SCP   scp[current_thread]

extern xdata xregs[EMU_NUM_THREADS][32];
extern fdata fregs[EMU_NUM_THREADS][32];
extern mdata mregs[EMU_NUM_THREADS][8];
extern fdata scp[EMU_NUM_THREADS][64][4];

extern uint32 current_thread;

void print_regs();
void print_comment(const char *comm);

extern "C" void init_emu(int debug, int fakesam);
extern "C" void minit(mreg dst, uint64 val);
extern "C" void init(xreg dst, uint64 val);
extern "C" uint64 xget(uint64 src1);
extern "C" uint64 csrget(csr src1);
extern "C" void fpinit(freg dst, uint64 val[2]);
extern "C" void initcsr(uint32 thread);
extern "C" void init_stack();
extern "C" void set_pc(uint64 pc);
extern "C" void set_thread(uint32 thread);
extern "C" uint32 get_thread();
extern "C" uint32 get_mask(unsigned maskNr);
#ifdef CHECKER
extern "C" void set_msg_port_data_func(void* f, void *g, void *h);
extern "C" bool get_msg_port_stall(uint32 thread, uint32 id);
uint64 get_msg_port_offset(uint32 port_id);
void write_msg_port_data(uint32 thread, uint32 port_id);
extern "C" void write_msg_port_data_(uint32 thread, uint32 port_id, uint32 *data);
extern "C" void update_msg_port_data();
#endif
extern "C" void get_reduce_info(uint64 value, uint64 * other_min, uint64 * action);
extern "C" uint64 get_reduce_value(int entry, int block, int * size, int * start_entry);
extern "C" uint64 get_scratchpad_value(int entry, int block, int * last_entry, int * size);
extern "C" std::list<bool> * get_scratchpad_conv_list();
extern "C" uint64 get_tensorfma_value(int entry, int pass, int block, int * size, int * passes, bool * conv_skip);

uint8 memread8(uint64 addr);
uint16 memread16(uint64 addr);
uint32 memread32(uint64 addr);
uint64 memread64(uint64 addr);
void memwrite8(uint64 addr, uint8 data);
void memwrite16(uint64 addr, uint16 data);
void memwrite32(uint64 addr, uint32 data);
void memwrite64(uint64 addr, uint64 data);

extern "C" void lb(xreg dst, int off, xreg base);
extern "C" void lh(xreg dst, int off, xreg base);
extern "C" void lw(xreg dst, int off, xreg base);
extern "C" void ld(xreg dst, int off, xreg base);
extern "C" void lbu(xreg dst, int off, xreg base);
extern "C" void lhu(xreg dst, int off, xreg base);
extern "C" void lwu(xreg dst, int off, xreg base);
extern "C" void addi(xreg dst, xreg src1, int imm);
extern "C" void addiw(xreg dst, xreg src1, int imm);
extern "C" void slt(xreg dst, xreg src1, xreg src2);
extern "C" void sltu(xreg dst, xreg src1, xreg src2);
extern "C" void slti(xreg dst, xreg src1, int imm);
extern "C" void sltiu(xreg dst, xreg src1, int imm);
extern "C" void mul(xreg dst, xreg src1, xreg src2);
extern "C" void mulw(xreg dst, xreg src1, xreg src2);
extern "C" void mulh(xreg dst, xreg src1, xreg src2);
extern "C" void mulhu(xreg dst, xreg src1, xreg src2);
extern "C" void div_(xreg dst, xreg src1, xreg src2);
extern "C" void divu(xreg dst, xreg src1, xreg src2);
extern "C" void divw(xreg dst, xreg src1, xreg src2);
extern "C" void divuw(xreg dst, xreg src1, xreg src2);
extern "C" void rem(xreg dst, xreg src1, xreg src2);
extern "C" void remu(xreg dst, xreg src1, xreg src2);
extern "C" void remw(xreg dst, xreg src1, xreg src2);
extern "C" void remuw(xreg dst, xreg src1, xreg src2);
extern "C" void add(xreg dst, xreg src1, xreg src2);
extern "C" void addw(xreg dst, xreg src1, xreg src2);
extern "C" void sub(xreg dst, xreg src1, xreg src2);
extern "C" void subw(xreg dst, xreg src1, xreg src2);
extern "C" void ori(xreg dst, xreg src1, int imm);
extern "C" void or_(xreg dst, xreg src1, xreg src2);
extern "C" void andi(xreg dst, xreg src1, int imm);
extern "C" void and_(xreg dst, xreg src1, xreg src2);
extern "C" void xori(xreg dst, xreg src1, int imm);
extern "C" void xor_(xreg dst, xreg src1, xreg src2);
extern "C" void lui(xreg dst, int imm);
extern "C" void auipc(xreg dst, int imm);
extern "C" void sllw(xreg dst, xreg src1, xreg src2);
extern "C" void sll(xreg dst, xreg src1, xreg src2);
extern "C" void slliw(xreg dst, xreg src1, int imm);
extern "C" void slli(xreg dst, xreg src1, int imm);
extern "C" void srlw(xreg dst, xreg src1, xreg src2);
extern "C" void srl(xreg dst, xreg src1, xreg src2);
extern "C" void srliw(xreg dst, xreg src1, int imm);
extern "C" void srli(xreg dst, xreg src1, int imm);
extern "C" void sra(xreg dst, xreg src1, xreg src2);
extern "C" void sraw(xreg dst, xreg src1, xreg src2);
extern "C" void srai(xreg dst, xreg src1, int imm);
extern "C" void sraiw(xreg dst, xreg src1, int imm);
extern "C" void jal(xreg dst, int imm);
extern "C" void jalr(xreg dst, xreg src1, int imm);
extern "C" void c_jalr(xreg dst, xreg src1, int imm);
extern "C" void beq(xreg src1, xreg src2, int imm);
extern "C" void bne(xreg src1, xreg src2, int imm);
extern "C" void blt(xreg src1, xreg src2, int imm);
extern "C" void bltu(xreg src1, xreg src2, int imm);
extern "C" void bge(xreg src1, xreg src2, int imm);
extern "C" void bgeu(xreg src1, xreg src2, int imm);
extern "C" void sd(xreg src1, int off, xreg base);
extern "C" void sw(xreg src1, int off, xreg base);
extern "C" void sh(xreg src1, int off, xreg base);
extern "C" void sb(xreg src1, int off, xreg base);
extern "C" void amoswap_w(xreg dst, xreg src1, xreg src2);
extern "C" void amoadd_w(xreg dst, xreg src1, xreg src2);
extern "C" void amoxor_w(xreg dst, xreg src1, xreg src2);
extern "C" void amoand_w(xreg dst, xreg src1, xreg src2);
extern "C" void amoor_w(xreg dst, xreg src1, xreg src2);
extern "C" void amomin_w(xreg dst, xreg src1, xreg src2);
extern "C" void amomax_w(xreg dst, xreg src1, xreg src2);
extern "C" void amominu_w(xreg dst, xreg src1, xreg src2);
extern "C" void amomaxu_w(xreg dst, xreg src1, xreg src2);
extern "C" void amoswap_d(xreg dst, xreg src1, xreg src2);
extern "C" void amoadd_d(xreg dst, xreg src1, xreg src2);
extern "C" void amoxor_d(xreg dst, xreg src1, xreg src2);
extern "C" void amoand_d(xreg dst, xreg src1, xreg src2);
extern "C" void amoor_d(xreg dst, xreg src1, xreg src2);
extern "C" void amomin_d(xreg dst, xreg src1, xreg src2);
extern "C" void amomax_d(xreg dst, xreg src1, xreg src2);
extern "C" void amominu_d(xreg dst, xreg src1, xreg src2);
extern "C" void amomaxu_d(xreg dst, xreg src1, xreg src2);
extern "C" void csrrw(xreg dst, csr src1, xreg src2);
extern "C" void csrrs(xreg dst, csr src1, xreg src2);
extern "C" void csrrc(xreg dst, csr src1, xreg src2);
extern "C" void csrrwi(xreg dst, csr src1, uint64 imm);
extern "C" void csrrsi(xreg dst, csr src1, uint64 imm);
extern "C" void csrrci(xreg dst, csr src1, uint64 imm);
extern "C" void sret();
extern "C" void mret();
extern "C" void wfi();
extern "C" void fence();
extern "C" void fence_i();
extern "C" void ecall();
extern "C" void ebreak();
extern "C" void flw   (freg dst, int off, xreg base);
extern "C" void fld   (freg dst, int off, xreg base);
extern "C" void flq   (freg dst, int off, xreg base);
extern "C" void flw_ps(freg dst, int off, xreg base);
extern "C" void fbc_ps(freg dst, int off, xreg base);
extern "C" void fbci_pi(freg dst, uint32 imm);
extern "C" void fbci_ps(freg dst, uint32 imm);
extern "C" void fbcx_ps(freg dst, xreg src);
extern "C" void fsw   (freg src1, int off, xreg base);
extern "C" void fsw_ps(freg src1, int off, xreg base);
extern "C" void fsd   (freg src1, int off, xreg base);
extern "C" void fsq   (freg src1, int off, xreg base);
extern "C" void fadd_s (freg dst, freg src1, freg src2);
extern "C" void fadd_ps (freg dst, freg src1, freg src2);
extern "C" void fsub_s (freg dst, freg src1, freg src2);
extern "C" void fsub_ps(freg dst, freg src1, freg src2);
extern "C" void fmul_s (freg dst, freg src1, freg src2);
extern "C" void fmul_ps(freg dst, freg src1, freg src2);
extern "C" void fdiv_s (freg dst, freg src1, freg src2);
extern "C" void fdiv_ps(freg dst, freg src1, freg src2);
extern "C" void fgw_ps(freg dst, freg src1, xreg base);
extern "C" void fgh_ps(freg dst, freg src1, xreg base);
extern "C" void fgb_ps(freg dst, freg src1, xreg base);
extern "C" void fscw_ps(freg src1, freg src2, xreg base);
extern "C" void fsch_ps(freg src1, freg src2, xreg base);
extern "C" void fscb_ps(freg src1, freg src2, xreg base);
extern "C" void fg32w_ps(freg dst, xreg src1, xreg src2);
extern "C" void fg32h_ps(freg dst, xreg src1, xreg src2);
extern "C" void fg32b_ps(freg dst, xreg src1, xreg src2);
extern "C" void fsc32w_ps(freg src3, xreg src1, xreg src2);
extern "C" void fsc32h_ps(freg src3, xreg src1, xreg src2);
extern "C" void fsc32b_ps(freg src3, xreg src1, xreg src2);
extern "C" void fmadd_s  (freg dst, freg src1, freg src2, freg src3);
extern "C" void fmadd_ps (freg dst, freg src1, freg src2, freg src3);
extern "C" void fmsub_s  (freg dst, freg src1, freg src2, freg src3);
extern "C" void fmsub_ps (freg dst, freg src1, freg src2, freg src3);
extern "C" void fnmadd_s (freg dst, freg src1, freg src2, freg src3);
extern "C" void fnmadd_ps(freg dst, freg src1, freg src2, freg src3);
extern "C" void fnmsub_s (freg dst, freg src1, freg src2, freg src3);
extern "C" void fnmsub_ps(freg dst, freg src1, freg src2, freg src3);
extern "C" void fcmov_ps (freg dst, freg src1, freg src2, freg src3);
extern "C" void fmin_s    (freg dst, freg src1, freg src2);
extern "C" void fmin_ps   (freg dst, freg src1, freg src2);
extern "C" void fmax_s    (freg dst, freg src1, freg src2);
extern "C" void fmax_ps   (freg dst, freg src1, freg src2);
extern "C" void fsgnj_s   (freg dst, freg src1, freg src2);
extern "C" void fsgnj_ps  (freg dst, freg src1, freg src2);
extern "C" void fsgnjn_s  (freg dst, freg src1, freg src2);
extern "C" void fsgnjn_ps (freg dst, freg src1, freg src2);
extern "C" void fsgnjx_s  (freg dst, freg src1, freg src2);
extern "C" void fsgnjx_ps (freg dst, freg src1, freg src2);
extern "C" void flt_ps    (freg dst, freg src1, freg src2);
//extern "C" void fltabs_ps (freg dst, freg src1, freg src2);
extern "C" void fle_ps    (freg dst, freg src1, freg src2);
extern "C" void feq_ps    (freg dst, freg src1, freg src2);
extern "C" void feqm_ps   (mreg dst, freg src1, freg src2);
extern "C" void fltm_ps   (mreg dst, freg src1, freg src2);
extern "C" void frcp_fix_rast(freg dst, freg src1, freg src2);
extern "C" void flem_ps   (mreg dst, freg src1, freg src2);
extern "C" void fsetm_ps  (mreg dst, freg src1);
extern "C" void fclass_s  (freg dst, freg src1);
extern "C" void fclass_ps (freg dst, freg src1);
extern "C" void fsqrt_s  (freg dst, freg src1);
extern "C" void fsqrt_ps (freg dst, freg src1);
extern "C" void frsq_ps  (freg dst, freg src1);
extern "C" void fsin_ps  (freg dst, freg src1);
//extern "C" void fcos_ps  (freg dst, freg src1);
extern "C" void fexp_ps  (freg dst, freg src1);
extern "C" void flog_ps  (freg dst, freg src1);
extern "C" void frcp_ps  (freg dst, freg src1);
extern "C" void frcpfxp_ps  (freg dst, freg src1);
extern "C" void fcvt_pw_ps  (freg dst, freg src1);
extern "C" void fcvt_pwu_ps (freg dst, freg src1);
extern "C" void fclass_d  (freg dst, freg src1);
extern "C" void fcvt_d_s  (freg dst, freg src1);
extern "C" void fcvt_s_d  (freg dst, freg src1);
extern "C" void fcvt_s_w  (freg dst, freg src1);
extern "C" void fcvt_s_wu (freg dst, freg src1);
extern "C" void fcvt_w_s  (freg dst, freg src1, rounding_mode rm);
void fcvt_w_s  (freg dst, freg src1);
extern "C" void fcvt_wu_s (freg dst, freg src1);
extern "C" void fcvt_d_w  (freg dst, freg src1);
extern "C" void fcvt_d_wu (freg dst, freg src1);
extern "C" void fcvt_d_l  (freg dst, freg src1);
extern "C" void fcvt_d_lu (freg dst, freg src1);
extern "C" void fcvt_w_d  (freg dst, freg src1);
extern "C" void fcvt_wu_d (freg dst, freg src1);
extern "C" void fcvt_l_d  (freg dst, freg src1);
extern "C" void fcvt_lu_d (freg dst, freg src1);
extern "C" void fcvt_ps_pw  (freg dst, freg src1);
extern "C" void fcvt_ps_pwu (freg dst, freg src1);
extern "C" void fcvt_ps_rast (freg dst, freg src1);
extern "C" void fcvt_rast_ps (freg dst, freg src1);
extern "C" void ffrc_ps (freg dst, freg src1);
extern "C" void fround_ps (freg dst, freg src1, rounding_mode rm);
extern "C" void fnot_pi (freg dst, freg src1);
extern "C" void fsat8_pi(freg dst, freg src1);
extern "C" void fadd_pi (freg dst, freg src1, freg src2);
extern "C" void fsub_pi (freg dst, freg src1, freg src2);
extern "C" void fmul_pi (freg dst, freg src1, freg src2);
extern "C" void fmulh_pi (freg dst, freg src1, freg src2);
extern "C" void fmulhu_pi (freg dst, freg src1, freg src2);
//extern "C" void fmulhsu_pi (freg dst, freg src1, freg src2);
extern "C" void fdiv_pi (freg dst, freg src1, freg src2);
extern "C" void fdivu_pi (freg dst, freg src1, freg src2);
extern "C" void frem_pi (freg st, freg src1, freg src2);
extern "C" void fremu_pi (freg st, freg src1, freg src2);
extern "C" void fmax_pi (freg dst, freg src1, freg src2);
extern "C" void fmin_pi (freg dst, freg src1, freg src2);
extern "C" void fmaxu_pi (freg dst, freg src1, freg src2);
extern "C" void fminu_pi (freg dst, freg src1, freg src2);
extern "C" void fand_pi (freg dst, freg src1, freg src2);
extern "C" void for_pi  (freg dst, freg src1, freg src2);
extern "C" void fxor_pi (freg dst, freg src1, freg src2);
extern "C" void fsll_pi (freg dst, freg src1, freg src2);
extern "C" void fsrl_pi (freg dst, freg src1, freg src2);
extern "C" void fsra_pi (freg dst, freg src1, freg src2);
extern "C" void flt_pi   (freg dst, freg src1, freg src2);
extern "C" void fltu_pi  (freg dst, freg src1, freg src2);
extern "C" void fle_pi   (freg dst, freg src1, freg src2);
extern "C" void feq_pi   (freg dst, freg src1, freg src2);
extern "C" void fltm_pi  (mreg dst, freg src1, freg src2);
extern "C" void faddi_pi (freg dst, freg src1, uint32 imm);
extern "C" void fandi_pi (freg dst, freg src1, uint32 imm);
extern "C" void fori_pi  (freg dst, freg src1, uint32 imm);
extern "C" void fxori_pi (freg dst, freg src1, uint32 imm);
extern "C" void fslli_pi (freg dst, freg src1, uint32 imm);
extern "C" void fsrli_pi (freg dst, freg src1, uint32 imm);
extern "C" void fsrai_pi (freg dst, freg src1, uint32 imm);
//extern "C" void fslloi_pi (freg dst, freg src1, freg src2, uint32 imm);
extern "C" void fpackreph_pi (freg dst, freg src1);
extern "C" void fpackrepb_pi (freg dst, freg src1);

extern "C" void feq_s        (xreg dst, freg src1, freg src2);
extern "C" void fle_s        (xreg dst, freg src1, freg src2);
extern "C" void flt_s        (xreg dst, freg src1, freg src2);
extern "C" void fcvt_ps_f16  (freg dst, freg src1);
extern "C" void fcvt_ps_un24 (freg dst, freg src1);
extern "C" void fcvt_ps_un16 (freg dst, freg src1);
extern "C" void fcvt_ps_un10 (freg dst, freg src1);
extern "C" void fcvt_ps_un8  (freg dst, freg src1);
extern "C" void fcvt_ps_un2  (freg dst, freg src1);
//extern "C" void fcvt_ps_sn24 (freg dst, freg src1);
extern "C" void fcvt_ps_sn16 (freg dst, freg src1);
//extern "C" void fcvt_ps_sn10 (freg dst, freg src1);
extern "C" void fcvt_ps_sn8  (freg dst, freg src1);
//extern "C" void fcvt_ps_sn2  (freg dst, freg src1);
extern "C" void fcvt_ps_f11  (freg dst, freg src1);
extern "C" void fcvt_ps_f10  (freg dst, freg src1);

extern "C" void fcvt_f16_ps  (freg dst, freg src1, rounding_mode rm);
extern "C" void fcvt_un24_ps (freg dst, freg src1, rounding_mode rm);
extern "C" void fcvt_un16_ps (freg dst, freg src1, rounding_mode rm);
extern "C" void fcvt_un10_ps (freg dst, freg src1, rounding_mode rm);
extern "C" void fcvt_un8_ps  (freg dst, freg src1, rounding_mode rm);
extern "C" void fcvt_un2_ps  (freg dst, freg src1, rounding_mode rm);
//extern "C" void fcvt_sn24_ps (freg dst, freg src1, rounding_mode rm);
extern "C" void fcvt_sn16_ps (freg dst, freg src1, rounding_mode rm);
extern "C" void fcvt_sn8_ps  (freg dst, freg src1, rounding_mode rm);
extern "C" void fcvt_f11_ps  (freg dst, freg src1, rounding_mode rm);
extern "C" void fcvt_f10_ps  (freg dst, freg src1, rounding_mode rm);

extern "C" void fswizz_ps    (freg dst, freg src1, uint8  imm);

extern "C" void cubeface_ps    (freg dst, freg src1, freg src2);
extern "C" void cubefaceidx_ps (freg dst, freg src1, freg src2);
extern "C" void cubesgnsc_ps   (freg dst, freg src1, freg src2);
extern "C" void cubesgntc_ps   (freg dst, freg src1, freg src2);

extern "C" void fmvz_x_ps     (xreg dst, freg src1, uint8 index);
extern "C" void fmvs_x_ps     (xreg dst, freg src1, uint8 index);
extern "C" void fcmovm_ps     (freg dst, freg src1, freg src2);

extern "C" void fmv_x_w      (xreg dst, freg src1);
extern "C" void fmv_w_x      (freg dst, xreg src1);
extern "C" void fmv_x_s      (xreg dst, freg src1);
extern "C" void fmv_s_x      (freg dst, xreg src1);

// Double precision instructions
extern "C" void fmadd_d  (freg dst, freg src1, freg src2, freg src3);
extern "C" void feq_d    (freg dst, freg src1, freg src2);
extern "C" void fmin_d   (freg dst, freg src1, freg src2);
extern "C" void fmax_d   (freg dst, freg src1, freg src2);
extern "C" void fadd_d   (freg dst, freg src1, freg src2);
extern "C" void fdiv_d   (freg dst, freg src1, freg src2);
extern "C" void fmul_d   (freg dst, freg src1, freg src2);
extern "C" void fsgnj_d  (freg dst, freg src1, freg src2);
extern "C" void fmv_d_x  (xreg dst, freg src1);
extern "C" void fmv_x_d  (xreg dst, freg src1);
extern "C" void fcvt_d_w (freg dst, freg src1);

// Mask instructions
extern "C" void maskand    (mreg dst, mreg src1, mreg src2);
extern "C" void maskor     (mreg dst, mreg src1, mreg src2);
extern "C" void maskxor    (mreg dst, mreg src1, mreg src2);
extern "C" void masknot    (mreg dst, mreg src1);
extern "C" void mova_x_m   (xreg dst);
extern "C" void mova_m_x   (xreg src1);
extern "C" void mov_m_x    (mreg dst, xreg src1, uint32 imm);
extern "C" void movi_m     (mreg dst, uint8 imm);
extern "C" void maskpopc   (xreg dst, mreg src1);
extern "C" void maskpopcz  (xreg dst, mreg src1);
extern "C" void maskpopc_rast (xreg dst, mreg src1, mreg src2, uint32 imm);

// Texture instructions
#include "txs.h"

// Special scalar instructions for graphics
extern "C" void packb(xreg dst, xreg src1, xreg src2);
extern "C" void bitmixb(xreg dst, xreg src1, xreg src2);

// Illegal instruction encodings will execute this
extern "C" void unknown();

#endif // _EMU_H
