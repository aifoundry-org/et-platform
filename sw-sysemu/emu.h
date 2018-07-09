#ifndef _EMU_H
#define _EMU_H

#include <list>

#include "emu_defines.h"

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

extern uint8_t in_sysemu;
extern uint8_t emu_use_fake_txfma;
extern uint32_t current_thread;

void print_regs();
void print_comment(const char *comm);

extern void init_emu(int debug, int fakesam);
extern void minit(mreg dst, uint64_t val);
extern void init(xreg dst, uint64_t val);
extern uint64_t xget(uint64_t src1);
extern uint64_t csrget(csr src1);
extern void fpinit(freg dst, uint64_t val[2]);
extern void initcsr(uint32_t thread);
extern void init_stack();
extern void set_pc(uint64_t pc);
extern void set_thread(uint32_t thread);
extern uint32_t get_thread();
extern uint32_t get_mask(unsigned maskNr);
extern void get_reduce_info(uint64_t value, uint64_t * other_min, uint64_t * action);
extern uint64_t get_reduce_value(int entry, int block, int * size, int * start_entry);
extern uint64_t get_scratchpad_value(int entry, int block, int * last_entry, int * size);
extern void get_scratchpad_conv_list(std::list<bool> * list);
extern uint64_t get_tensorfma_value(int entry, int pass, int block, int * size, int * passes, bool * conv_skip);
extern uint64_t virt_to_phys_emu(uint64_t addr, mem_access_type macc);

extern void set_msg_port_data_func(void* f, void *g, void *h);
extern bool get_msg_port_stall(uint32_t thread, uint32_t id);
extern void write_msg_port_data_(uint32_t thread, uint32_t port_id, uint32_t *data);
extern void update_msg_port_data();

uint8_t memread8(uint64_t addr, bool trans = true);
uint16_t memread16(uint64_t addr, bool trans = true);
uint32_t memread32(uint64_t addr, bool trans = true);
uint64_t memread64(uint64_t addr, bool trans = true);
void memwrite8(uint64_t addr, uint8_t data, bool trans = true);
void memwrite16(uint64_t addr, uint16_t data, bool trans = true);
void memwrite32(uint64_t addr, uint32_t data, bool trans = true);
void memwrite64(uint64_t addr, uint64_t data, bool trans = true);

