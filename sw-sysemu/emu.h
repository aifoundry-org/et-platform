
#ifndef EMULH
#define EMULH

#include <list>

#define DEBUG_EMU 1
#define DEBUG_MASK 1
#define DISASM 1

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

// set to 1 if floating point 32 operation sets bits 127:32 to 0,
// and 64 bits operations set bits 127:64 to 0
#define ZERO_EXTEND_UNUSED_FREG_BITS

#define CSR_PRV_U  0
#define CSR_PRV_S  1
#define CSR_PRV_H  2
#define CSR_PRV_M  3

// CSRs
typedef enum
{
    csr_mhartid,
    csr_fcsr,
    csr_frm,
    csr_mip,
    csr_misa,
    csr_mtvec,
    csr_medeleg,
    csr_mideleg,
    csr_mie,
    csr_stvec,
    csr_prv,
    csr_mstatus,
    csr_mepc,
    csr_mcause,
    csr_sstatus,
    csr_fflags,
    csr_mt1rvect,
    csr_mt1en,
    csr_satp,
    csr_cacheop,
    csr_icache_ctrl,
    csr_tconvctrl,
    csr_tconvsize,
    csr_tloadctrl,
    csr_tfmastart,
    csr_tstore,
    csr_reduce,
    write_ctrl,
    csr_smsg_port0,
    csr_smsg_port1,
    csr_smsg_port2,
    csr_smsg_port3,
    csr_msg_port0,
    csr_msg_port1,
    csr_msg_port2,
    csr_msg_port3,
    CSR_MAX
} csr;

// Used to access different threads transparently
#define XREGS xregs[current_thread]
#define FREGS fregs[current_thread]
#define MREGS mregs[current_thread]
#define SCP   scp[current_thread]

typedef enum
{
        m0 = 0,
        m1 = 1,
        m2 = 2,
        m3 = 3,
        m4 = 4,
        m5 = 5,
        m6 = 6,
        m7 = 7,
        MAXMREG = 8,
        mnone = -1
} mreg;

typedef enum
{
        x0 = 0,
        x1 = 1,
        x2 = 2,
        x3 = 3,
        x4 = 4,
        x5 = 5,
        x6 = 6,
        x7 = 7,
        x8 = 8,
        x9 = 9,
        x10 = 10,
        x11 = 11,
        x12 = 12,
        x13 = 13,
        x14 = 14,
        x15 = 15,
        x16 = 16,
        x17 = 17,
        x18 = 18,
        x19 = 19,
        x20 = 20,
        x21 = 21,
        x22 = 22,
        x23 = 23,
        x24 = 24,
        x25 = 25,
        x26 = 26,
        x27 = 27,
        x28 = 28,
        x29 = 29,
        x30 = 30,
        x31 = 31,
        MAXXREG = 32,
        xnone = -1
} xreg;

typedef enum
{
        f0 = 0,
        f1 = 1,
        f2 = 2,
        f3 = 3,
        f4 = 4,
        f5 = 5,
        f6 = 6,
        f7 = 7,
        f8 = 8,
        f9 = 9,
        f10 = 10,
        f11 = 11,
        f12 = 12,
        f13 = 13,
        f14 = 14,
        f15 = 15,
        f16 = 16,
        f17 = 17,
        f18 = 18,
        f19 = 19,
        f20 = 20,
        f21 = 21,
        f22 = 22,
        f23 = 23,
        f24 = 24,
        f25 = 25,
        f26 = 26,
        f27 = 27,
        f28 = 28,
        f29 = 29,
        f30 = 30,
        f31 = 31,
        MAXFREG,
        fnone = -1
} freg;

typedef enum
{
        rne = 0,
        rtz = 1,
        rdn = 2,
        rup = 3,
        rmm = 4,
        rmdyn = 7 //dynamic rounding mode, read from rm register
} rounding_mode;


typedef double             float64;
typedef float              float32;
typedef __uint128_t        uint128;
typedef __int128_t         int128;
typedef unsigned long long uint64;
typedef   signed long long  int64;
typedef unsigned int       uint32;
typedef   signed int        int32;
typedef unsigned short     uint16;
typedef   signed short      int16;
typedef unsigned char      uint8;
typedef   signed char       int8;

typedef union
{
  uint8   b[16];
  uint16  h[8];
  uint32  u[4];
  int32   i[4];
  float32 f[4];
  uint64  x[2];
  int64   q[2];
  float64 d[2];
} fdata;

typedef union
{
  uint8   b[8];
  uint16  h[4];
  uint32  w[2];
  int32   ws[2];
  uint64  x;
  int64   xs;
} xdata;

