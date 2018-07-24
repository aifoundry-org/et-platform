#ifndef _EMU_DEFINES_H
#define _EMU_DEFINES_H

#if defined(__cplusplus) && (__cplusplus >= 201103L)
#include <cinttypes>
#else
#include <inttypes.h>
#endif

// Basic types
typedef double             float64_t;
typedef float              float32_t;

// CSRs
typedef enum
{
    csr_prv = 0, // this is internal to HW

    // ----- U-mode registers ------------------------------------------------
    // csr_ustatus,
    // csr_uie,
    // csr_utvec,
    // csr_uscratch,
    // csr_uepc,
    // csr_ucause,
    // csr_utval,
    // csr_uip,
    csr_fflags,
    csr_frm,
    csr_fcsr,
    // csr_cycle,
    // csr_time,
    // csr_instret,
    // csr_hpmcounter3,
    // csr_hpmcounter4,
    // csr_hpmcounter5,
    // csr_hpmcounter6,

    // ----- U-mode ET registers ---------------------------------------------
    csr_treduce,
    csr_tfmastart,
    csr_tconvsize,
    csr_tconvctrl,
    csr_tcoop,
    csr_tmask,
    // csr_top,
    csr_flbarrier,
    csr_ucacheop,
    csr_tloadctrl,
    csr_tstore,
    csr_umsg_port0,
    csr_umsg_port1,
    csr_umsg_port2,
    csr_umsg_port3,

    // ----- S-mode registers ------------------------------------------------
    csr_sstatus,
    // csr_sedeleg,
    // csr_sideleg,
    csr_sie,
    csr_stvec,
    csr_scounteren,
    csr_sscratch,
    csr_sepc,
    csr_scause,
    csr_stval,
    csr_sip,
    csr_satp,

    // ----- S-mode ET registers ---------------------------------------------
    csr_scacheop,
    csr_smsg_port0,
    csr_smsg_port1,
    csr_smsg_port2,
    csr_smsg_port3,

    // ----- M-mode registers ------------------------------------------------
    csr_mvendorid,
    csr_marchid,
    csr_mimpid,
    csr_mhartid,
    csr_mstatus,
    csr_misa,
    csr_medeleg,
    csr_mideleg,
    csr_mie,
    csr_mtvec,
    csr_mcounteren,
    csr_mscratch,
    csr_mepc,
    csr_mcause,
    csr_mtval,
    csr_mip,
    // csr_pmcfg0,
    // csr_pmcfg2,
    // csr_pmpaddr0,
    // csr_pmpaddr1,
    // ...
    // csr_pmpaddr15,
    // csr_mcycle,
    // csr_minstret,
    // csr_mhpmcounter3,
    // csr_mhpmcounter4,
    // csr_mhpmcounter5,
    // csr_mhpmcounter6,
    // csr_mhpmevent3,
    // csr_mhpmevent4,
    // csr_mhpmevent5,
    // csr_mhpmevent6,
    // --- debug registers ---
    // csr_tselect,
    // csr_tdata1,
    // csr_tdata2,
    // csr_tdata3,
    // csr_dcsr,
    // csr_dpc,
    // csr_dscratch,

    // ----- M-mode ET registers ---------------------------------------------
    csr_icache_ctrl,
    csr_write_ctrl,

    // ----- Validation only registers ---------------------------------------
    validation0,
    validation1,
    validation2,
    validation3,

    CSR_MAX
} csr;

#define CSR_MAX_UMODE   csr_sstatus
#define CSR_MAX_SMODE   csr_mvendorid
#define CSR_MAX_MMODE   CSR_MAX

// MMU

// Memory access type
typedef enum
{
    Mem_Access_Load,
    Mem_Access_Store,
    Mem_Access_Fetch
} mem_access_type;

#define PA_SIZE        40
#define PA_M           (((uint64_t)1 << PA_SIZE) - 1)
#define PG_OFFSET_SIZE 12
#define PG_OFFSET_M    (((uint64_t)1 << PG_OFFSET_SIZE) - 1)
#define PPN_SIZE       (PA_SIZE - PG_OFFSET_SIZE)
#define PPN_M          (((uint64_t)1 << PPN_SIZE) - 1)
#define PTE_V_OFFSET   0
#define PTE_R_OFFSET   1
#define PTE_W_OFFSET   2
#define PTE_X_OFFSET   3
#define PTE_U_OFFSET   4
#define PTE_G_OFFSET   5
#define PTE_A_OFFSET   6
#define PTE_D_OFFSET   7
#define PTE_PPN_OFFSET 10

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

typedef enum
{
    MSG_ENABLE = 7,
    MSG_DISABLE = 3,
    MSG_PGET = 0,
    MSG_PGETNB = 1,
} msg_port_conf_action;