extern void lb(xreg dst, int off, xreg base);
extern void lh(xreg dst, int off, xreg base);
extern void lw(xreg dst, int off, xreg base);
extern void ld(xreg dst, int off, xreg base);
extern void lbu(xreg dst, int off, xreg base);
extern void lhu(xreg dst, int off, xreg base);
extern void lwu(xreg dst, int off, xreg base);
extern void addi(xreg dst, xreg src1, int imm);
extern void addiw(xreg dst, xreg src1, int imm);
extern void slt(xreg dst, xreg src1, xreg src2);
extern void sltu(xreg dst, xreg src1, xreg src2);
extern void slti(xreg dst, xreg src1, int imm);
extern void sltiu(xreg dst, xreg src1, int imm);
extern void mul(xreg dst, xreg src1, xreg src2);
extern void mulw(xreg dst, xreg src1, xreg src2);
extern void mulh(xreg dst, xreg src1, xreg src2);
extern void mulhu(xreg dst, xreg src1, xreg src2);
extern void div_(xreg dst, xreg src1, xreg src2);
extern void divu(xreg dst, xreg src1, xreg src2);
extern void divw(xreg dst, xreg src1, xreg src2);
extern void divuw(xreg dst, xreg src1, xreg src2);
extern void rem(xreg dst, xreg src1, xreg src2);
extern void remu(xreg dst, xreg src1, xreg src2);
extern void remw(xreg dst, xreg src1, xreg src2);
extern void remuw(xreg dst, xreg src1, xreg src2);
extern void add(xreg dst, xreg src1, xreg src2);
extern void addw(xreg dst, xreg src1, xreg src2);
extern void sub(xreg dst, xreg src1, xreg src2);
extern void subw(xreg dst, xreg src1, xreg src2);
extern void ori(xreg dst, xreg src1, int imm);
extern void or_(xreg dst, xreg src1, xreg src2);
extern void andi(xreg dst, xreg src1, int imm);
extern void and_(xreg dst, xreg src1, xreg src2);
extern void xori(xreg dst, xreg src1, int imm);
extern void xor_(xreg dst, xreg src1, xreg src2);
extern void lui(xreg dst, int imm);
extern void auipc(xreg dst, int imm);
extern void sllw(xreg dst, xreg src1, xreg src2);
extern void sll(xreg dst, xreg src1, xreg src2);
extern void slliw(xreg dst, xreg src1, int imm);
extern void slli(xreg dst, xreg src1, int imm);
extern void srlw(xreg dst, xreg src1, xreg src2);
extern void srl(xreg dst, xreg src1, xreg src2);
extern void srliw(xreg dst, xreg src1, int imm);
extern void srli(xreg dst, xreg src1, int imm);
extern void sra(xreg dst, xreg src1, xreg src2);
extern void sraw(xreg dst, xreg src1, xreg src2);
extern void srai(xreg dst, xreg src1, int imm);
extern void sraiw(xreg dst, xreg src1, int imm);
extern void jal(xreg dst, int imm);
extern void jalr(xreg dst, xreg src1, int imm);
//extern void c_jalr(xreg dst, xreg src1, int imm);
extern void beq(xreg src1, xreg src2, int imm);
extern void bne(xreg src1, xreg src2, int imm);
extern void blt(xreg src1, xreg src2, int imm);
extern void bltu(xreg src1, xreg src2, int imm);
extern void bge(xreg src1, xreg src2, int imm);
extern void bgeu(xreg src1, xreg src2, int imm);
extern void sd(xreg src1, int off, xreg base);
extern void sw(xreg src1, int off, xreg base);
extern void sh(xreg src1, int off, xreg base);
extern void sb(xreg src1, int off, xreg base);
extern void amoswap_w(xreg dst, xreg src1, xreg src2);
extern void amoadd_w(xreg dst, xreg src1, xreg src2);
extern void amoxor_w(xreg dst, xreg src1, xreg src2);
extern void amoand_w(xreg dst, xreg src1, xreg src2);
extern void amoor_w(xreg dst, xreg src1, xreg src2);
extern void amomin_w(xreg dst, xreg src1, xreg src2);
extern void amomax_w(xreg dst, xreg src1, xreg src2);
extern void amominu_w(xreg dst, xreg src1, xreg src2);
extern void amomaxu_w(xreg dst, xreg src1, xreg src2);
extern void amoswap_d(xreg dst, xreg src1, xreg src2);
extern void amoadd_d(xreg dst, xreg src1, xreg src2);
extern void amoxor_d(xreg dst, xreg src1, xreg src2);
extern void amoand_d(xreg dst, xreg src1, xreg src2);
extern void amoor_d(xreg dst, xreg src1, xreg src2);
extern void amomin_d(xreg dst, xreg src1, xreg src2);
extern void amomax_d(xreg dst, xreg src1, xreg src2);
extern void amominu_d(xreg dst, xreg src1, xreg src2);
extern void amomaxu_d(xreg dst, xreg src1, xreg src2);
extern void csrrw(xreg dst, csr src1, xreg src2);
extern void csrrs(xreg dst, csr src1, xreg src2);
extern void csrrc(xreg dst, csr src1, xreg src2);
extern void csrrwi(xreg dst, csr src1, uint64_t imm);
extern void csrrsi(xreg dst, csr src1, uint64_t imm);
extern void csrrci(xreg dst, csr src1, uint64_t imm);
extern void sret();
extern void mret();
extern void wfi();
extern void fence();
extern void fence_i();
extern void ecall();
extern void ebreak();
extern void flw       (freg dst, int off, xreg base);
extern void flq       (freg dst, int off, xreg base);
extern void flw_ps    (freg dst, int off, xreg base);
extern void fbc_ps    (freg dst, int off, xreg base);
extern void fbci_pi   (freg dst, uint32_t imm);
extern void fbci_ps   (freg dst, uint32_t imm);          // no rounding mode
extern void fbcx_ps   (freg dst, xreg src);            // no rounding mode
extern void fsw       (freg src1, int off, xreg base);
extern void fsw_ps    (freg src1, int off, xreg base);
extern void fsq       (freg src1, int off, xreg base);
extern void fadd_s    (freg dst, freg src1, freg src2, rounding_mode rm);
extern void fadd_ps   (freg dst, freg src1, freg src2, rounding_mode rm);
extern void fsub_s    (freg dst, freg src1, freg src2, rounding_mode rm);
extern void fsub_ps   (freg dst, freg src1, freg src2, rounding_mode rm);
extern void fmul_s    (freg dst, freg src1, freg src2, rounding_mode rm);
extern void fmul_ps   (freg dst, freg src1, freg src2, rounding_mode rm);
extern void fdiv_s    (freg dst, freg src1, freg src2, rounding_mode rm);
extern void fdiv_ps   (freg dst, freg src1, freg src2, rounding_mode rm);
extern void fgw_ps    (freg dst, freg src1, xreg base);
extern void fgh_ps    (freg dst, freg src1, xreg base);
extern void fgb_ps    (freg dst, freg src1, xreg base);
extern void fscw_ps   (freg src1, freg src2, xreg base);
extern void fsch_ps   (freg src1, freg src2, xreg base);
extern void fscb_ps   (freg src1, freg src2, xreg base);
extern void fg32w_ps  (freg dst, xreg src1, xreg src2);
extern void fg32h_ps  (freg dst, xreg src1, xreg src2);
extern void fg32b_ps  (freg dst, xreg src1, xreg src2);
extern void fsc32w_ps (freg src3, xreg src1, xreg src2);
extern void fsc32h_ps (freg src3, xreg src1, xreg src2);
extern void fsc32b_ps (freg src3, xreg src1, xreg src2);
extern void fmadd_s   (freg dst, freg src1, freg src2, freg src3, rounding_mode rm);
extern void fmadd_ps  (freg dst, freg src1, freg src2, freg src3, rounding_mode rm);
extern void fmsub_s   (freg dst, freg src1, freg src2, freg src3, rounding_mode rm);
extern void fmsub_ps  (freg dst, freg src1, freg src2, freg src3, rounding_mode rm);
extern void fnmadd_s  (freg dst, freg src1, freg src2, freg src3, rounding_mode rm);
extern void fnmadd_ps (freg dst, freg src1, freg src2, freg src3, rounding_mode rm);
extern void fnmsub_s  (freg dst, freg src1, freg src2, freg src3, rounding_mode rm);
extern void fnmsub_ps (freg dst, freg src1, freg src2, freg src3, rounding_mode rm);
extern void fcmov_ps  (freg dst, freg src1, freg src2, freg src3);   // no rounding mode
extern void fmin_s    (freg dst, freg src1, freg src2);     // no rounding mode
extern void fmin_ps   (freg dst, freg src1, freg src2);     // no rounding mode
extern void fmax_s    (freg dst, freg src1, freg src2);     // no rounding mode
extern void fmax_ps   (freg dst, freg src1, freg src2);     // no rounding mode
extern void fsgnj_s   (freg dst, freg src1, freg src2);     // no rounding mode
extern void fsgnj_ps  (freg dst, freg src1, freg src2);     // no rounding mode
extern void fsgnjn_s  (freg dst, freg src1, freg src2);     // no rounding mode
extern void fsgnjn_ps (freg dst, freg src1, freg src2);     // no rounding mode
extern void fsgnjx_s  (freg dst, freg src1, freg src2);     // no rounding mode
extern void fsgnjx_ps (freg dst, freg src1, freg src2);     // no rounding mode
extern void flt_ps    (freg dst, freg src1, freg src2);     // no rounding mode
//extern void fltabs_ps (freg dst, freg src1, freg src2, rounding_mode rm);
extern void fle_ps    (freg dst, freg src1, freg src2);     // no rounding mode
extern void feq_ps    (freg dst, freg src1, freg src2);     // no rounding mode
extern void feqm_ps   (mreg dst, freg src1, freg src2);     // no rounding mode
extern void fltm_ps   (mreg dst, freg src1, freg src2);     // no rounding mode
extern void frcp_fix_rast(freg dst, freg src1, freg src2);  // no rounding mode
extern void flem_ps      (mreg dst, freg src1, freg src2);     // no rounding mode
extern void fsetm_ps     (mreg dst, freg src1);                // no rounding mode
extern void fclass_s     (freg dst, freg src1);                // no rounding mode
extern void fclass_ps    (freg dst, freg src1);                // no rounding mode
extern void fsqrt_s      (freg dst, freg src1, rounding_mode rm);
extern void fsqrt_ps     (freg dst, freg src1, rounding_mode rm);
extern void frsq_ps      (freg dst, freg src1, rounding_mode rm);
extern void fsin_ps      (freg dst, freg src1, rounding_mode rm);
//extern void fcos_ps    (freg dst, freg src1, rounding_mode rm);
extern void fexp_ps      (freg dst, freg src1, rounding_mode rm);
extern void flog_ps      (freg dst, freg src1, rounding_mode rm);
extern void frcp_ps      (freg dst, freg src1, rounding_mode rm);
extern void frcpfxp_ps   (freg dst, freg src1, rounding_mode rm);
extern void fcvt_pw_ps   (freg dst, freg src1, rounding_mode rm);
extern void fcvt_pwu_ps  (freg dst, freg src1, rounding_mode rm);
extern void fcvt_s_w     (freg dst, freg src1, rounding_mode rm);
extern void fcvt_s_wu    (freg dst, freg src1, rounding_mode rm);
extern void fcvt_w_s     (freg dst, freg src1, rounding_mode rm);
extern void fcvt_wu_s    (freg dst, freg src1, rounding_mode rm);
extern void fcvt_ps_pw   (freg dst, freg src1, rounding_mode rm);
extern void fcvt_ps_pwu  (freg dst, freg src1, rounding_mode rm);
extern void fcvt_ps_rast (freg dst, freg src1, rounding_mode rm);
extern void fcvt_rast_ps (freg dst, freg src1, rounding_mode rm);
extern void ffrc_ps   (freg dst, freg src1, rounding_mode rm);
extern void fround_ps (freg dst, freg src1, rounding_mode rm);
extern void fnot_pi   (freg dst, freg src1);
extern void fsat8_pi  (freg dst, freg src1);
extern void fadd_pi   (freg dst, freg src1, freg src2);
extern void fsub_pi   (freg dst, freg src1, freg src2);
extern void fmul_pi   (freg dst, freg src1, freg src2);
extern void fmulh_pi  (freg dst, freg src1, freg src2);
extern void fmulhu_pi (freg dst, freg src1, freg src2);
//extern void fmulhsu_pi (freg dst, freg src1, freg src2);
extern void fdiv_pi  (freg dst, freg src1, freg src2);
extern void fdivu_pi (freg dst, freg src1, freg src2);
extern void frem_pi  (freg st, freg src1, freg src2);
extern void fremu_pi (freg st, freg src1, freg src2);
extern void fmax_pi  (freg dst, freg src1, freg src2);
extern void fmin_pi  (freg dst, freg src1, freg src2);
extern void fmaxu_pi (freg dst, freg src1, freg src2);
extern void fminu_pi (freg dst, freg src1, freg src2);
extern void fand_pi  (freg dst, freg src1, freg src2);
extern void for_pi   (freg dst, freg src1, freg src2);
extern void fxor_pi  (freg dst, freg src1, freg src2);
extern void fsll_pi  (freg dst, freg src1, freg src2);
extern void fsrl_pi  (freg dst, freg src1, freg src2);
extern void fsra_pi  (freg dst, freg src1, freg src2);
extern void flt_pi   (freg dst, freg src1, freg src2);
extern void fltu_pi  (freg dst, freg src1, freg src2);
extern void fle_pi   (freg dst, freg src1, freg src2);
extern void feq_pi   (freg dst, freg src1, freg src2);
extern void fltm_pi  (mreg dst, freg src1, freg src2);
extern void faddi_pi (freg dst, freg src1, uint32_t imm);
extern void fandi_pi (freg dst, freg src1, uint32_t imm);
extern void fori_pi  (freg dst, freg src1, uint32_t imm);
extern void fxori_pi (freg dst, freg src1, uint32_t imm);
extern void fslli_pi (freg dst, freg src1, uint32_t imm);
extern void fsrli_pi (freg dst, freg src1, uint32_t imm);
extern void fsrai_pi (freg dst, freg src1, uint32_t imm);
//extern void fslloi_pi (freg dst, freg src1, freg src2, uint32_t imm);
extern void fpackreph_pi (freg dst, freg src1);
extern void fpackrepb_pi (freg dst, freg src1);