typedef union
{
  uint8   b[8];
} mdata;

typedef union
{
        int32   i;
        uint32  u;
        uint64  x;
        int64   xs;
        float32 f;
        float64 d;
} iufval;

typedef struct {
  bool enabled;
  uint8 logsize;
  uint8 max_msgs;
  uint8 scp_set;
  uint8 scp_way;
  uint8 rd_ptr;
  uint8 wr_ptr;
  bool stall;
} msg_port_conf;

typedef enum {
  MSG_ENABLE = 7,
  MSG_DISABLE = 3,
  MSG_PGET = 0,
  MSG_PGETNB = 1,
} msg_port_conf_action;

// Maximum number of threads
#define EMU_NUM_MINIONS 4096
#define EMU_THREADS_PER_MINION 2
#define EMU_NUM_THREADS (EMU_NUM_MINIONS*EMU_THREADS_PER_MINION)
#define NR_MSG_PORTS 4

extern xdata xregs[EMU_NUM_THREADS][32];
extern fdata fregs[EMU_NUM_THREADS][32];
extern mdata mregs[EMU_NUM_THREADS][8];
extern fdata scp[EMU_NUM_THREADS][64][4];

typedef enum
{
        // PS memory instructions
        FLW,
        FLD,
        FSW,
        FSD,
        FSWB,
        FSWH,
        FBC,
        FBCI,
        FGW,
        FGH,
        FGB,
        FSCW,
        FSCH,
        FSCB,
        FG32W,
        FG32H,
        FG32B,
        FSC32W,
        FSC32H,
        FSC32B,
        // PS computation instructions
        FADD,
        FSUB,
        FMUL,
        FDIV,
        FMIN,
        FMAX,
        FMADD,
        FMSUB,
        FNMADD,
        FNMSUB,
        // PS 1-source
        FSQRT,
        FRSQ,
        FSIN,
        //FCOS,
        FEXP,
        FLOG,
        FRCP,
        FRCPFXP,
        FCVTDS,
        FCVTSD,
        FCVTPSPW,
        FCVTPSPWU,
        FFRC,
        FROUND,
        FSWIZZ,
        FCMOV, // PS conversion and move
        FCVTPWPS,
        FCVTPWUPS,
        FSGNJ,
        FSGNJN,
        FSGNJX,
        FMVZXPS,  // warning: unimplemented
        FMVSXPS,  // warning: unimplemented
        FEQ, // Floating point compare
        FLE,
        FLT,
        //FLTABS,
        CUBEFACE,
        CUBEFACEIDX,
        CUBESGNSC,
        CUBESGNTC,
        FCLASS, // warning: unimplemented
        FCVTPSF16, // Graphics Upconvert to PS
        FCVTPSF11,
        FCVTPSF10,
        FCVTPSUN24,
        FCVTPSUN16,
        FCVTPSUN10,
        FCVTPSUN8,
        FCVTPSUN2,
        //FCVTPSSN24,
        FCVTPSSN16,
        //FCVTPSSN10,
        FCVTPSSN8,
        //FCVTPSSN2,
        FCVTF16PS, // Graphics DownConvert from PS
        FCVTF11PS,
        FCVTF10PS,
        FCVTUN24PS,
        FCVTUN16PS,
        FCVTUN10PS,
        FCVTUN8PS,
        FCVTUN2PS,
        //FCVTSN24PS,
        FCVTSN16PS,
        FCVTSN8PS,
        MAND, // Mask operations
        MOR,
        MXOR,
        MNOT,
        FSET,
        MOVAMX,
        MOVAXM,
        FADDPI, // Packed Integer extension
        FSUBPI,
        FMULPI,
        FMULHPI,
        FMULHUPI,
        //FMULHSUPI,
        FDIVPI,
        FDIVUPI,
        FREMPI,
        FREMUPI,
        FMINPI,
        FMAXPI,
        FMINUPI,
        FMAXUPI,
        FANDPI,
        FORPI,
        FXORPI,
        FNOTPI,
        FSLLPI,
        FSRLPI,
        FSRAPI,
        FLTPI,
        FLTUPI,
        FLEPI,
        FEQPI,
        //FADDPQ,
        FADDIPI, // Packed Integer with Immediate
        FANDIPI,
        FORIPI,
        FXORIPI,
        FSLLIPI,
        FSRLIPI,
        FSRAIPI,
        //FSLLOIPI,
        FPACKREPBPI,
        FPACKREPHPI,
        FCVTWS, // integer opcodes that should not really be here :-)
        FCVTWUS,
        FCVTSW,
        FCVTSWU,
        FCVTDW,
        FCVTDWU,
        FCVTDL,
        FCVTDLU,
        FCVTWD,
        FCVTWUD,
        FCVTLD,
        FCVTLUD,
        SIMPLE_INT, // Integer ISA
        MUL_INT,
        DIV_INT,
        REM_INT,
        MASKOP,     // Mask ops
        LD,
        STORE_INT,

        // Please keep me last at all times!
        MAXOPCODE
} opcode;


