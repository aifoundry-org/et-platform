/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef BEMU_DEFINES_H
#define BEMU_DEFINES_H

#include "state.h"
#include "traps.h"

namespace bemu {


// Maximum number of threads
#define EMU_NUM_SHIRES          35
#define EMU_NUM_MINION_SHIRES   (EMU_NUM_SHIRES - 1)
#define EMU_NUM_COMPUTE_SHIRES  (EMU_NUM_MINION_SHIRES - 2)
#define EMU_MASTER_SHIRE        (EMU_NUM_MINION_SHIRES - 2)
#define EMU_SPARE_SHIRE         (EMU_NUM_MINION_SHIRES - 1)
#define EMU_IO_SHIRE_SP         (EMU_NUM_SHIRES - 1)
#define EMU_THREADS_PER_MINION  2
#define EMU_MINIONS_PER_NEIGH   8
#define EMU_THREADS_PER_NEIGH   (EMU_THREADS_PER_MINION * EMU_MINIONS_PER_NEIGH)
#define EMU_NEIGH_PER_SHIRE     4
#define EMU_MINIONS_PER_SHIRE   (EMU_MINIONS_PER_NEIGH * EMU_NEIGH_PER_SHIRE)
#define EMU_THREADS_PER_SHIRE   (EMU_THREADS_PER_NEIGH * EMU_NEIGH_PER_SHIRE)
#define EMU_TBOXES_PER_SHIRE    2
#define EMU_RBOXES_PER_SHIRE    1
#define EMU_NUM_NEIGHS          ((EMU_NUM_MINION_SHIRES * EMU_NEIGH_PER_SHIRE) + 1)
#define EMU_NUM_MINIONS         ((EMU_NUM_MINION_SHIRES * EMU_MINIONS_PER_SHIRE) + 1)
#define EMU_NUM_THREADS         ((EMU_NUM_MINION_SHIRES * EMU_THREADS_PER_SHIRE) + 1)
#define EMU_NUM_TBOXES          (EMU_NUM_COMPUTE_SHIRES * EMU_TBOXES_PER_SHIRE)
#define EMU_NUM_RBOXES          (EMU_NUM_COMPUTE_SHIRES * EMU_RBOXES_PER_SHIRE)
#define EMU_IO_SHIRE_SP_THREAD  (EMU_NUM_THREADS - 1)
#define IO_SHIRE_ID             254
#define IO_SHIRE_SP_HARTID      (IO_SHIRE_ID * EMU_THREADS_PER_SHIRE)

// Message ports
#define NR_MSG_PORTS         4
#define PORT_LOG2_MIN_SIZE   2
#define PORT_LOG2_MAX_SIZE   5

// Some TensorFMA defines
#define TFMA_MAX_AROWS    16
#define TFMA_MAX_ACOLS    16
#define TFMA_MAX_BCOLS    16
#define TFMA_REGS_PER_ROW (TFMA_MAX_BCOLS / unsigned(VLEN/32))

// FastLocalBarrier
#define FAST_LOCAL_BARRIERS 32

// TensorQuant defines
#define TQUANT_MAX_TRANS 10

// FastCreditCounters
#define EMU_NUM_FCC_COUNTERS_PER_THREAD 2

// Main memory size (16GiB)
#define EMU_DRAM_SIZE  (16ULL*1024ULL*1024ULL*1024ULL)

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

// MATP mode field values
#define MATP_MODE_BARE  0
#define MATP_MODE_MV39  8
#define MATP_MODE_MV48  9

// MSTATUS field offsets
#define MSTATUS_MXR     19
#define MSTATUS_SUM     18
#define MSTATUS_MPRV    17
#define MSTATUS_XS      15
#define MSTATUS_FS      13
#define MSTATUS_MPP     11
#define MSTATUS_SPP     8

// L2
#define SC_NUM_BANKS  4

// CSRs
enum : uint16_t {
#define CSRDEF(num, lower, upper)       CSR_##upper = num,
#include "csrs.h"
#undef CSRDEF
};

// Memory access type
enum mem_access_type {
    Mem_Access_Load,
    Mem_Access_LoadL,
    Mem_Access_LoadG,
    Mem_Access_Store,
    Mem_Access_StoreL,
    Mem_Access_StoreG,
    Mem_Access_Fetch,
    Mem_Access_PTW,
    Mem_Access_AtomicL,
    Mem_Access_AtomicG,
    Mem_Access_TxLoad,
    Mem_Access_TxLoadL2Scp,
    Mem_Access_TxStore,
    Mem_Access_Prefetch,
    Mem_Access_CacheOp
};

#define Mem_Access_Type_Size 14

// CacheOp type
enum cacheop_type {
    CacheOp_None,
    CacheOp_EvictL2,
    CacheOp_EvictL3,
    CacheOp_EvictDDR,
    CacheOp_FlushL2,
    CacheOp_FlushL3,
    CacheOp_FlushDDR,
    CacheOp_PrefetchL1,
    CacheOp_PrefetchL2,
    CacheOp_PrefetchL3,
    CacheOp_Lock,
    CacheOp_Unlock,
    CacheOp_CacheOp
};

using mreg = unsigned;
using xreg = unsigned;
using freg = unsigned;

enum : mreg {
    m0 = 0,
    m1 = 1,
    m2 = 2,
    m3 = 3,
    m4 = 4,
    m5 = 5,
    m6 = 6,
    m7 = 7,
};

enum : xreg {
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
};

enum : freg {
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
};

/* Obsolete texsnd/texrcv instuctions use the low 128b of the fregs for data transfers */
#define VL_TBOX 4

// ET DV environment commands
#define ET_DIAG_NOP             (0x0)
#define ET_DIAG_PUTCHAR         (0x1)
#define ET_DIAG_RAND            (0x2)
#define ET_DIAG_RAND_MEM_UPPER  (0x3)
#define ET_DIAG_RAND_MEM_LOWER  (0x4)
#define ET_DIAG_IRQ_INJ         (0x5)
#define ET_DIAG_ECC_INJ         (0x6)
#define ET_DIAG_CYCLE           (0x7)

// ET DV DIAG_IRQ_INJ sub-opcode
#define ET_DIAG_IRQ_INJ_MEI     (0)
#define ET_DIAG_IRQ_INJ_TI      (1)
#define ET_DIAG_IRQ_INJ_SEI     (2)

// mem reset pattern is 4 bytes (to allow for instance, 0xDEADBEEF)
#define MEM_RESET_PATTERN_SIZE 4

// L2 scratchpad
#define L2_SCP_BASE        0x80000000ULL
#define L2_SCP_OFFSET      0x00800000ULL
#define L2_SCP_SIZE        0x00400000ULL
#define L2_SCP_LINEAR_BASE 0xC0000000ULL
#define L2_SCP_LINEAR_SIZE 0x40000000ULL
#define SCP_REGION_BASE    0x80000000ULL
#define SCP_REGION_SIZE    0x80000000ULL

// IO region
#define IO_REGION_BASE     0x0000000000ULL
#define IO_REGION_SIZE     0x0040000000ULL

// PU PLIC
#define PU_PLIC_TIMER0_INTR_ID       6
#define PU_PLIC_PCIE_MESSAGE_INTR_ID 33

// SPIO PLIC
#define SPIO_PLIC_PSHIRE_PCIE0_EDMA0_INTR_ID 96
#define SPIO_PLIC_MBOX_MMIN_INTR_ID          114
#define SPIO_PLIC_MBOX_HOST_INTR_ID          115
#define SPIO_PLIC_GPIO_INTR_ID               122

// PCIe
#define ETSOC_CX_ATU_NUM_INBOUND_REGIONS 32
#define ETSOC_CC_NUM_DMA_WR_CHAN 4
#define ETSOC_CC_NUM_DMA_RD_CHAN 4

// PMU
#define PMU_MINION_EVENT_NONE             0
#define PMU_MINION_EVENT_CYCLES           1
#define PMU_MINION_EVENT_RETIRED_INST0    2
#define PMU_MINION_EVENT_RETIRED_INST1    3
#define PMU_MINION_EVENT_BRANCHES0        4
#define PMU_MINION_EVENT_BRANCHES1        5
#define PMU_MINION_EVENT_DCACHE_ACCESS0   6
#define PMU_MINION_EVENT_DCACHE_ACCESS1   7
#define PMU_MINION_EVENT_DCACHE_MISSES0   8
#define PMU_MINION_EVENT_DCACHE_MISSES1   9
#define PMU_MINION_EVENT_L2_MISS_REQ      10
#define PMU_MINION_EVENT_L2_MISS_REQ_REJ  11
#define PMU_MINION_EVENT_L2_EVICT_REQ     12
#define PMU_MINION_EVENT_L2_EVICT_REQ_REJ 13
#define PMU_MINION_EVENT_TL_INST          14
#define PMU_MINION_EVENT_TL_OPS           15
#define PMU_MINION_EVENT_TS_INST          16
#define PMU_MINION_EVENT_TS_OPS           17
#define PMU_MINION_EVENT_TFMA_WAIT_TENB   18
#define PMU_MINION_EVENT_TIMA_OPS         19
#define PMU_MINION_EVENT_TXFMA_3216_OPS   20
#define PMU_MINION_EVENT_TXFMA_32_OPS     21
#define PMU_MINION_EVENT_TXFMA_INT_OPS    22
#define PMU_MINION_EVENT_TRANS_OPS        23
#define PMU_MINION_EVENT_SHORT_OPS        24
#define PMU_MINION_EVENT_MASK_OPS         25
#define PMU_MINION_EVENT_TFMA_INST        26
#define PMU_MINION_EVENT_TREDUCE_INST     27
#define PMU_MINION_EVENT_TQUANT_INST      28

} // namespace bemu

#endif // BEMU_DEFINES_H