extern void feq_s        (xreg dst, freg src1, freg src2);  // no rounding mode
extern void fle_s        (xreg dst, freg src1, freg src2);  // no rounding mode
extern void flt_s        (xreg dst, freg src1, freg src2);  // no rounding mode
extern void fcvt_ps_f16  (freg dst, freg src1, rounding_mode rm);
extern void fcvt_ps_un24 (freg dst, freg src1, rounding_mode rm);
extern void fcvt_ps_un16 (freg dst, freg src1, rounding_mode rm);
extern void fcvt_ps_un10 (freg dst, freg src1, rounding_mode rm);
extern void fcvt_ps_un8  (freg dst, freg src1, rounding_mode rm);
extern void fcvt_ps_un2  (freg dst, freg src1, rounding_mode rm);
//extern void fcvt_ps_sn24 (freg dst, freg src1, rounding_mode rm);
extern void fcvt_ps_sn16 (freg dst, freg src1, rounding_mode rm);
//extern void fcvt_ps_sn10 (freg dst, freg src1, rounding_mode rm);
extern void fcvt_ps_sn8  (freg dst, freg src1, rounding_mode rm);
//extern void fcvt_ps_sn2  (freg dst, freg src1, rounding_mode rm);
extern void fcvt_ps_f11  (freg dst, freg src1, rounding_mode rm);
extern void fcvt_ps_f10  (freg dst, freg src1, rounding_mode rm);