extern uint32 current_thread;

extern "C" void init_emu(int debug, int fakesam);
extern "C" void minit(mreg dst, uint64 val);
extern "C" void init(xreg dst, uint64 val);
extern "C" uint64 xget(uint64 src1);
extern "C" uint64 csrget(csr src1);
extern "C" void fpinit(freg dst, uint64 val[2]);
extern "C" void initcsr(uint32 thread);
void init_stack();
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

extern "C" void lb(xreg dst, int off, xreg base, const char *comm);
extern "C" void lh(xreg dst, int off, xreg base, const char *comm);
extern "C" void lw(xreg dst, int off, xreg base, const char *comm);
extern "C" void ld(xreg dst, int off, xreg base, const char *comm);
extern "C" void lbu(xreg dst, int off, xreg base, const char *comm);
extern "C" void lhu(xreg dst, int off, xreg base, const char *comm);
extern "C" void lwu(xreg dst, int off, xreg base, const char *comm);
extern "C" void addi(xreg dst, xreg src1, int imm, const char *comm);
extern "C" void addiw(xreg dst, xreg src1, int imm, const char *comm);
extern "C" void slt(xreg dst, xreg src1, xreg src2, const char *comm);
extern "C" void sltu(xreg dst, xreg src1, xreg src2, const char *comm);
extern "C" void slti(xreg dst, xreg src1, int imm, const char *comm);
extern "C" void sltiu(xreg dst, xreg src1, int imm, const char *comm);
extern "C" void mul(xreg dst, xreg src1, xreg src2, const char *comm);
extern "C" void mulw(xreg dst, xreg src1, xreg src2, const char *comm);
extern "C" void mulh(xreg dst, xreg src1, xreg src2, const char *comm);
extern "C" void mulhu(xreg dst, xreg src1, xreg src2, const char *comm);
extern "C" void div_(xreg dst, xreg src1, xreg src2, const char *comm);
extern "C" void divu(xreg dst, xreg src1, xreg src2, const char *comm);
extern "C" void divw(xreg dst, xreg src1, xreg src2, const char *comm);
extern "C" void divuw(xreg dst, xreg src1, xreg src2, const char *comm);
extern "C" void rem(xreg dst, xreg src1, xreg src2, const char *comm);
extern "C" void remu(xreg dst, xreg src1, xreg src2, const char *comm);
extern "C" void remw(xreg dst, xreg src1, xreg src2, const char *comm);
extern "C" void remuw(xreg dst, xreg src1, xreg src2, const char *comm);
extern "C" void add(xreg dst, xreg src1, xreg src2, const char *comm);
extern "C" void addw(xreg dst, xreg src1, xreg src2, const char *comm);
extern "C" void sub(xreg dst, xreg src1, xreg src2, const char *comm);
extern "C" void subw(xreg dst, xreg src1, xreg src2, const char *comm);
extern "C" void ori(xreg dst, xreg src1, int imm, const char *comm);
extern "C" void or_(xreg dst, xreg src1, xreg src2, const char *comm);
extern "C" void andi(xreg dst, xreg src1, int imm, const char *comm);
extern "C" void and_(xreg dst, xreg src1, xreg src2, const char *comm);
extern "C" void xori(xreg dst, xreg src1, int imm, const char *comm);
extern "C" void xor_(xreg dst, xreg src1, xreg src2, const char *comm);
extern "C" void lui(xreg dst, int imm, const char *comm);
extern "C" void auipc(xreg dst, int imm, const char *comm);
extern "C" void mv(xreg dst, xreg src1, const char *comm);
extern "C" void sllw(xreg dst, xreg src1, xreg src2, const char *comm);
extern "C" void sll(xreg dst, xreg src1, xreg src2, const char *comm);
extern "C" void slliw(xreg dst, xreg src1, int imm, const char *comm);
extern "C" void slli(xreg dst, xreg src1, int imm, const char *comm);
extern "C" void srlw(xreg dst, xreg src1, xreg src2, const char *comm);
extern "C" void srl(xreg dst, xreg src1, xreg src2, const char *comm);
extern "C" void srliw(xreg dst, xreg src1, int imm, const char *comm);
extern "C" void srli(xreg dst, xreg src1, int imm, const char *comm);
extern "C" void sra(xreg dst, xreg src1, xreg src2, const char *comm);
extern "C" void sraw(xreg dst, xreg src1, xreg src2, const char *comm);
extern "C" void srai(xreg dst, xreg src1, int imm, const char *comm);
extern "C" void sraiw(xreg dst, xreg src1, int imm, const char *comm);
extern "C" void jal(xreg dst, int imm, const char *comm);
extern "C" void jalr(xreg dst, xreg src1, int imm, const char *comm);
extern "C" void c_jalr(xreg dst, xreg src1, int imm, const char *comm);
extern "C" void beq(xreg src1, xreg src2, int imm, const char *comm);
extern "C" void bne(xreg src1, xreg src2, int imm, const char *comm);
extern "C" void blt(xreg src1, xreg src2, int imm, const char *comm);
extern "C" void bltu(xreg src1, xreg src2, int imm, const char *comm);
extern "C" void bge(xreg src1, xreg src2, int imm, const char *comm);
extern "C" void bgeu(xreg src1, xreg src2, int imm, const char *comm);
extern "C" void li(xreg dst, uint64 imm, const char *comm);
extern "C" void sd(xreg src1, int off, xreg base, const char *comm);
extern "C" void sw(xreg src1, int off, xreg base, const char *comm);
extern "C" void sh(xreg src1, int off, xreg base, const char *comm);
extern "C" void sb(xreg src1, int off, xreg base, const char *comm);
extern "C" void amoswap_w(xreg dst, xreg src1, xreg src2, const char *comm);
extern "C" void amoadd_w(xreg dst, xreg src1, xreg src2, const char *comm);
extern "C" void amoxor_w(xreg dst, xreg src1, xreg src2, const char *comm);
extern "C" void amoand_w(xreg dst, xreg src1, xreg src2, const char *comm);
extern "C" void amoor_w(xreg dst, xreg src1, xreg src2, const char *comm);
extern "C" void amomin_w(xreg dst, xreg src1, xreg src2, const char *comm);
extern "C" void amomax_w(xreg dst, xreg src1, xreg src2, const char *comm);
extern "C" void amominu_w(xreg dst, xreg src1, xreg src2, const char *comm);
extern "C" void amomaxu_w(xreg dst, xreg src1, xreg src2, const char *comm);
extern "C" void amoswap_d(xreg dst, xreg src1, xreg src2, const char *comm);
extern "C" void amoadd_d(xreg dst, xreg src1, xreg src2, const char *comm);
extern "C" void amoxor_d(xreg dst, xreg src1, xreg src2, const char *comm);
extern "C" void amoand_d(xreg dst, xreg src1, xreg src2, const char *comm);
extern "C" void amoor_d(xreg dst, xreg src1, xreg src2, const char *comm);
extern "C" void amomin_d(xreg dst, xreg src1, xreg src2, const char *comm);
extern "C" void amomax_d(xreg dst, xreg src1, xreg src2, const char *comm);
extern "C" void amominu_d(xreg dst, xreg src1, xreg src2, const char *comm);
extern "C" void amomaxu_d(xreg dst, xreg src1, xreg src2, const char *comm);
extern "C" void csrr(xreg dst, csr src1, const char *comm);
extern "C" void csrw(csr dst, xreg src1, const char *comm);
extern "C" void csrwi(csr dst, uint64 imm, const char *comm);
extern "C" void csrs(csr dst, xreg src1, const char *comm);
extern "C" void csrrw(xreg dst, csr src1, xreg src2, const char *comm);
extern "C" void mret(const char *comm);
extern "C" void wfi(const char *comm);
extern "C" void fence(const char *comm);
extern "C" void fence_i(const char *comm);
extern "C" void ecall(const char *comm);
extern "C" void flw   (freg dst, int off, xreg base, const char *comm);
extern "C" void fld   (freg dst, int off, xreg base, const char *comm);
extern "C" void flq   (freg dst, int off, xreg base, const char *comm);
extern "C" void flw_ps(freg dst, int off, xreg base, const char *comm);
extern "C" void fbc_ps(freg dst, int off, xreg base, const char *comm);
extern "C" void fbci_pi(freg dst, uint32 imm, const char *comm);
extern "C" void fbci_ps(freg dst, uint32 imm, const char *comm);
extern "C" void fbcx_ps(freg dst, xreg src, const char *comm);
//extern "C" void fbcx_pq(freg dst, xreg src, const char *comm);
extern "C" void fsw   (freg src1, int off, xreg base, const char *comm);
extern "C" void fsw_ps(freg src1, int off, xreg base, const char *comm);
extern "C" void fsd   (freg src1, int off, xreg base, const char *comm);
extern "C" void fsq   (freg src1, int off, xreg base, const char *comm);
extern "C" void fadd_s (freg dst, freg src1, freg src2, const char *comm);
extern "C" void fadd_ps (freg dst, freg src1, freg src2, const char *comm);
extern "C" void fsub_s (freg dst, freg src1, freg src2, const char *comm);
extern "C" void fsub_ps(freg dst, freg src1, freg src2, const char *comm);
extern "C" void fmul_s (freg dst, freg src1, freg src2, const char *comm);
extern "C" void fmul_ps(freg dst, freg src1, freg src2, const char *comm);
extern "C" void fdiv_s (freg dst, freg src1, freg src2, const char *comm);
extern "C" void fdiv_ps(freg dst, freg src1, freg src2, const char *comm);
extern "C" void fneg_s (freg dst, freg src1, const char *comm);
extern "C" void fneg_ps(freg dst, freg src1, const char *comm);
extern "C" void fmov_s_x(freg dst, xreg src1, const char *comm);
extern "C" void fmov_x_s(xreg dst, freg src1, const char *comm);
extern "C" void fcvt_x_s(xreg dst, freg src1, const char *comm);
extern "C" void fgw_ps(freg dst, freg src1, xreg base, const char *comm);
extern "C" void fgh_ps(freg dst, freg src1, xreg base, const char *comm);
extern "C" void fgb_ps(freg dst, freg src1, xreg base, const char *comm);
extern "C" void fscw_ps(freg src1, freg src2, xreg base, const char *comm);
extern "C" void fsch_ps(freg src1, freg src2, xreg base, const char *comm);
extern "C" void fscb_ps(freg src1, freg src2, xreg base, const char *comm);
extern "C" void fg32w_ps(freg dst, xreg src1, xreg src2, const char *comm);
extern "C" void fg32h_ps(freg dst, xreg src1, xreg src2, const char *comm);
extern "C" void fg32b_ps(freg dst, xreg src1, xreg src2, const char *comm);
extern "C" void fsc32w_ps(freg src3, xreg src1, xreg src2, const char *comm);
extern "C" void fsc32h_ps(freg src3, xreg src1, xreg src2, const char *comm);
extern "C" void fsc32b_ps(freg src3, xreg src1, xreg src2, const char *comm);
extern "C" void fmadd_s  (freg dst, freg src1, freg src2, freg src3, const char *comm);
extern "C" void fmadd_ps (freg dst, freg src1, freg src2, freg src3, const char *comm);
extern "C" void fmsub_s  (freg dst, freg src1, freg src2, freg src3, const char *comm);
extern "C" void fmsub_ps (freg dst, freg src1, freg src2, freg src3, const char *comm);
extern "C" void fnmadd_s (freg dst, freg src1, freg src2, freg src3, const char *comm);
extern "C" void fnmadd_ps(freg dst, freg src1, freg src2, freg src3, const char *comm);
extern "C" void fnmsub_s (freg dst, freg src1, freg src2, freg src3, const char *comm);
extern "C" void fnmsub_ps(freg dst, freg src1, freg src2, freg src3, const char *comm);
extern "C" void fcmov_ps (freg dst, freg src1, freg src2, freg src3, const char *comm);
extern "C" void fmin_s    (freg dst, freg src1, freg src2, const char *comm);
extern "C" void fmin_ps   (freg dst, freg src1, freg src2, const char *comm);
extern "C" void fmax_s    (freg dst, freg src1, freg src2, const char *comm);
extern "C" void fmax_ps   (freg dst, freg src1, freg src2, const char *comm);
extern "C" void fsgnj_s   (freg dst, freg src1, freg src2, const char *comm);
extern "C" void fsgnj_ps  (freg dst, freg src1, freg src2, const char *comm);
extern "C" void fsgnjn_s  (freg dst, freg src1, freg src2, const char *comm);
extern "C" void fsgnjn_ps (freg dst, freg src1, freg src2, const char *comm);
extern "C" void fsgnjx_s  (freg dst, freg src1, freg src2, const char *comm);
extern "C" void fsgnjx_ps (freg dst, freg src1, freg src2, const char *comm);
extern "C" void flt_ps    (freg dst, freg src1, freg src2, const char *comm);
//extern "C" void fltabs_ps (freg dst, freg src1, freg src2, const char *comm);
extern "C" void fle_ps    (freg dst, freg src1, freg src2, const char *comm);
extern "C" void feq_ps    (freg dst, freg src1, freg src2, const char *comm);
extern "C" void feqm_ps   (mreg dst, freg src1, freg src2, const char *comm);
extern "C" void fltm_ps   (mreg dst, freg src1, freg src2, const char *comm);
extern "C" void flem_ps   (mreg dst, freg src1, freg src2, const char *comm);
extern "C" void fsetm_ps  (mreg dst, freg src1,            const char *comm);
extern "C" void fclass_ps (freg dst, freg src1,            const char *comm);
extern "C" void fsqrt_s  (freg dst, freg src1, const char *comm);
extern "C" void fsqrt_ps (freg dst, freg src1, const char *comm);
extern "C" void frsq_ps  (freg dst, freg src1, const char *comm);
extern "C" void fsin_ps  (freg dst, freg src1, const char *comm);
//extern "C" void fcos_ps  (freg dst, freg src1, const char *comm);
extern "C" void fexp_ps  (freg dst, freg src1, const char *comm);
extern "C" void flog_ps  (freg dst, freg src1, const char *comm);
extern "C" void frcp_ps  (freg dst, freg src1, const char *comm);
extern "C" void frcpfxp_ps  (freg dst, freg src1, const char *comm);
extern "C" void fcvt_pw_ps  (freg dst, freg src1, const char *comm);
extern "C" void fcvt_pwu_ps (freg dst, freg src1, const char *comm);
extern "C" void fcvt_d_s  (freg dst, freg src1, const char *comm);
extern "C" void fcvt_s_d  (freg dst, freg src1, const char *comm);
extern "C" void fcvt_s_w  (freg dst, freg src1, const char *comm);
extern "C" void fcvt_s_wu (freg dst, freg src1, const char *comm);
extern "C" void fcvt_w_s  (freg dst, freg src1, rounding_mode rm, const char *comm);
void fcvt_w_s  (freg dst, freg src1, const char *comm);
extern "C" void fcvt_wu_s (freg dst, freg src1, const char *comm);
extern "C" void fcvt_d_w  (freg dst, freg src1, const char *comm);
extern "C" void fcvt_d_wu (freg dst, freg src1, const char *comm);
extern "C" void fcvt_d_l  (freg dst, freg src1, const char *comm);
extern "C" void fcvt_d_lu (freg dst, freg src1, const char *comm);
extern "C" void fcvt_w_d  (freg dst, freg src1, const char *comm);
extern "C" void fcvt_wu_d (freg dst, freg src1, const char *comm);
extern "C" void fcvt_l_d  (freg dst, freg src1, const char *comm);
extern "C" void fcvt_lu_d (freg dst, freg src1, const char *comm);
extern "C" void fcvt_ps_pw  (freg dst, freg src1, const char *comm);
extern "C" void fcvt_ps_pwu (freg dst, freg src1, const char *comm);
extern "C" void ffrc_ps (freg dst, freg src1, const char *comm);
extern "C" void fround_ps (freg dst, freg src1, rounding_mode rm, const char *comm);
extern "C" void fnot_pi (freg dst, freg src1, const char *comm);
extern "C" void fadd_pi (freg dst, freg src1, freg src2, const char *comm);
extern "C" void fsub_pi (freg dst, freg src1, freg src2, const char *comm);
extern "C" void fmul_pi (freg dst, freg src1, freg src2, const char *comm);
extern "C" void fmulh_pi (freg dst, freg src1, freg src2, const char *comm);
extern "C" void fmulhu_pi (freg dst, freg src1, freg src2, const char *comm);
//extern "C" void fmulhsu_pi (freg dst, freg src1, freg src2, const char *comm);
extern "C" void fdiv_pi (freg dst, freg src1, freg src2, const char *comm);
extern "C" void fdivu_pi (freg dst, freg src1, freg src2, const char *comm);
extern "C" void frem_pi (freg st, freg src1, freg src2, const char *comm);
extern "C" void fremu_pi (freg st, freg src1, freg src2, const char *comm);
extern "C" void fmax_pi (freg dst, freg src1, freg src2, const char *comm);
extern "C" void fmin_pi (freg dst, freg src1, freg src2, const char *comm);
extern "C" void fmaxu_pi (freg dst, freg src1, freg src2, const char *comm);
extern "C" void fminu_pi (freg dst, freg src1, freg src2, const char *comm);
extern "C" void fand_pi (freg dst, freg src1, freg src2, const char *comm);
extern "C" void for_pi  (freg dst, freg src1, freg src2, const char *comm);
extern "C" void fxor_pi (freg dst, freg src1, freg src2, const char *comm);
extern "C" void fsll_pi (freg dst, freg src1, freg src2, const char *comm);
extern "C" void fsrl_pi (freg dst, freg src1, freg src2, const char *comm);
extern "C" void fsra_pi (freg dst, freg src1, freg src2, const char *comm);
extern "C" void flt_pi   (freg dst, freg src1, freg src2, const char *comm);
extern "C" void fltu_pi  (freg dst, freg src1, freg src2, const char *comm);
extern "C" void fle_pi   (freg dst, freg src1, freg src2, const char *comm);
extern "C" void feq_pi   (freg dst, freg src1, freg src2, const char *comm);
extern "C" void fltm_pi  (mreg dst, freg src1, freg src2, const char *comm);
extern "C" void faddi_pi (freg dst, freg src1, uint32 imm, const char *comm);
extern "C" void fandi_pi (freg dst, freg src1, uint32 imm, const char *comm);
extern "C" void fori_pi  (freg dst, freg src1, uint32 imm, const char *comm);
extern "C" void fxori_pi (freg dst, freg src1, uint32 imm, const char *comm);
extern "C" void fslli_pi (freg dst, freg src1, uint32 imm, const char *comm);
extern "C" void fsrli_pi (freg dst, freg src1, uint32 imm, const char *comm);
extern "C" void fsrai_pi (freg dst, freg src1, uint32 imm, const char *comm);
//extern "C" void fslloi_pi (freg dst, freg src1, freg src2, uint32 imm, const char *comm);
extern "C" void fpackreph_pi (freg dst, freg src1, const char *comm);
extern "C" void fpackrepb_pi (freg dst, freg src1, const char *comm);

