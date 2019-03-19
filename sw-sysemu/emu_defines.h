#ifndef _EMU_DEFINES_H
#define _EMU_DEFINES_H

#include <cinttypes>
#include <bitset>

// Maximum number of threads
#define EMU_NUM_SHIRES          34 // 32 Compute + Master + IO Shire SP
#define EMU_NUM_COMPUTE_SHIRES  32
#define EMU_MASTER_SHIRE        32
#define EMU_IO_SHIRE_SP         33
#define IO_SHIRE_ID             254
#define EMU_NEIGH_PER_SHIRE     4
#define EMU_MINIONS_PER_NEIGH   8
#define EMU_TBOXES_PER_SHIRE    2
#define EMU_RBOXES_PER_SHIRE    1
#define EMU_MINIONS_PER_SHIRE   (EMU_NEIGH_PER_SHIRE*EMU_MINIONS_PER_NEIGH)
#define EMU_NUM_MINIONS         (EMU_NUM_SHIRES*EMU_MINIONS_PER_SHIRE)
#define EMU_THREADS_PER_MINION  2
#define EMU_THREADS_PER_NEIGH   (EMU_THREADS_PER_MINION*EMU_MINIONS_PER_NEIGH)
#define EMU_THREADS_PER_SHIRE   (EMU_THREADS_PER_NEIGH*EMU_NEIGH_PER_SHIRE)
#define EMU_NUM_THREADS         (EMU_THREADS_PER_SHIRE*EMU_NUM_SHIRES)
#define EMU_NUM_TBOXES          (EMU_NUM_COMPUTE_SHIRES*EMU_TBOXES_PER_SHIRE)
#define EMU_NUM_RBOXES          (EMU_NUM_COMPUTE_SHIRES*EMU_RBOXES_PER_SHIRE)

#define NR_MSG_PORTS    4

// L1 Dcache configuration
#define L1D_NUM_SETS    16
#define L1D_NUM_WAYS    4
#define L1D_LINE_SIZE   64

// Some TensorFMA defines
#define TFMA_MAX_AROWS    16
#define TFMA_MAX_ACOLS    16
#define TFMA_MAX_BCOLS    16
#define TFMA_REGS_PER_ROW (TFMA_MAX_BCOLS / VL)
// FastLocalBarrier
#define FAST_LOCAL_BARRIERS 32
// TensorQuant defines
#define TQUANT_MAX_TRANS 10
// FastCreditCounters
#define EMU_NUM_FCC_COUNTERS_PER_THREAD 2

// VA to PA translation
#define PA_SIZE        40
#define PA_M           ((((uint64_t)1) << PA_SIZE) - 1)
#define VA_SIZE        48
#define VA_M           ((((uint64_t)1) << VA_SIZE) - 1)
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

// ESR region 'base address' field in bits [39:32]
#define ESR_REGION             0x0100000000ULL  // ESR Region bit [32] == 1
#define ESR_REGION_MASK        0xFF00000000ULL  // Mask to determine if address is in the ESR region (bits [39:32])
#define ESR_REGION_MASK_SHIFT  32

// ESR region 'pp' field in bits [31:30]
#define ESR_REGION_PROT_MASK   0x00C0000000ULL  // ESR Region Protection is defined in bits [31:30]
#define ESR_REGION_PROT_SHIFT  30               // Bits to shift to get the ESR Region Protection defined in bits [31:30]

// ESR region 'shireid' field in bits [29:22]
#define ESR_REGION_SHIRE_MASK  0x003FC00000ULL  // On the ESR Region the Shire is defined by bits [29:22]
#define ESR_REGION_LOCAL_SHIRE 0x003FC00000ULL  // On the ESR Region the Local Shire is defined by bits [29:22] == 8'hff
#define ESR_REGION_SHIRE_SHIFT 22               // Bits to shift to get Shire, Shire is defined by bits [19:22]
#define ESR_REGION_OFFSET      0x0000400000ULL

// ESR region 'subregion' field in bits [21:20]
#define ESR_SREGION_MASK       0x0000300000ULL  // The ESR Region is defined by bits [21:20] and further limited by bits [19:12] depending on the region
#define ESR_SREGION_SHIFT      20               // Bits to shift to get Region, Region is defined in bits [21:20]

