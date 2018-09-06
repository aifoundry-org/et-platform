#ifndef _EMU_DEFINES_H
#define _EMU_DEFINES_H

#if defined(__cplusplus) && (__cplusplus >= 201103L)
#include <cinttypes>
#else
#include <inttypes.h>
#endif

// Basic types
#ifdef HAVE_SOFTFLOAT
#include <softfloat/softfloat_types.h>
#else
typedef float float32_t;
#endif

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
    csr_cycle,
    csr_cycleh,
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
    csr_offtxfma,
    csr_tmask,
    csr_twait,
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
    csr_mcycle,
    csr_mcycleh,
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
    csr_validation0,
    csr_validation1,
    csr_validation2,
    csr_validation3,

    // ----- Used for illegal CSR accesses -----------------------------------
    csr_unknown,

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

// VA to PA translation
#define PA_SIZE        40
#define PA_M           ((((uint64_t)1) << PA_SIZE) - 1)
#define PG_OFFSET_SIZE 12
#define PG_OFFSET_M    ((((uint64_t)1) << PG_OFFSET_SIZE) - 1)
#define PPN_SIZE       (PA_SIZE - PG_OFFSET_SIZE)
#define PPN_M          ((((uint64_t)1) << PPN_SIZE) - 1)
#define PTE_V_OFFSET   0
#define PTE_R_OFFSET   1
#define PTE_W_OFFSET   2
#define PTE_X_OFFSET   3
#define PTE_U_OFFSET   4
#define PTE_G_OFFSET   5
#define PTE_A_OFFSET   6
#define PTE_D_OFFSET   7
#define PTE_PPN_OFFSET 10

// SATP mode field values
#define SATP_MODE_BARE  0
#define SATP_MODE_SV39  8
#define SATP_MODE_SV48  9

// MSTATUS field offsets
#define MSTATUS_MXR     19
#define MSTATUS_SUM     18
#define MSTATUS_MPRV    17
#define MSTATUS_XS      15
#define MSTATUS_FS      13
#define MSTATUS_MPP     11
#define MSTATUS_SPP     8

// fclass result bitmask
#define FCLASS_NEG_INFINITY     0x0001
#define FCLASS_NEG_NORMAL       0x0002
#define FCLASS_NEG_SUBNORMAL    0x0004
#define FCLASS_NEG_ZERO         0x0008
#define FCLASS_POS_ZERO         0x0010
#define FCLASS_POS_SUBNORMAL    0x0020
#define FCLASS_POS_NORMAL       0x0040
#define FCLASS_POS_INFINITY     0x0080
#define FCLASS_SNAN             0x0100
#define FCLASS_QNAN             0x0200
// fclass combined classes
#define FCLASS_INFINITY         (FCLASS_NEG_INFINITY  | FCLASS_POS_INFINITY )
#define FCLASS_NORMAL           (FCLASS_NEG_NORMAL    | FCLASS_POS_NORMAL   )
#define FCLASS_SUBNORMAL        (FCLASS_NEG_SUBNORMAL | FCLASS_POS_SUBNORMAL)
#define FCLASS_ZERO             (FCLASS_NEG_ZERO      | FCLASS_POS_ZERO     )
#define FCLASS_NAN              (FCLASS_SNAN          | FCLASS_QNAN         )
#define FCLASS_NEGATIVE         (FCLASS_NEG_INFINITY | FCLASS_NEG_NORMAL | FCLASS_NEG_SUBNORMAL | FCLASS_NEG_ZERO)
#define FCLASS_POSITIVE         (FCLASS_POS_INFINITY | FCLASS_POS_NORMAL | FCLASS_POS_SUBNORMAL | FCLASS_POS_ZERO)

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
    FSATU8PI,
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
#define VL  8
#endif

#if (VL != 4) && (VL != 8)
#error "Only 128-bit and 256-bit vectors supported"
#endif

/* Obsolete texsnd/texrcv instuctions use the low 128b of the fregs for data transfers */
#define VL_TBOX 4

// vector register value type
typedef union
{
    uint8_t   b[VL*4];
    uint16_t  h[VL*2];
    uint32_t  u[VL];
    int32_t   i[VL];
    float32_t f[VL];
    float     flt[VL];
    uint64_t  x[VL/2];
    int64_t   q[VL/2];
} fdata;

// general purpose register value type
typedef union
{
    uint8_t   b[8];
    uint16_t  h[4];
    uint32_t  w[2];
    int32_t   ws[2];
    uint64_t  x;
    int64_t   xs;
} xdata;

// mask register value type
typedef struct
{
    uint8_t   b[VL];
#if (VL==4)
    // FIXME: for compatibility with the RTL that has 8b masks for 128b vectors
    uint8_t   _xb[VL];
#endif
} mdata;

// useful for type conversions
typedef union
{
    int32_t   i;
    uint32_t  u;
    float32_t f;
    float     flt;
    uint64_t  x;
    int64_t   xs;
#ifdef USE_REAL_TXFMA
    double    dbl;
#endif
} iufval;