extern "C" void fbc_ph   (freg dst, int off, xreg base, const char *comm);
extern "C" void fmin_ph  (freg dst, freg src1, freg src2, const char *comm);
extern "C" void fmax_ph  (freg dst, freg src1, freg src2, const char *comm);
extern "C" void fmadd_ph (freg dst, freg src1, freg src2, freg src3, const char *comm);
extern "C" void fmsub_ph (freg dst, freg src1, freg src2, freg src3, const char *comm);
extern "C" void fnmadd_ph(freg dst, freg src1, freg src2, freg src3, const char *comm);
extern "C" void fnmsub_ph(freg dst, freg src1, freg src2, freg src3, const char *comm);

extern "C" void feq_s        (xreg dst, freg src1, freg src2, const char *comm);
extern "C" void fle_s        (xreg dst, freg src1, freg src2, const char *comm);
extern "C" void flt_s        (xreg dst, freg src1, freg src2, const char *comm);
extern "C" void fcvt_ps_f16  (freg dst, freg src1, const char *comm);
extern "C" void fcvt_ps_un24 (freg dst, freg src1, const char *comm);
extern "C" void fcvt_ps_un16 (freg dst, freg src1, const char *comm);
extern "C" void fcvt_ps_un10 (freg dst, freg src1, const char *comm);
extern "C" void fcvt_ps_un8  (freg dst, freg src1, const char *comm);
extern "C" void fcvt_ps_un2  (freg dst, freg src1, const char *comm);
//extern "C" void fcvt_ps_sn24 (freg dst, freg src1, const char *comm);
extern "C" void fcvt_ps_sn16 (freg dst, freg src1, const char *comm);
//extern "C" void fcvt_ps_sn10 (freg dst, freg src1, const char *comm);
extern "C" void fcvt_ps_sn8  (freg dst, freg src1, const char *comm);
//extern "C" void fcvt_ps_sn2  (freg dst, freg src1, const char *comm);
extern "C" void fcvt_ps_f11  (freg dst, freg src1, const char *comm);
extern "C" void fcvt_ps_f10  (freg dst, freg src1, const char *comm);

