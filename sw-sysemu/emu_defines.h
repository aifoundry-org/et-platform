#ifndef BEMU_DEFINES_H
#define BEMU_DEFINES_H

#include "state.h"

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
#define TFMA_REGS_PER_ROW (TFMA_MAX_BCOLS / (VLEN/32))
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

// ESR region 'pp' field in bits [31:30]
#define ESR_REGION_PROT_MASK    0x00C0000000ULL // ESR Region Protection is defined in bits [31:30]
#define ESR_REGION_PROT_SHIFT   30              // Bits to shift to get the ESR Region Protection defined in bits [31:30]

// ESR region 'shireid' field in bits [29:22]
// The local shire has bits [29:22] = 8'b11111111
#define ESR_REGION_SHIRE_MASK   0x003FC00000ULL
#define ESR_REGION_LOCAL_SHIRE  0x003FC00000ULL
#define ESR_REGION_SHIRE_SHIFT  22

// ESR region 'hart' field in bits [19:12] (when 'subregion' is 2'b00)
#define ESR_REGION_HART_MASK    0x00000FF000ULL
#define ESR_REGION_HART_SHIFT   12

// ESR region 'neighborhood' field in bits [19:16] (when 'subregion' is 2'b01)
// The broadcast neighborhood has bits [19:16] == 4'b1111
#define ESR_REGION_NEIGH_MASK       0x00000F0000ULL
#define ESR_REGION_NEIGH_BROADCAST  0x00000F0000ULL
#define ESR_REGION_NEIGH_SHIFT      16

// ESR region 'bank' field in bits [16:13] (when 'subregion' is 2'b11)
#define ESR_REGION_BANK_MASK    0x000001E000ULL
#define ESR_REGION_BANK_SHIFT   13

// ESR region 'ESR' field in bits [xx:3] (depends on 'subregion' and
// 'extregion' fields). The following masks have all the bits of the address
// enabled, except the ones specifiying the 'shire', 'neigh', 'hart', and
// 'bank' fields.
#define ESR_HART_ESR_MASK       0xFFC0300FFFULL
#define ESR_NEIGH_ESR_MASK      0xFFC030FFFFULL
#define ESR_CACHE_ESR_MASK      0xFFC03E1FFFULL
#define ESR_SHIRE_ESR_MASK      0xFFC03FFFFFULL
#define ESR_RBOX_ESR_MASK       0xFFC03FFFFFULL

// bits [21:20] and further limited by bits [19:12] depending on the region
#define ESR_SREGION_MASK        0x0100300000ULL

// The ESR Extended Region is defined by bits [21:17] (when 'subregion' is 2'b11)
#define ESR_SREGION_EXT_MASK    0x01003E0000ULL
#define ESR_SREGION_EXT_SHIFT   17

// Base addresses for the various ESR subregions
//  * Hart ESR Region is at region [21:20] == 2'b00
//  * Neigh ESR Region is at region [21:20] == 2'b01
//  * The ESR Region at region [21:20] == 2'b10 is reserved
//  * ShireCache ESR Region is at region [21:20] == 2'b11 and [19:17] == 2'b000
//  * RBOX ESR Region is at region [21:20] == 2'b11 and [19:17] == 2'b001
//  * ShireOther ESR Region is at region [21:20] == 2'b11 and [19:17] == 2'b010
#define ESR_HART_REGION        0x0100000000ULL
#define ESR_NEIGH_REGION       0x0100100000ULL
#define ESR_RSRVD_REGION       0x0100200000ULL
#define ESR_CACHE_REGION       0x0100300000ULL
#define ESR_RBOX_REGION        0x0100320000ULL
#define ESR_SHIRE_REGION       0x0100340000ULL

// Helper macros to construct ESR addresses in the various subregions