extern void fcvt_f16_ps  (freg dst, freg src1, rounding_mode rm);
extern void fcvt_un24_ps (freg dst, freg src1, rounding_mode rm);
extern void fcvt_un16_ps (freg dst, freg src1, rounding_mode rm);
extern void fcvt_un10_ps (freg dst, freg src1, rounding_mode rm);
extern void fcvt_un8_ps  (freg dst, freg src1, rounding_mode rm);
extern void fcvt_un2_ps  (freg dst, freg src1, rounding_mode rm);
//extern void fcvt_sn24_ps (freg dst, freg src1, rounding_mode rm);
extern void fcvt_sn16_ps (freg dst, freg src1, rounding_mode rm);
extern void fcvt_sn8_ps  (freg dst, freg src1, rounding_mode rm);
extern void fcvt_f11_ps  (freg dst, freg src1, rounding_mode rm);
extern void fcvt_f10_ps  (freg dst, freg src1, rounding_mode rm);

extern void fswizz_ps    (freg dst, freg src1, uint8_t  imm);         // no rounding mode

extern void cubeface_ps    (freg dst, freg src1, freg src2);        // no rounding mode
extern void cubefaceidx_ps (freg dst, freg src1, freg src2);        // no rounding mode
extern void cubesgnsc_ps   (freg dst, freg src1, freg src2);        // no rounding mode
extern void cubesgntc_ps   (freg dst, freg src1, freg src2);        // no rounding mode