extern "C" void fcvt_f16_ps  (freg dst, freg src1, rounding_mode rm, const char *comm);
extern "C" void fcvt_un24_ps (freg dst, freg src1, rounding_mode rm, const char *comm);
extern "C" void fcvt_un16_ps (freg dst, freg src1, rounding_mode rm, const char *comm);
extern "C" void fcvt_un10_ps (freg dst, freg src1, rounding_mode rm, const char *comm);
extern "C" void fcvt_un8_ps  (freg dst, freg src1, rounding_mode rm, const char *comm);
extern "C" void fcvt_un2_ps  (freg dst, freg src1, rounding_mode rm, const char *comm);
//extern "C" void fcvt_sn24_ps (freg dst, freg src1, rounding_mode rm, const char *comm);
extern "C" void fcvt_sn16_ps (freg dst, freg src1, rounding_mode rm, const char *comm);
extern "C" void fcvt_sn8_ps  (freg dst, freg src1, rounding_mode rm, const char *comm);
extern "C" void fcvt_f11_ps  (freg dst, freg src1, rounding_mode rm, const char *comm);
extern "C" void fcvt_f10_ps  (freg dst, freg src1, rounding_mode rm, const char *comm);

extern "C" void fswizz_ps    (freg dst, freg src1, uint8  imm, const char *comm);