// ESR region 'extregion' field in bits [21:17] (when 'subregion' is 2'b11)
#define ESR_SREGION_EXT_MASK   0x00003E0000ULL  // The ESR Extended Region is defined by bits [21:17]
#define ESR_SREGION_EXT_SHIFT  17               // Bits to shift to get Extended Region, Extended Region is defined in bits [21:17]

// ESR region 'hart' field in bits [19:12] (when 'subregion' is 2'b00)
#define ESR_HART_MASK          0x00000FF000ULL  // On HART ESR Region the HART is defined in bits [19:12]
#define ESR_HART_SHIFT         12               // On HART ESR Region bits to shift to get the HART defined in bits [19:12]

// ESR region 'neighborhood' field in bits [19:16] (when 'subregion' is 2'b01)
#define ESR_NEIGH_MASK         0x00000F0000ULL  // On Neighborhood ESR Region Neighborhood is defined when bits [19:16]
#define ESR_NEIGH_SHIFT        16               // On Neighborhood ESR Region bits to shift to get the Neighborhood defined in bits [19:16]
#define ESR_NEIGH_OFFSET       0x0000010000ULL  // On Neighborhood ESR Region the Neighborhood is defined by bits [19:16]
#define ESR_NEIGH_BROADCAST    15               // On Neighborhood ESR Region Neighborhood Broadcast is defined when bits [19:16] == 4'b1111

// ESR region 'bank' field in bits [16:13] (when 'subregion' is 2'b11)
#define ESR_BANK_MASK          0x000001E000ULL  // On Shire Cache ESR Region Bank is defined in bits [16:13]
#define ESR_BANK_SHIFT         13               // On Shire Cache ESR Region bits to shift to get Bank defined in bits [16:13]

// ESR region 'ESR' field in bits [xx:3] (depends on 'subregion' and 'extregion' fields)
#define ESR_HART_ESR_MASK      0x0000000FF8ULL  // On HART ESR Region the ESR is defined by bits [11:3]
#define ESR_NEIGH_ESR_MASK     0x000000FFF8ULL  // On Neighborhood ESR Region the ESR is defined in bits [15:3]
#define ESR_SC_ESR_MASK        0x0000001FF8ULL  // On Shire Cache ESR Region the ESR is defined in bits [12:3]
#define ESR_SHIRE_ESR_MASK     0x000001FFF8ULL  // On Shire ESR Region the ESR is defined in bits [16:3]
#define ESR_RBOX_ESR_MASK      0x000001FFF8ULL  // On RBOX ESR Region the ESR is defined in bits [16:3]
#define ESR_ESR_ID_SHIFT       3

// Base addresses for the various ESR subregions
#define ESR_HART_REGION        0x0100000000ULL  // Neighborhood ESR Region is at region [21:20] == 2'b00 and [11] == 1'b0
#define ESR_NEIGH_REGION       0x0100100000ULL  // Neighborhood ESR Region is at region [21:20] == 2'b01
#define ESR_CACHE_REGION       0x0100300000ULL  // Shire Cache ESR Region is at region [21:20] == 2'b11 and [19:17] == 2'b000
#define ESR_RBOX_REGION        0x0100320000ULL  // RBOX ESR Region is at region [21:20] == 2'b11 and [19:17] == 2'b001
#define ESR_SHIRE_REGION       0x0100340000ULL  // Shire ESR Region is at region [21:20] == 2'b11 and [19:17] == 2'b010

// Helper macros to construct ESR addresses in the various subregions