typedef enum
{
    // PS memory instructions
    FLW,
    FSW,
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
    FCLASS,
    FCLASSPS,
    FCVTPSF16, // Graphics Upconvert to PS
    FCVTPSF11,
    FCVTPSF10,
    FCVTPSUN24,
    FCVTPSUN16,
    FCVTPSUN10,
    FCVTPSUN8,
    FCVTPSUN2,
    FCVTPSRAST,
    FCVTRASTPS,
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
    FSAT8PI,
    FSLLPI,
    FSRLPI,
    FSRAPI,
    FLTPI,
    FLTUPI,
    FLEPI,
    FEQPI,
    FRCP_FIX_RAST,
    FADDIPI, // Packed Integer with Immediate
    FANDIPI,
    FORIPI,
    FXORIPI,
    FSLLIPI,
    FSRLIPI,
    FSRAIPI,
    FPACKREPBPI,
    FPACKREPHPI,
    FCVTWS, // integer opcodes that should not really be here :-)
    FCVTWUS,
    FCVTSW,
    FCVTSWU,
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

// Number of 32-bit lanes in a vector register
#ifndef VL
#define VL  4
#endif

#if (VL != 4) && (VL != 8)
#error "Only 128-bit and 256-bit vectors supported"
#endif

typedef union
{
    uint8_t   b[VL*4];
    uint16_t  h[VL*2];
    uint32_t  u[VL];
    int32_t   i[VL];
    float32_t f[VL];
    uint64_t  x[VL/2];
    int64_t   q[VL/2];
} fdata;

typedef union
{
    uint8_t   b[8];
    uint16_t  h[4];
    uint32_t  w[2];
    int32_t   ws[2];
    uint64_t  x;
    int64_t   xs;
} xdata;

typedef union
{
    uint8_t   b[VL];
} mdata;

typedef union
{
    int32_t   i;
    uint32_t  u;
    uint64_t  x;
    int64_t   xs;
    float32_t f;
} iufval;

typedef struct
{
    bool enabled;
    bool stall;
    bool umode;
    bool use_scp;
    bool enable_oop;
    uint8_t logsize;
    uint8_t max_msgs;
    uint8_t scp_set;
    uint8_t scp_way;
    uint8_t rd_ptr;
    uint8_t wr_ptr;
} msg_port_conf;

#if VL == 4
#define MASK2BYTE(_MR) (_MR.b[3]<<3|_MR.b[2]<<2|_MR.b[1]<<1|_MR.b[0])
#else
#define MASK2BYTE(_MR) (_MR.b[7]<<7|_MR.b[6]<<6|_MR.b[5]<<5|_MR.b[4]<<4|_MR.b[3]<<3|_MR.b[2]<<2|_MR.b[1]<<1|_MR.b[0])
#endif

// set to 1 if floating point 32 operation sets bits 127:32 to 0,
// and 64 bits operations set bits 127:64 to 0
#define ZERO_EXTEND_UNUSED_FREG_BITS

// Privilege levels
#define CSR_PRV_U  0
#define CSR_PRV_S  1
#define CSR_PRV_H  2
#define CSR_PRV_M  3

// Traps
#define CSR_MCAUSE_INSTR_ADDR_MISALIGNED         0ull
#define CSR_MCAUSE_INSTR_ACCESS_FAULT            1ull
#define CSR_MCAUSE_ILLEGAL_INSTRUCTION           2ull
#define CSR_MCAUSE_BREAKPOINT                    3ull
#define CSR_MCAUSE_LOAD_ADDR_MISALIGNED          4ull
#define CSR_MCAUSE_LOAD_ACCESS_FAULT             5ull
#define CSR_MCAUSE_STORE_AMO_ADDR_MISALIGNED     6ull
#define CSR_MCAUSE_STORE_AMO_ACCESS_FAULT        7ull
#define CSR_MCAUSE_ECALL_FROM_UMODE              8ull
#define CSR_MCAUSE_ECALL_FROM_SMODE              9ull
#define CSR_MCAUSE_ECALL_FROM_MMODE             11ull
#define CSR_MCAUSE_INSTR_PAGE_FAULT             12ull
#define CSR_MCAUSE_LOAD_PAGE_FAULT              13ull
#define CSR_MCAUSE_STORE_AMO_PAGE_FAULT         15ull

// Maximum number of threads
#define EMU_NUM_MINIONS         4096
#define EMU_THREADS_PER_MINION  2
#define EMU_NUM_THREADS         (EMU_NUM_MINIONS*EMU_THREADS_PER_MINION)
#define NR_MSG_PORTS            4

// Enable some features
#define DEBUG_EMU   1
#define DEBUG_MASK  1
#define DISASM      1

#endif // _EMU_DEFINES_H