// message port value type
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
#define CAUSE_MISALIGNED_FETCH      0x00
#define CAUSE_FETCH_ACCESS          0x01
#define CAUSE_ILLEGAL_INSTRUCTION   0x02
#define CAUSE_BREAKPOINT            0x03
#define CAUSE_MISALIGNED_LOAD       0x04
#define CAUSE_LOAD_ACCESS           0x05
#define CAUSE_MISALIGNED_STORE      0x06
#define CAUSE_STORE_ACCESS          0x07
#define CAUSE_USER_ECALL            0x08
#define CAUSE_SUPERVISOR_ECALL      0x09
#define CAUSE_HYPERVISOR_ECALL      0x0a
#define CAUSE_MACHINE_ECALL         0x0b
#define CAUSE_FETCH_PAGE_FAULT      0x0c
#define CAUSE_LOAD_PAGE_FAULT       0x0d
#define CAUSE_STORE_PAGE_FAULT      0x0f

// base class for all traps
class trap_t
{
public:
    trap_t(uint64_t n) : cause(n) {}
    uint64_t get_cause() const { return cause; }

    virtual bool has_tval() const = 0;
    virtual uint64_t get_tval() const = 0;
    virtual const char* what() = 0;

private:
    const uint64_t cause;
};

// define a trap type without tval
#define DECLARE_TRAP_TVAL_N(n, x) \
    class x : public trap_t { \
    public: \
        x() : trap_t(n) {} \
        virtual bool has_tval() const override { return false; } \
        virtual uint64_t get_tval() const override { return 0; } \
        virtual const char* what() override { return #x; } \
    };

// define a trap type with tval
#define DECLARE_TRAP_TVAL_Y(n, x) \
    class x : public trap_t { \
    public: \
        x(uint64_t v) : trap_t(n), tval(v) {} \
        virtual bool has_tval() const override { return true; } \
        virtual uint64_t get_tval() const override { return tval; } \
        virtual const char* what() override { return #x; } \
    private: \
        const uint64_t tval; \
    };

DECLARE_TRAP_TVAL_Y(CAUSE_MISALIGNED_FETCH,     trap_instruction_address_misaligned)
DECLARE_TRAP_TVAL_Y(CAUSE_FETCH_ACCESS,         trap_instruction_access_fault)
DECLARE_TRAP_TVAL_Y(CAUSE_ILLEGAL_INSTRUCTION,  trap_illegal_instruction)
DECLARE_TRAP_TVAL_Y(CAUSE_BREAKPOINT,           trap_breakpoint)
DECLARE_TRAP_TVAL_Y(CAUSE_MISALIGNED_LOAD,      trap_load_address_misaligned)
DECLARE_TRAP_TVAL_Y(CAUSE_MISALIGNED_STORE,     trap_store_address_misaligned)
DECLARE_TRAP_TVAL_Y(CAUSE_LOAD_ACCESS,          trap_load_access_fault)
DECLARE_TRAP_TVAL_Y(CAUSE_STORE_ACCESS,         trap_store_access_fault)
DECLARE_TRAP_TVAL_N(CAUSE_USER_ECALL,           trap_user_ecall)
DECLARE_TRAP_TVAL_N(CAUSE_SUPERVISOR_ECALL,     trap_supervisor_ecall)
DECLARE_TRAP_TVAL_N(CAUSE_HYPERVISOR_ECALL,     trap_hypervisor_ecall)
DECLARE_TRAP_TVAL_N(CAUSE_MACHINE_ECALL,        trap_machine_ecall)
DECLARE_TRAP_TVAL_Y(CAUSE_FETCH_PAGE_FAULT,     trap_instruction_page_fault)
DECLARE_TRAP_TVAL_Y(CAUSE_LOAD_PAGE_FAULT,      trap_load_page_fault)
DECLARE_TRAP_TVAL_Y(CAUSE_STORE_PAGE_FAULT,     trap_store_page_fault)

// Maximum number of threads
#define EMU_NUM_MINIONS         1024
#define EMU_THREADS_PER_MINION  2
#define EMU_NUM_THREADS         (EMU_NUM_MINIONS*EMU_THREADS_PER_MINION)
#define EMU_MINIONS_PER_SHIRE   32
#define NR_MSG_PORTS            4

// Enable some features
#define DEBUG_EMU   1
#define DEBUG_MASK  1
#define DISASM      1

// Scratchpad defines
#define L1_SCP_ENTRIES    64
#define L1_SCP_LINE_SIZE  64
#define L1_SCP_BLOCKS     (L1_SCP_LINE_SIZE / (VL * 4))
#define L1_SCP_BLOCK_SIZE (VL * 4)
// Some Tensor defines
#define TFMA_MAX_AROWS    16
#define TFMA_MAX_ACOLS    16
#define TFMA_MAX_BCOLS    16
#define TFMA_REGS_PER_ROW (64 / (VL * 4))

#endif // _EMU_DEFINES_H