#define ESR_HART(prot, shire, hart, name) \
    (uint64_t(ESR_HART_REGION) + \
     (uint64_t(prot) << ESR_REGION_PROT_SHIFT) + \
     (uint64_t(shire) << ESR_REGION_SHIRE_SHIFT) + \
     (uint64_t(hart) << ESR_HART_SHIFT) + \
      uint64_t(ESR_HART_ ## name))

#define ESR_NEIGH(prot, shire, neigh, name) \
    (uint64_t(ESR_NEIGH_REGION) + \
     (uint64_t(prot) << ESR_REGION_PROT_SHIFT) + \
     (uint64_t(shire) << ESR_REGION_SHIRE_SHIFT) + \
     (uint64_t(neigh) << ESR_NEIGH_SHIFT) + \
      uint64_t(ESR_NEIGH_ ## name))

#define ESR_CACHE(prot, shire, bank, name) \
    (uint64_t(ESR_CACHE_REGION) + \
     (uint64_t(prot) << ESR_REGION_PROT_SHIFT) + \
     (uint64_t(shire) << ESR_REGION_SHIRE_SHIFT) + \
     (uint64_t(bank) << ESR_BANK_SHIFT) + \
      uint64_t(ESR_CACHE_ ## name))

#define ESR_RBOX(prot, shire, name) \
    (uint64_t(ESR_RBOX_REGION) + \
     (uint64_t(prot) << ESR_REGION_PROT_SHIFT) + \
     (uint64_t(shire) << ESR_REGION_SHIRE_SHIFT) + \
      uint64_t(ESR_RBOX_ ## name))

#define ESR_SHIRE(prot, shire, name) \
    (uint64_t(ESR_SHIRE_REGION) + \
     (uint64_t(prot) << ESR_REGION_PROT_SHIFT) + \
     (uint64_t(shire) << ESR_REGION_SHIRE_SHIFT) + \
      uint64_t(ESR_SHIRE_ ## name))

// Hart ESRs
#define ESR_HART_0                      0x000   /* PP = 0b00 */
#define ESR_HART_PORT0                  0x800   /* PP = 0b00 */
#define ESR_HART_PORT1                  0x810   /* PP = 0b00 */
#define ESR_HART_PORT2                  0x820   /* PP = 0b00 */
#define ESR_HART_PORT3                  0x830   /* PP = 0b00 */

// Neighborhood ESRs
#define ESR_NEIGH_0                     0x0000  /* PP = 0b00 */
#define ESR_NEIGH_IPI_REDIRECT_PC       0x0040  /* PP = 0b00 */
#define ESR_NEIGH_TBOX_CONTROL          0x8000  /* PP = 0b00 */
#define ESR_NEIGH_TBOX_STATUS           0x8008  /* PP = 0b00 */
#define ESR_NEIGH_TBOX_IMGT_PTR         0x8010  /* PP = 0b00 */

// ShireCache ESRs
#define ESR_CACHE_0                     0x0000  /* PP = 0b00 */
#define ESR_CACHE_IDX_COP_SM_CTL        0x0030  /* PP = 0b00 */
#define ESR_CACHE_UNKNOWN               0x01D8  /* PP = 0b00 */

// RBOX ESRs
#define ESR_RBOX_0                      0x00000 /* PP = 0b00 */
#define ESR_RBOX_CONFIG                 0x00000 /* PP = 0b00 */
#define ESR_RBOX_IN_BUF_PG              0x00001 /* PP = 0b00 */
#define ESR_RBOX_IN_BUF_CFG             0x00002 /* PP = 0b00 */
#define ESR_RBOX_OUT_BUF_PG             0x00003 /* PP = 0b00 */
#define ESR_RBOX_OUT_BUF_CFG            0x00004 /* PP = 0b00 */
#define ESR_RBOX_STATUS                 0x00005 /* PP = 0b00 */
#define ESR_RBOX_START                  0x00006 /* PP = 0b00 */
#define ESR_RBOX_CONSUME                0x00007 /* PP = 0b00 */

// Shire ESRs
#define ESR_SHIRE_0                         0x00000 /* PP = 0b00 */
#define ESR_SHIRE_MINION_FEATURE            0x00000
#define ESR_SHIRE_IPI_REDIRECT_TRIGGER      0x00080
#define ESR_SHIRE_IPI_REDIRECT_FILTER       0x00088
#define ESR_SHIRE_IPI_TRIGGER               0x00090
#define ESR_SHIRE_IPI_TRIGGER_CLEAR         0x00098
#define ESR_SHIRE_FCC0                      0x000C0
#define ESR_SHIRE_FCC1                      0x000C8
#define ESR_SHIRE_FCC2                      0x000D0
#define ESR_SHIRE_FCC3                      0x000D8
#define ESR_SHIRE_FLB0                      0x00100
#define ESR_SHIRE_FLB1                      0x00108
#define ESR_SHIRE_FLB2                      0x00110
#define ESR_SHIRE_FLB3                      0x00118
#define ESR_SHIRE_FLB4                      0x00120
#define ESR_SHIRE_FLB5                      0x00128
#define ESR_SHIRE_FLB6                      0x00130
#define ESR_SHIRE_FLB7                      0x00138
#define ESR_SHIRE_FLB8                      0x00140
#define ESR_SHIRE_FLB9                      0x00148
#define ESR_SHIRE_FLB10                     0x00150
#define ESR_SHIRE_FLB11                     0x00158
#define ESR_SHIRE_FLB12                     0x00160
#define ESR_SHIRE_FLB13                     0x00168
#define ESR_SHIRE_FLB14                     0x00170
#define ESR_SHIRE_FLB15                     0x00178
#define ESR_SHIRE_FLB16                     0x00180
#define ESR_SHIRE_FLB17                     0x00188
#define ESR_SHIRE_FLB18                     0x00190
#define ESR_SHIRE_FLB19                     0x00198
#define ESR_SHIRE_FLB20                     0x001A0
#define ESR_SHIRE_FLB21                     0x001A8
#define ESR_SHIRE_FLB22                     0x001B0
#define ESR_SHIRE_FLB23                     0x001B8
#define ESR_SHIRE_FLB24                     0x001C0
#define ESR_SHIRE_FLB25                     0x001C8
#define ESR_SHIRE_FLB26                     0x001D0
#define ESR_SHIRE_FLB27                     0x001D8
#define ESR_SHIRE_FLB28                     0x001E0
#define ESR_SHIRE_FLB29                     0x001E8
#define ESR_SHIRE_FLB30                     0x001F0
#define ESR_SHIRE_FLB31                     0x001F8
#define ESR_SHIRE_COOP_MODE                 0x00290
#define ESR_SHIRE_ICACHE_UPREFETCH          0x002F8
#define ESR_SHIRE_ICACHE_SPREFETCH          0x00300
#define ESR_SHIRE_ICACHE_MPREFETCH          0x00308
#define ESR_SHIRE_BROADCAST0                0x1FFF0
#define ESR_SHIRE_BROADCAST1                0x1FFF8

// Broadcast ESR fields
#define ESR_BROADCAST_PROT_MASK              0x1800000000000000ULL // Region protection is defined in bits [60:59] in esr broadcast data write.
#define ESR_BROADCAST_PROT_SHIFT             59
#define ESR_BROADCAST_ESR_SREGION_MASK       0x07C0000000000000ULL // bits [21:17] in Memory Shire Esr Map. Esr region.
#define ESR_BROADCAST_ESR_SREGION_MASK_SHIFT 54
#define ESR_BROADCAST_ESR_ADDR_MASK          0x003FFF0000000000ULL // bits[17:3] in Memory Shire Esr Map. Esr address
#define ESR_BROADCAST_ESR_ADDR_SHIFT         40
#define ESR_BROADCAST_ESR_SHIRE_MASK         0x000000FFFFFFFFFFULL // bit mask Shire to spread the broadcast bits
#define ESR_BROADCAST_ESR_MAX_SHIRES         ESR_BROADCAST_ESR_ADDR_SHIFT

// L2
#define SC_NUM_BANKS  4

// L2 scratchpad
#define L2_SCP_BASE        0x80000000ULL
#define L2_SCP_OFFSET      0x00800000ULL
#define L2_SCP_SIZE        0x00400000ULL
#define L2_SCP_LINEAR_BASE 0xC0000000ULL
#define L2_SCP_LINEAR_SIZE 0x40000000ULL

// CSRs
enum : uint16_t {
#define CSRDEF(num, lower, upper)       CSR_##upper = num,
#include "csrs.h"
#undef CSRDEF
};

// Memory access type
enum mem_access_type {
    Mem_Access_Load,
    Mem_Access_Store,
    Mem_Access_Fetch,
    Mem_Access_PTW,
    Mem_Access_AtomicL,
    Mem_Access_AtomicG,
    Mem_Access_TxLoad,
    Mem_Access_TxStore,
    Mem_Access_Prefetch,
    Mem_Access_CacheOp
};

enum mreg {
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
};

enum xreg {
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
};

enum freg {
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
    MAXFREG = 32,
    fnone = -1
};

enum rounding_mode {
    rne = 0,
    rtz = 1,
    rdn = 2,
    rup = 3,
    rmm = 4,
    rmdyn = 7 //dynamic rounding mode, read from rm register
};

enum et_core_t {
   ET_MINION = 0,
   ET_MAXION
};

// Number of 32-bit lanes in a vector register
#ifndef VL
#define VL  8
#endif

#if (VL != 8)
#error "Only 256-bit vectors supported"
#endif

/* Obsolete texsnd/texrcv instuctions use the low 128b of the fregs for data transfers */
#define VL_TBOX 4

// generic array of aliased types
template<size_t N> union fdata_array_t {
    uint8_t   b[N*4];
    uint16_t  h[N*2];
    uint32_t  u[N];
    int32_t   i[N];
    uint64_t  x[N/2];
    int64_t   q[N/2];
};

// vector register value type
typedef fdata_array_t<VL> fdata;

// general purpose register value type
union xdata {
    uint8_t   b[8];
    uint16_t  h[4];
    uint32_t  w[2];
    int32_t   ws[2];
    uint64_t  x;
    int64_t   xs;
};

// mask register value type
struct mdata {
    std::bitset<VL>  b;
};

// Privilege levels
#define CSR_PRV_U  0
#define CSR_PRV_S  1
#define CSR_PRV_H  2
#define CSR_PRV_M  3

// Traps
#define CAUSE_MISALIGNED_FETCH          0x00
#define CAUSE_FETCH_ACCESS              0x01
#define CAUSE_ILLEGAL_INSTRUCTION       0x02
#define CAUSE_BREAKPOINT                0x03
#define CAUSE_MISALIGNED_LOAD           0x04
#define CAUSE_LOAD_ACCESS               0x05
#define CAUSE_MISALIGNED_STORE          0x06
#define CAUSE_STORE_ACCESS              0x07
#define CAUSE_USER_ECALL                0x08
#define CAUSE_SUPERVISOR_ECALL          0x09
#define CAUSE_MACHINE_ECALL             0x0b
#define CAUSE_FETCH_PAGE_FAULT          0x0c
#define CAUSE_LOAD_PAGE_FAULT           0x0d
#define CAUSE_STORE_PAGE_FAULT          0x0f
#define CAUSE_FETCH_BUS_ERROR           0x19
#define CAUSE_FETCH_ECC_ERROR           0x1a
#define CAUSE_LOAD_PAGE_SPLIT_FAULT     0x1b
#define CAUSE_STORE_PAGE_SPLIT_FAULT    0x1c
#define CAUSE_BUS_ERROR                 0x1d
#define CAUSE_MCODE_INSTRUCTION         0x1e
#define CAUSE_TXFMA_OFF                 0x1f
#define CAUSE_USER_SOFTWARE_INTERRUPT       0x8000000000000000ULL
#define CAUSE_SUPERVISOR_SOFTWARE_INTERRUPT 0x8000000000000001ULL
#define CAUSE_MACHINE_SOFTWARE_INTERRUPT    0x8000000000000003ULL
#define CAUSE_USER_TIMER_INTERRUPT          0x8000000000000004ULL
#define CAUSE_SUPERVISOR_TIMER_INTERRUPT    0x8000000000000005ULL
#define CAUSE_MACHINE_TIMER_INTERRUPT       0x8000000000000007ULL
#define CAUSE_USER_EXTERNAL_INTERRUPT       0x8000000000000008ULL
#define CAUSE_SUPERVISOR_EXTERNAL_INTERRUPT 0x8000000000000009ULL
#define CAUSE_MACHINE_EXTERNAL_INTERRUPT    0x800000000000000bULL

// base class for all traps
class trap_t {
  public:
    trap_t(uint64_t n) : cause(n) {}
    uint64_t get_cause() const { return cause; }

    virtual bool has_tval() const = 0;
    virtual uint64_t get_tval() const = 0;
    virtual const char* what() const = 0;

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
        virtual const char* what() const override { return #x; } \
    };

// define a trap type with tval
#define DECLARE_TRAP_TVAL_Y(n, x) \
    class x : public trap_t { \
      public: \
        x(uint64_t v) : trap_t(n), tval(v) {} \
        virtual bool has_tval() const override { return true; } \
        virtual uint64_t get_tval() const override { return tval; } \
        virtual const char* what() const override { return #x; } \
      private: \
        const uint64_t tval; \
    };

// Exceptions
DECLARE_TRAP_TVAL_Y(CAUSE_MISALIGNED_FETCH,       trap_instruction_address_misaligned)
DECLARE_TRAP_TVAL_Y(CAUSE_FETCH_ACCESS,           trap_instruction_access_fault)
DECLARE_TRAP_TVAL_Y(CAUSE_ILLEGAL_INSTRUCTION,    trap_illegal_instruction)
DECLARE_TRAP_TVAL_Y(CAUSE_BREAKPOINT,             trap_breakpoint)
DECLARE_TRAP_TVAL_Y(CAUSE_MISALIGNED_LOAD,        trap_load_address_misaligned)
DECLARE_TRAP_TVAL_Y(CAUSE_MISALIGNED_STORE,       trap_store_address_misaligned)
DECLARE_TRAP_TVAL_Y(CAUSE_LOAD_ACCESS,            trap_load_access_fault)
DECLARE_TRAP_TVAL_Y(CAUSE_STORE_ACCESS,           trap_store_access_fault)
DECLARE_TRAP_TVAL_N(CAUSE_USER_ECALL,             trap_user_ecall)
DECLARE_TRAP_TVAL_N(CAUSE_SUPERVISOR_ECALL,       trap_supervisor_ecall)
DECLARE_TRAP_TVAL_N(CAUSE_MACHINE_ECALL,          trap_machine_ecall)
DECLARE_TRAP_TVAL_Y(CAUSE_FETCH_PAGE_FAULT,       trap_instruction_page_fault)
DECLARE_TRAP_TVAL_Y(CAUSE_LOAD_PAGE_FAULT,        trap_load_page_fault)
DECLARE_TRAP_TVAL_Y(CAUSE_STORE_PAGE_FAULT,       trap_store_page_fault)
DECLARE_TRAP_TVAL_Y(CAUSE_LOAD_PAGE_SPLIT_FAULT,  trap_load_split_page_fault)
DECLARE_TRAP_TVAL_Y(CAUSE_STORE_PAGE_SPLIT_FAULT, trap_store_split_page_fault)
DECLARE_TRAP_TVAL_Y(CAUSE_BUS_ERROR,              trap_bus_error)
DECLARE_TRAP_TVAL_Y(CAUSE_MCODE_INSTRUCTION,      trap_mcode_instruction)
DECLARE_TRAP_TVAL_Y(CAUSE_TXFMA_OFF,              trap_txfma_off)
DECLARE_TRAP_TVAL_N(CAUSE_FETCH_BUS_ERROR,        trap_fetch_bus_error)
DECLARE_TRAP_TVAL_N(CAUSE_FETCH_ECC_ERROR,        trap_fetch_ecc_error)

// Interrupts
DECLARE_TRAP_TVAL_N(CAUSE_USER_SOFTWARE_INTERRUPT,       trap_user_software_interrupt)
DECLARE_TRAP_TVAL_N(CAUSE_SUPERVISOR_SOFTWARE_INTERRUPT, trap_supervisor_software_interrupt)
DECLARE_TRAP_TVAL_N(CAUSE_MACHINE_SOFTWARE_INTERRUPT,    trap_machine_software_interrupt)
DECLARE_TRAP_TVAL_N(CAUSE_USER_TIMER_INTERRUPT,          trap_user_timer_interrupt)
DECLARE_TRAP_TVAL_N(CAUSE_SUPERVISOR_TIMER_INTERRUPT,    trap_supervisor_timer_interrupt)
DECLARE_TRAP_TVAL_N(CAUSE_MACHINE_TIMER_INTERRUPT,       trap_machine_timer_interrupt)
DECLARE_TRAP_TVAL_N(CAUSE_USER_EXTERNAL_INTERRUPT,       trap_user_external_interrupt)
DECLARE_TRAP_TVAL_N(CAUSE_SUPERVISOR_EXTERNAL_INTERRUPT, trap_supervisor_external_interrupt)
DECLARE_TRAP_TVAL_N(CAUSE_MACHINE_EXTERNAL_INTERRUPT,    trap_machine_external_interrupt)

class checker_wait_t {
public:
  virtual const char * what () const = 0;
};

// define a checker wait exception
#define DECLARE_CHECKER_WAIT(x)                                  \
  class x : public checker_wait_t {                              \
  public:                                                        \
  virtual const char* what() const override { return #x; }             \
  };

DECLARE_CHECKER_WAIT(checker_wait_fcc)

#endif // _EMU_DEFINES_H