//extern "C" void fadd_pq        (freg dst, freg src1, freg src2, const char *comm);

extern "C" void cubeface_ps    (freg dst, freg src1, freg src2, const char *comm);
extern "C" void cubefaceidx_ps (freg dst, freg src1, freg src2, const char *comm);
extern "C" void cubesgnsc_ps   (freg dst, freg src1, freg src2, const char *comm);
extern "C" void cubesgntc_ps   (freg dst, freg src1, freg src2, const char *comm);

extern "C" void fmvz_x_ps     (xreg dst, freg src1, uint8 index, const char *comm);
extern "C" void fmvs_x_ps     (xreg dst, freg src1, uint8 index, const char *comm);
extern "C" void fmv_x_pq      (xreg dst, freg src1, uint8 index, const char *comm);
extern "C" void fcmovm_ps     (freg dst, freg src1, freg src2, const char *comm);

extern "C" void fmv_x_s      (xreg dst, freg src1, const char *comm);
extern "C" void fmv_s_x      (freg dst, xreg src1, const char *comm);
extern "C" void fmv_x_d      (xreg dst, freg src1, const char *comm);

//to be removed
//extern "C" void fmv_x_ps     (xreg dst, freg src1, uint8 index, const char *comm);
//extern "C" void fmv_ps_x     (freg dst, xreg src1, uint8 bdcast, const char *comm);