extern void fmvz_x_ps     (xreg dst, freg src1, uint8_t index);       // no rounding mode
extern void fmvs_x_ps     (xreg dst, freg src1, uint8_t index);       // no rounding mode
extern void fcmovm_ps     (freg dst, freg src1, freg src2);         // no rounding mode

extern void fmv_x_w      (xreg dst, freg src1);     // no rounding mode
extern void fmv_w_x      (freg dst, xreg src1);     // no rounding mode

// Mask instructions
extern void maskand       (mreg dst, mreg src1, mreg src2);
extern void maskor        (mreg dst, mreg src1, mreg src2);
extern void maskxor       (mreg dst, mreg src1, mreg src2);
extern void masknot       (mreg dst, mreg src1);
extern void mova_x_m      (xreg dst);
extern void mova_m_x      (xreg src1);
extern void mov_m_x       (mreg dst, xreg src1, uint32_t imm);
extern void maskpopc      (xreg dst, mreg src1);
extern void maskpopcz     (xreg dst, mreg src1);
extern void maskpopc_rast (xreg dst, mreg src1, mreg src2, uint32_t imm);

// Texture instructions
#include "txs.h"

// Special scalar instructions for graphics
extern void packb(xreg dst, xreg src1, xreg src2);
extern void bitmixb(xreg dst, xreg src1, xreg src2);

// Illegal instruction encodings will execute this
extern void unknown();

// Functions used by the checker/sys_emu
extern void set_memory_funcs(void * func_memread8_, void * func_memread16_,
                             void * func_memread32_, void * func_memread64_,
                             void * func_memwrite8_, void * func_memwrite16_,
                             void * func_memwrite32_, void * func_memwrite64_);

#endif // _EMU_H