#define ESR_HART(shire, hart, name) \
    ((uint64_t(shire) << ESR_REGION_SHIRE_SHIFT) + \
     (uint64_t(hart) << ESR_REGION_HART_SHIFT) + \
      uint64_t(ESR_ ## name))

#define ESR_NEIGH(shire, neigh, name) \
    ((uint64_t(shire) << ESR_REGION_SHIRE_SHIFT) + \
     (uint64_t(neigh) << ESR_REGION_NEIGH_SHIFT) + \
      uint64_t(ESR_ ## name))

#define ESR_CACHE(shire, bank, name) \
    ((uint64_t(shire) << ESR_REGION_SHIRE_SHIFT) + \
     (uint64_t(bank) << ESR_REGION_BANK_SHIFT) + \
      uint64_t(ESR_ ## name))

#define ESR_RBOX(shire, name) \
    ((uint64_t(shire) << ESR_REGION_SHIRE_SHIFT) + \
      uint64_t(ESR_ ## name))

#define ESR_SHIRE(shire, name) \
    ((uint64_t(shire) << ESR_REGION_SHIRE_SHIFT) + \
      uint64_t(ESR_ ## name))

// Hart ESRs
#define ESR_HART_U0             0x0100000000ULL /* PP = 0b00 */
#define ESR_HART_S0             0x0140000000ULL /* PP = 0b01 */
#define ESR_HART_M0             0x01C0000000ULL /* PP = 0b11 */

// Message Ports
#define ESR_PORT0               0x0100000800ULL /* PP = 0b00 */
#define ESR_PORT1               0x0100000840ULL /* PP = 0b00 */
#define ESR_PORT2               0x0100000880ULL /* PP = 0b00 */
#define ESR_PORT3               0x01000008c0ULL /* PP = 0b00 */

// Neighborhood ESRs
#define ESR_NEIGH_U0                0x0100100000ULL /* PP = 0b00 */
#define ESR_NEIGH_S0                0x0140100000ULL /* PP = 0b01 */
#define ESR_NEIGH_M0                0x01C0100000ULL /* PP = 0b11 */
#define ESR_DUMMY0                  0x0100100000ULL /* PP = 0b00 */
#define ESR_DUMMY1                  0x0100100008ULL /* PP = 0b00 */
#define ESR_MINION_BOOT             0x01C0100018ULL /* PP = 0b11 */
#define ESR_MPROT                   0x01C0100020ULL /* PP = 0b11 */
#define ESR_DUMMY2                  0x01C0100028ULL /* PP = 0b11 */
#define ESR_DUMMY3                  0x01C0100030ULL /* PP = 0b11 */
#define ESR_VMSPAGESIZE             0x01C0100038ULL /* PP = 0b11 */
#define ESR_IPI_REDIRECT_PC         0x0100100040ULL /* PP = 0b00 */
#define ESR_PMU_CTRL                0x01C0100068ULL /* PP = 0b11 */
#define ESR_NEIGH_CHICKEN           0x01C0100070ULL /* PP = 0b11 */
#define ESR_ICACHE_ERR_LOG_CTL      0x01C0100078ULL /* PP = 0b11 */
#define ESR_ICACHE_ERR_LOG_INFO     0x01C0100080ULL /* PP = 0b11 */
#define ESR_ICACHE_ERR_LOG_ADDRESS  0x01C0100088ULL /* PP = 0b11 */
#define ESR_ICACHE_SBE_DBE_COUNTS   0x01C0100090ULL /* PP = 0b11 */
#define ESR_TEXTURE_CONTROL         0x0100108000ULL /* PP = 0b00 */
#define ESR_TEXTURE_STATUS          0x0100108008ULL /* PP = 0b00 */
#define ESR_TEXTURE_IMAGE_TABLE_PTR 0x0100108010ULL /* PP = 0b00 */

// ShireCache ESRs
#define ESR_CACHE_U0                      0x0100300000ULL /* PP = 0b00 */
#define ESR_CACHE_S0                      0x0140300000ULL /* PP = 0b01 */
#define ESR_CACHE_M0                      0x01C0300000ULL /* PP = 0b11 */
#define ESR_SC_L3_SHIRE_SWIZZLE_CTL       0x01C0300000ULL /* PP = 0b11 */
#define ESR_SC_REQQ_CTL                   0x01C0300008ULL /* PP = 0b11 */
#define ESR_SC_PIPE_CTL                   0x01C0300010ULL /* PP = 0b11 */
#define ESR_SC_L2_CACHE_CTL               0x01C0300018ULL /* PP = 0b11 */
#define ESR_SC_L3_CACHE_CTL               0x01C0300020ULL /* PP = 0b11 */
#define ESR_SC_SCP_CACHE_CTL              0x01C0300028ULL /* PP = 0b11 */
#define ESR_SC_IDX_COP_SM_CTL             0x01C0300030ULL /* PP = 0b11 */
#define ESR_SC_IDX_COP_SM_PHYSICAL_INDEX  0x01C0300038ULL /* PP = 0b11 */
#define ESR_SC_IDX_COP_SM_DATA0           0x01C0300040ULL /* PP = 0b11 */
#define ESR_SC_IDX_COP_SM_DATA1           0x01C0300048ULL /* PP = 0b11 */
#define ESR_SC_IDX_COP_SM_ECC             0x01C0300050ULL /* PP = 0b11 */
#define ESR_SC_ERR_LOG_CTL                0x01C0300058ULL /* PP = 0b11 */
#define ESR_SC_ERR_LOG_INFO               0x01C0300060ULL /* PP = 0b11 */
#define ESR_SC_ERR_LOG_ADDRESS            0x01C0300068ULL /* PP = 0b11 */
#define ESR_SC_SBE_DBE_COUNTS             0x01C0300070ULL /* PP = 0b11 */
#define ESR_SC_REQQ_DEBUG_CTL             0x01C0300078ULL /* PP = 0b11 */
#define ESR_SC_REQQ_DEBUG0                0x01C0300080ULL /* PP = 0b11 */
#define ESR_SC_REQQ_DEBUG1                0x01C0300088ULL /* PP = 0b11 */
#define ESR_SC_REQQ_DEBUG2                0x01C0300090ULL /* PP = 0b11 */
#define ESR_SC_REQQ_DEBUG3                0x01C0300098ULL /* PP = 0b11 */
//#define ESR_SC_TRACE_ADDRESS_ENABLE     /* PP = 0b10 */
//#define ESR_SC_TRACE_ADDRESS_VALUE      /* PP = 0b10 */
//#define ESR_SC_TRACE_CTL                /* PP = 0b10 */
//#define ESR_SC_PERFMON_CTL_STATUS       /* PP = 0b11 */
//#define ESR_SC_PERFMON_CYC_CNTR         /* PP = 0b11 */
//#define ESR_SC_PERFMON_P0_CNTR          /* PP = 0b11 */
//#define ESR_SC_PERFMON_P1_CNTR          /* PP = 0b11 */
//#define ESR_SC_PERFMON_P0_QUAL          /* PP = 0b11 */
//#define ESR_SC_PERFMON_P1_QUAL          /* PP = 0b11 */


// RBOX ESRs
#define ESR_RBOX_U0             0x0100320000ULL /* PP = 0b00 */
#define ESR_RBOX_S0             0x0140320000ULL /* PP = 0b01 */
#define ESR_RBOX_M0             0x01C0320000ULL /* PP = 0b11 */
#define ESR_RBOX_CONFIG         0x0100320000ULL /* PP = 0b00 */
#define ESR_RBOX_IN_BUF_PG      0x0100320008ULL /* PP = 0b00 */
#define ESR_RBOX_IN_BUF_CFG     0x0100320010ULL /* PP = 0b00 */
#define ESR_RBOX_OUT_BUF_PG     0x0100320018ULL /* PP = 0b00 */
#define ESR_RBOX_OUT_BUF_CFG    0x0100320020ULL /* PP = 0b00 */
#define ESR_RBOX_STATUS         0x0100320028ULL /* PP = 0b00 */
#define ESR_RBOX_START          0x0100320030ULL /* PP = 0b00 */
#define ESR_RBOX_CONSUME        0x0100320038ULL /* PP = 0b00 */

// Shire ESRs
#define ESR_SHIRE_U0              0x0100340000ULL /* PP = 0b00 */
#define ESR_SHIRE_S0              0x0140340000ULL /* PP = 0b01 */
#define ESR_SHIRE_M0              0x01C0340000ULL /* PP = 0b11 */
#define ESR_MINION_FEATURE        0x01C0340000ULL /* PP = 0b11 */
#define ESR_IPI_REDIRECT_TRIGGER  0x0100340080ULL /* PP = 0b00 */
#define ESR_IPI_REDIRECT_FILTER   0x01C0340088ULL /* PP = 0b11 */
#define ESR_IPI_TRIGGER           0x01C0340090ULL /* PP = 0b11 */
#define ESR_IPI_TRIGGER_CLEAR     0x01C0340098ULL /* PP = 0b11 */
#define ESR_FCC_CREDINC_0         0x01003400C0ULL /* PP = 0b00 */
#define ESR_FCC_CREDINC_1         0x01003400C8ULL /* PP = 0b00 */
#define ESR_FCC_CREDINC_2         0x01003400D0ULL /* PP = 0b00 */
#define ESR_FCC_CREDINC_3         0x01003400D8ULL /* PP = 0b00 */
#define ESR_FAST_LOCAL_BARRIER0   0x0100340100ULL /* PP = 0b00 */
#define ESR_FAST_LOCAL_BARRIER1   0x0100340108ULL /* PP = 0b00 */
#define ESR_FAST_LOCAL_BARRIER2   0x0100340110ULL /* PP = 0b00 */
#define ESR_FAST_LOCAL_BARRIER3   0x0100340118ULL /* PP = 0b00 */
#define ESR_FAST_LOCAL_BARRIER4   0x0100340120ULL /* PP = 0b00 */
#define ESR_FAST_LOCAL_BARRIER5   0x0100340128ULL /* PP = 0b00 */
#define ESR_FAST_LOCAL_BARRIER6   0x0100340130ULL /* PP = 0b00 */
#define ESR_FAST_LOCAL_BARRIER7   0x0100340138ULL /* PP = 0b00 */
#define ESR_FAST_LOCAL_BARRIER8   0x0100340140ULL /* PP = 0b00 */
#define ESR_FAST_LOCAL_BARRIER9   0x0100340148ULL /* PP = 0b00 */
#define ESR_FAST_LOCAL_BARRIER10  0x0100340150ULL /* PP = 0b00 */
#define ESR_FAST_LOCAL_BARRIER11  0x0100340158ULL /* PP = 0b00 */
#define ESR_FAST_LOCAL_BARRIER12  0x0100340160ULL /* PP = 0b00 */
#define ESR_FAST_LOCAL_BARRIER13  0x0100340168ULL /* PP = 0b00 */
#define ESR_FAST_LOCAL_BARRIER14  0x0100340170ULL /* PP = 0b00 */
#define ESR_FAST_LOCAL_BARRIER15  0x0100340178ULL /* PP = 0b00 */
#define ESR_FAST_LOCAL_BARRIER16  0x0100340180ULL /* PP = 0b00 */
#define ESR_FAST_LOCAL_BARRIER17  0x0100340188ULL /* PP = 0b00 */
#define ESR_FAST_LOCAL_BARRIER18  0x0100340190ULL /* PP = 0b00 */
#define ESR_FAST_LOCAL_BARRIER19  0x0100340198ULL /* PP = 0b00 */
#define ESR_FAST_LOCAL_BARRIER20  0x01003401A0ULL /* PP = 0b00 */
#define ESR_FAST_LOCAL_BARRIER21  0x01003401A8ULL /* PP = 0b00 */
#define ESR_FAST_LOCAL_BARRIER22  0x01003401B0ULL /* PP = 0b00 */
#define ESR_FAST_LOCAL_BARRIER23  0x01003401B8ULL /* PP = 0b00 */
#define ESR_FAST_LOCAL_BARRIER24  0x01003401C0ULL /* PP = 0b00 */
#define ESR_FAST_LOCAL_BARRIER25  0x01003401C8ULL /* PP = 0b00 */
#define ESR_FAST_LOCAL_BARRIER26  0x01003401D0ULL /* PP = 0b00 */
#define ESR_FAST_LOCAL_BARRIER27  0x01003401D8ULL /* PP = 0b00 */
#define ESR_FAST_LOCAL_BARRIER28  0x01003401E0ULL /* PP = 0b00 */
#define ESR_FAST_LOCAL_BARRIER29  0x01003401E8ULL /* PP = 0b00 */
#define ESR_FAST_LOCAL_BARRIER30  0x01003401F0ULL /* PP = 0b00 */
#define ESR_FAST_LOCAL_BARRIER31  0x01003401F8ULL /* PP = 0b00 */
#define ESR_SHIRE_COOP_MODE       0x0140340290ULL /* PP = 0b01 */
#define ESR_ICACHE_UPREFETCH      0x01003402F8ULL /* PP = 0b00 */
#define ESR_ICACHE_SPREFETCH      0x0140340300ULL /* PP = 0b01 */
#define ESR_ICACHE_MPREFETCH      0x01C0340308ULL /* PP = 0b11 */

// Broadcast ESRs
#define ESR_BROADCAST_DATA      0x013FF5FFF0ULL /* PP = 0b00 */
#define ESR_UBROADCAST          0x013FF5FFF8ULL /* PP = 0b00 */
#define ESR_SBROADCAST          0x017FF5FFF8ULL /* PP = 0b01 */
#define ESR_MBROADCAST          0x01FFF5FFF8ULL /* PP = 0b11 */

// L2
#define SC_NUM_BANKS  4

// L2 scratchpad
#define L2_SCP_BASE        0x80000000ULL
#define L2_SCP_OFFSET      0x00800000ULL
#define L2_SCP_SIZE        0x00400000ULL
#define L2_SCP_LINEAR_BASE 0xC0000000ULL
#define L2_SCP_LINEAR_SIZE 0x40000000ULL

// Message ports
#define ESR_HART_PORT_ADDR_VALID(x)         (((x) & 0xF38) == 0x800)
#define ESR_HART_PORT_NUM_MASK              0xC0ULL
#define ESR_HART_PORT_NUM_SHIFT             6

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
#define VL  (VLEN/32)
#endif

/* Obsolete texsnd/texrcv instuctions use the low 128b of the fregs for data transfers */
#define VL_TBOX 4

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
#define CAUSE_USER_SOFTWARE_INTERRUPT               0x8000000000000000ULL
#define CAUSE_SUPERVISOR_SOFTWARE_INTERRUPT         0x8000000000000001ULL
#define CAUSE_MACHINE_SOFTWARE_INTERRUPT            0x8000000000000003ULL
#define CAUSE_USER_TIMER_INTERRUPT                  0x8000000000000004ULL
#define CAUSE_SUPERVISOR_TIMER_INTERRUPT            0x8000000000000005ULL
#define CAUSE_MACHINE_TIMER_INTERRUPT               0x8000000000000007ULL
#define CAUSE_USER_EXTERNAL_INTERRUPT               0x8000000000000008ULL
#define CAUSE_SUPERVISOR_EXTERNAL_INTERRUPT         0x8000000000000009ULL
#define CAUSE_MACHINE_EXTERNAL_INTERRUPT            0x800000000000000bULL
#define CAUSE_BAD_IPI_REDICERT_INTERRUPT            0x800000000000000fULL
#define CAUSE_ICACHE_ECC_COUNTER_OVERFLOW_INTERRUPT 0x8000000000000013ULL

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
DECLARE_TRAP_TVAL_N(CAUSE_USER_SOFTWARE_INTERRUPT,                trap_user_software_interrupt)
DECLARE_TRAP_TVAL_N(CAUSE_SUPERVISOR_SOFTWARE_INTERRUPT,          trap_supervisor_software_interrupt)
DECLARE_TRAP_TVAL_N(CAUSE_MACHINE_SOFTWARE_INTERRUPT,             trap_machine_software_interrupt)
DECLARE_TRAP_TVAL_N(CAUSE_USER_TIMER_INTERRUPT,                   trap_user_timer_interrupt)
DECLARE_TRAP_TVAL_N(CAUSE_SUPERVISOR_TIMER_INTERRUPT,             trap_supervisor_timer_interrupt)
DECLARE_TRAP_TVAL_N(CAUSE_MACHINE_TIMER_INTERRUPT,                trap_machine_timer_interrupt)
DECLARE_TRAP_TVAL_N(CAUSE_USER_EXTERNAL_INTERRUPT,                trap_user_external_interrupt)
DECLARE_TRAP_TVAL_N(CAUSE_SUPERVISOR_EXTERNAL_INTERRUPT,          trap_supervisor_external_interrupt)
DECLARE_TRAP_TVAL_N(CAUSE_MACHINE_EXTERNAL_INTERRUPT,             trap_machine_external_interrupt)
DECLARE_TRAP_TVAL_N(CAUSE_BAD_IPI_REDICERT_INTERRUPT,             trap_bad_ipi_redirect_interrupt)
DECLARE_TRAP_TVAL_N(CAUSE_ICACHE_ECC_COUNTER_OVERFLOW_INTERRUPT,  trap_icache_ecc_counter_overflow_interrupt)

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

#endif // BEMU_DEFINES_H