// Double precision instructions
extern "C" void fmadd_d  (freg dst, freg src1, freg src2, freg src3, const char *comm);
extern "C" void feq_d    (freg dst, freg src1, freg src2, const char *comm);
extern "C" void fmin_d   (freg dst, freg src1, freg src2, const char *comm);
extern "C" void fmax_d   (freg dst, freg src1, freg src2, const char *comm);
extern "C" void fadd_d   (freg dst, freg src1, freg src2, const char *comm);
extern "C" void fdiv_d   (freg dst, freg src1, freg src2, const char *comm);
extern "C" void fmul_d   (freg dst, freg src1, freg src2, const char *comm);
extern "C" void fsgnj_d  (freg dst, freg src1, freg src2, const char *comm);
extern "C" void fmv_d_x  (xreg dst, freg src1, const char *comm);
extern "C" void fcvt_d_w (freg dst, freg src1, const char *comm);

// Mask instructions
extern "C" void maskand    (mreg dst, mreg src1, mreg src2, const char *comm);
extern "C" void maskor     (mreg dst, mreg src1, mreg src2, const char *comm);
extern "C" void maskxor    (mreg dst, mreg src1, mreg src2, const char *comm);
extern "C" void masknot    (mreg dst, mreg src1, const char *comm);
extern "C" void mova_x_m   (xreg dst, const char *comm);
extern "C" void mova_m_x   (xreg src1, const char *comm);
extern "C" void mov_m_x    (mreg dst, xreg src1, uint32 imm, const char *comm);
extern "C" void movi_m     (mreg dst, uint8 imm, const char *comm);
extern "C" void maskpopc   (xreg dst, mreg src1, const char *comm);
extern "C" void maskpopcz  (xreg dst, mreg src1, const char *comm);
extern "C" void maskpopc_rast (xreg dst, mreg src1, mreg src2, uint32 imm, const char *comm);

// Texture instructions
extern "C" void texsndh   (xreg src1, xreg src2, const char *comm);
extern "C" void texsnds   (freg src1, const char *comm);
extern "C" void texsndt   (freg src1, const char *comm);
extern "C" void texsndr   (freg src1, const char *comm);
extern "C" void texrcv    (freg dst, const uint32 imm, const char *comm);

extern "C" void init_txs(uint64 imgTableAddr);

// Special scalar instructions
extern "C" void packb(xreg dst, xreg src1, xreg src2, const char *comm);
extern "C" void bitmixb(xreg dst, xreg src1, xreg src2, const char *comm);

void print_regs();
void print_comment(const char *comm);
bool bset (mreg src1, const char *comm);

#endif
