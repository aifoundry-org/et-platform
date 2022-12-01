/***********************************************************************
*
* Copyright (C) 2020 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*
************************************************************************/
/***********************************************************************/
/*! \file layout.h
    \brief A C header that defines the ET-SOC SW memory layout.
*/
/***********************************************************************/
#ifndef _LAYOUT_H_
#define _LAYOUT_H_

#ifndef __ASSEMBLER__
#include <assert.h>
#endif

#include "system/etsoc_ddr_region_map.h"

/* Turn of clang format for this file. It is more readable that way. */
// clang-format off

/* Aligns an address to the next 64-byte cache line */
#define ALIGN_64B(x)               (((x + 0x3FULL) / 0x40ULL) * 0x40ULL)
#define ALIGN_4K(x)                (((x + 0xFFFULL) / 0x1000ULL) * 0x1000ULL)
#define SIZE_8B                    0x8
#define SIZE_32B                   0x20
#define SIZE_64B                   0x40
#define SIZE_1KB                   0x400
#define SIZE_4KB                   0x1000
#define SIZE_1MB                   0x100000
#define SIZE_4MB                   0x400000
#define SIZE_8MB                   0x800000

/* Architectural parameters */
#define MASTER_SHIRE               32
#define NUM_SHIRES                 33
#define NUM_MEM_SHIRES             8
#define NUM_MASTER_SHIRES          1
#define NUM_COMPUTE_SHIRES         (NUM_SHIRES - NUM_MASTER_SHIRES)
#define HARTS_PER_SHIRE            64
#define NUM_HARTS                  (NUM_SHIRES * HARTS_PER_SHIRE)
#define NEIGH_PER_SHIRE            4
#define MASTER_SHIRE_COMPUTE_HARTS 32
#define MAX_SIMULTANEOUS_KERNELS   4
#define CM_HART_COUNT              (NUM_COMPUTE_SHIRES * HARTS_PER_SHIRE + MASTER_SHIRE_COMPUTE_HARTS)

/* MM-CM messages related defines */
#define MESSAGE_FLAG_SIZE           SIZE_8B
#define MESSAGE_BUFFER_SIZE         SIZE_64B
#define BROADCAST_MESSAGE_CTRL_SIZE SIZE_64B
#define CM_MM_IFACE_CIRCBUFFER_SIZE SIZE_4KB
#define CM_MM_MESSAGE_COUNTER_SIZE  SIZE_64B
#define KERNEL_LAUNCH_FLAG_SIZE     SIZE_64B

/* FW trace related defines */
#define TRACE_CB_MAX_SIZE           SIZE_64B
/* 4KB for SP DM services Trace Buffer + 4KB for Exception Trace buffer. */
#define SP_TRACE_SUB_BUFFER_COUNT   2

#define SP_DEV_STATS_TRACE_SUB_BUFFER_COUNT   1

/* MM VQs related defines */
#define MM_SQ_COUNT_MAX             4
#define MM_SQ_SIZE_MAX              0x900

/* SCP related defines */
/* TODO: SW-8195: See if these values can come from HAL */
#define ETSOC_SCP_REGION_BASEADDR                   0x80000000ULL
#define ETSOC_SCP_GET_SHIRE_OFFSET(scp_addr)        (scp_addr & 0x7FFFFFULL)
#define ETSOC_SCP_GET_SHIRE_SIZE                    0x280000 /* 2.5 MB */
#define ETSOC_SCP_GET_SHIRE_ID(scp_addr)            ((scp_addr >> 23) & 0x7FULL)
#define ETSOC_SCP_GET_SHIRE_ADDR(shire_id, offset)  (((shire_id << 23) & 0x3F800000ULL) + \
                                                    ETSOC_SCP_REGION_BASEADDR + offset)

/*********************************************************************/
/*              - Shire 32 L2 SCP Region Layout (2.5M) -             */
/*                    (Base Address: 0x90000000)                     */
/*     - user                    - base-offset   - size              */
/*     CM Unicast buff           0x0             0x5000 (20K)        */
/*     CM Unicast locks          0x5000          0x140 (320 bytes)   */
/*     CM Kernel flags           0x5140          0x100 (256 bytes)   */
/*     MM SQ prefetch buffer     0x5240          0x2400 (9K)         */
/*     Broadcast Message Buffer  0x7640          0x40 (64B)          */
/*     Broadcast Message Control 0x7680          0x40 (64B)          */
/*     CM shires boot mask       0x76C0          0x40 (64B)          */
/*********************************************************************/
#define CM_MM_IFACE_UNICAST_CIRCBUFFERS_BASE_OFFSET  0x0
#define CM_MM_IFACE_UNICAST_CIRCBUFFERS_BASE_ADDR    ETSOC_SCP_GET_SHIRE_ADDR(MASTER_SHIRE, CM_MM_IFACE_UNICAST_CIRCBUFFERS_BASE_OFFSET)
#define CM_MM_IFACE_UNICAST_CIRCBUFFERS_SIZE         ((1 + MAX_SIMULTANEOUS_KERNELS) * CM_MM_IFACE_CIRCBUFFER_SIZE)

#define CM_MM_IFACE_UNICAST_LOCKS_BASE_OFFSET        CM_MM_IFACE_UNICAST_CIRCBUFFERS_SIZE
#define CM_MM_IFACE_UNICAST_LOCKS_BASE_ADDR          ETSOC_SCP_GET_SHIRE_ADDR(MASTER_SHIRE, CM_MM_IFACE_UNICAST_LOCKS_BASE_OFFSET)
#define CM_MM_IFACE_UNICAST_LOCKS_SIZE               ((1 + MAX_SIMULTANEOUS_KERNELS) * 64) /* Slot 0 is for Thread 0 (Dispatcher), rest for Kernel Workers */

#define CM_KERNEL_LAUNCHED_FLAG_BASE_OFFSET          (CM_MM_IFACE_UNICAST_LOCKS_BASE_OFFSET + CM_MM_IFACE_UNICAST_LOCKS_SIZE)
#define CM_KERNEL_LAUNCHED_FLAG_BASEADDR             ETSOC_SCP_GET_SHIRE_ADDR(MASTER_SHIRE, CM_KERNEL_LAUNCHED_FLAG_BASE_OFFSET)
#define CM_KERNEL_LAUNCHED_FLAG_BASEADDR_SIZE        (MAX_SIMULTANEOUS_KERNELS * KERNEL_LAUNCH_FLAG_SIZE)

#define FW_GLOBAL_UART_LOCK_BASE_OFFSET              (CM_KERNEL_LAUNCHED_FLAG_BASE_OFFSET + CM_KERNEL_LAUNCHED_FLAG_BASEADDR_SIZE)
#define FW_GLOBAL_UART_LOCK_ADDR                     ETSOC_SCP_GET_SHIRE_ADDR(MASTER_SHIRE, FW_GLOBAL_UART_LOCK_BASE_OFFSET)
#define FW_GLOBAL_UART_LOCK_SIZE                     64

#define MM_SQ_PREFETCHED_BUFFER_BASE_OFFSET          (FW_GLOBAL_UART_LOCK_BASE_OFFSET + FW_GLOBAL_UART_LOCK_SIZE)
#define MM_SQ_PREFETCHED_BUFFER_BASEADDR             ETSOC_SCP_GET_SHIRE_ADDR(MASTER_SHIRE, MM_SQ_PREFETCHED_BUFFER_BASE_OFFSET)
#define MM_SQ_PREFETCHED_BUFFER_SIZE                 (MM_SQ_COUNT_MAX * MM_SQ_SIZE_MAX)

/* Master Minion to Worker Minion Broadcat message buffer. */
#define FW_MASTER_TO_WORKER_BROADCAST_MESSAGE_BUFFER_OFFSET (MM_SQ_PREFETCHED_BUFFER_BASE_OFFSET + MM_SQ_PREFETCHED_BUFFER_SIZE)
#define FW_MASTER_TO_WORKER_BROADCAST_MESSAGE_BUFFER        ETSOC_SCP_GET_SHIRE_ADDR(MASTER_SHIRE, FW_MASTER_TO_WORKER_BROADCAST_MESSAGE_BUFFER_OFFSET)
#define FW_MASTER_TO_WORKER_BROADCAST_MESSAGE_BUFFER_SIZE   MESSAGE_BUFFER_SIZE

/* Master Minion to Worker Minion Broadcat message control. */
#define FW_MASTER_TO_WORKER_BROADCAST_MESSAGE_CTRL_OFFSET   (FW_MASTER_TO_WORKER_BROADCAST_MESSAGE_BUFFER_OFFSET + FW_MASTER_TO_WORKER_BROADCAST_MESSAGE_BUFFER_SIZE)
#define FW_MASTER_TO_WORKER_BROADCAST_MESSAGE_CTRL          ETSOC_SCP_GET_SHIRE_ADDR(MASTER_SHIRE, FW_MASTER_TO_WORKER_BROADCAST_MESSAGE_CTRL_OFFSET)
#define FW_MASTER_TO_WORKER_BROADCAST_MESSAGE_CTRL_SIZE     BROADCAST_MESSAGE_CTRL_SIZE

#define CM_SHIRES_BOOT_MASK_OFFSET                          (FW_MASTER_TO_WORKER_BROADCAST_MESSAGE_CTRL_OFFSET + FW_MASTER_TO_WORKER_BROADCAST_MESSAGE_CTRL_SIZE)
#define CM_SHIRES_BOOT_MASK_BASEADDR                        ETSOC_SCP_GET_SHIRE_ADDR(MASTER_SHIRE, CM_SHIRES_BOOT_MASK_OFFSET)
#define CM_SHIRES_BOOT_MASK_SIZE                            SIZE_64B /* Occupy a single cache-line */

/*****************************************************************/
/*              - Low MCODE Region Layout (2M) -                 */
/*                (Base Address: 0x8000000000)                   */
/*     - user            - base-offset   - size                  */
/*     Machine FW Entry  0x1000          0x1FF000 (2044K)        */
/*****************************************************************/
 /* Default reset vector */
#define FW_MACHINE_MMODE_ENTRY              (LOW_MCODE_SUBREGION_BASE + SIZE_4KB)

/*****************************************************************/
/*              - Low MDATA Region Layout (6M) -                 */
/*                (Base Address: 0x8000200000)                   */
/*     - user            - base-offset   - size                  */
/*     Machine FW data   0x0             0x600000 (6M)           */
/*****************************************************************/
#define FW_MMODE_STACK_BASE                 (LOW_MDATA_SUBREGION_BASE + LOW_MDATA_SUBREGION_SIZE)
/* Used by trap handler. 64B is the offset to distribute stack bases across memory controllers. */
#define FW_MMODE_STACK_SCRATCH_REGION_SIZE  SIZE_64B
/* Large enough for re-entrant M-mode traps, but small enough so all 2112 stacks + data fit in 6MB region */
#define FW_MMODE_STACK_SIZE                 (SIZE_1KB + FW_MMODE_STACK_SCRATCH_REGION_SIZE)

/*****************************************************************/
/*              - Low SCODE Region Layout (8M) -                 */
/*                (Base Address: 0x8000800000)                   */
/*     - user            - base-offset   - size                  */
/*     Master FW Entry   0x0             0x400000 (4M)           */
/*     Worker FW Entry   0x1000000       0x400000 (4M)           */
/*****************************************************************/
#define FW_MASTER_SMODE_ENTRY               LOW_SCODE_SUBREGION_BASE
#define FW_WORKER_SMODE_ENTRY               (LOW_SCODE_SUBREGION_BASE + SIZE_4MB) /* SCODE + 4M */

/****************************************************************************/
/*              - Low SDATA Region Layout (48M) -                           */
/*                (Base Address: 0x8001000000)                              */
/*     - user                   - base-offset   - size                      */
/*     Master FW sdata           0x8001000000    0x400000  (4M)      - BAR  */
/*     Worker FW sdata           0x8001400000    0x400000  (4M)      NA     */
/*     CM-MM Message counter     0x8001800000    0x21000   (132K)    NA     */
/*     CM SMode Trace CB         0x8001821000    0x20800   (130K)    NA     */
/*     CM UMode Trace Config     0x8001841800    0x40      (64B)     NA     */
/*     UNUSED                    0x8001841840    0x1F5C7C0 (~31.36M) NA     */
/*     SMODE Stacks end          0x800379E000    0x861000  (8580K)   NA     */
/*     SMODE Stacks base         0x8003FFF000                        NA     */
/****************************************************************************/

/* Master Minion FW SData region.
   (WARNING: Fixed address - should sync with MM FW linker script) */
#define FW_MASTER_SDATA_BASE                                LOW_SDATA_SUBREGION_BASE
#define FW_MASTER_SDATA_SIZE                                SIZE_4MB

/* Worker Minion FW SData region.
   (WARNING: Fixed address - should sync with CM FW linker script) */
#define FW_WORKER_SDATA_BASE                                (FW_MASTER_SDATA_BASE + FW_MASTER_SDATA_SIZE)
#define FW_WORKER_SDATA_SIZE                                SIZE_4MB

/* Master Minion and Worker Minion message counter. */
#define CM_MM_HART_MESSAGE_COUNTER                          (FW_WORKER_SDATA_BASE + FW_WORKER_SDATA_SIZE)
#define CM_MM_HART_MESSAGE_COUNTER_SIZE                     (CM_MM_MESSAGE_COUNTER_SIZE * NUM_HARTS)

/* CM S-mode Trace control block */
#define CM_SMODE_TRACE_CB_BASEADDR                          (CM_MM_HART_MESSAGE_COUNTER + CM_MM_HART_MESSAGE_COUNTER_SIZE)
#define CM_SMODE_TRACE_CB_SIZE                              (TRACE_CB_MAX_SIZE * CM_HART_COUNT)

/* CM U-mode Trace config region. */
#define CM_UMODE_TRACE_CFG_BASEADDR                         (CM_SMODE_TRACE_CB_BASEADDR + CM_SMODE_TRACE_CB_SIZE)
#define CM_UMODE_TRACE_CFG_SIZE                             SIZE_64B

/* Stack grows downward, so start from end of the region. */
#define FW_SMODE_STACK_BASE                                 (LOW_SDATA_SUBREGION_BASE + LOW_SDATA_SUBREGION_SIZE - SIZE_4KB)
#define FW_SMODE_STACK_SCRATCH_REGION_SIZE                  SIZE_64B /* Used by trap handler. 64B is the offset to distribute stack bases across memory controllers. */
#define FW_SMODE_STACK_SIZE                                 (SIZE_4KB + FW_SMODE_STACK_SCRATCH_REGION_SIZE) /* (4K + 64B) stack * 2112 stacks = 8580KB */
#define FW_SMODE_STACK_END                                  (FW_SMODE_STACK_BASE - (FW_SMODE_STACK_SIZE * NUM_HARTS))

/****************************************************************************/
/*              - Low OS Region Layout (4032M) -                            */
/*                (Base Address: 0x8004000000)                              */
/*     - user                    - base          - size              - BAR  */
/*     DMA Linked Lists          0x8004000000    0x800     (2K)      NA     */
/*     Scratch (4K aligned)      0x8004001000    0x400000  (4M)      0      */
/*     SP BL2 FW Trace           0x8004401000    0x2000    (8K)      0      */
/*     MM SMode Trace            0x8004403000    0x100000  (1M)      0      */
/*     CM SMode Trace            0x8004503000    0x820000  (8.125M)  0      */
/*     SP Dev Stats Trace        0x8004D23000    0x100000  (1.0 MB)  0      */
/*     MM Dev Stats Trace        0x8004E23000    0x100000  (1.0 MB)  0      */
/*     CM UMode Trace CB (FIXED) 0x8004F23000    0x20800   (130K)    NA     */
/*     UNUSED                    0x8004F43800    0x9C800   (626K)    NA     */
/*     UMode stacks end          0x8004FBF800    0x840800  ~(8.25M)  NA     */
/*     UMode stacks base         0x8005800000                        NA     */
/*     UNUSED                    0x8005800000    0x1000    (4K)      NA     */
/*     Kernel UMode Entry(FIXED) 0x8005801000    until DRAM End      NA     */
/*     / Host Managed DRAM                                                  */
/*                                                                          */
/*              - Low Memory Sub-Region Layout (28G) -                      */
/*                (Base Address: 0x8100000000)                              */
/*     - user                    - base          - size              - BAR  */
/*     Host Managed DRAM         0x8100000000    until region Ends     NA   */
/****************************************************************************/

/* Storage for DMA configuration linked lists. Store at the end of U-mode stack base + 4K offset.
   For each DMA channel. Each link-list can have maximum DMA_MAX_ENTRIES_PER_LL number of entries.
   And per entry size is 24 bytes. Memory is reserved dynamically based on number of entries per link-list */
#define DMA_LL_BASE                             LOW_OS_SUBREGION_BASE
#define DMA_PER_ENTRY_SIZE                      24U
#define DMA_MAX_ENTRIES_PER_LL                  8U
#define DMA_LL_SIZE                             ALIGN_64B((DMA_MAX_ENTRIES_PER_LL + 1U /* Link entry*/) * DMA_PER_ENTRY_SIZE)

/* Read Chan 0 LL Base = 0x8004000000 */
#define DMA_CHAN_READ_0_LL_BASE                 DMA_LL_BASE
/* Read Chan 1 LL Base = 0x8004000100 */
#define DMA_CHAN_READ_1_LL_BASE                 (DMA_CHAN_READ_0_LL_BASE + DMA_LL_SIZE)
/* Read Chan 2 LL Base = 0x8004000200 */
#define DMA_CHAN_READ_2_LL_BASE                 (DMA_CHAN_READ_1_LL_BASE + DMA_LL_SIZE)
/* Read Chan 3 LL Base = 0x8004000300 */
#define DMA_CHAN_READ_3_LL_BASE                 (DMA_CHAN_READ_2_LL_BASE + DMA_LL_SIZE)
/* Write Chan 0 LL Base = 0x8004000400 */
#define DMA_CHAN_WRITE_0_LL_BASE                (DMA_CHAN_READ_3_LL_BASE + DMA_LL_SIZE)
/* Write Chan 1 LL Base = 0x8004000500 */
#define DMA_CHAN_WRITE_1_LL_BASE                (DMA_CHAN_WRITE_0_LL_BASE + DMA_LL_SIZE)
/* Write Chan 2 LL Base = 0x8004000600 */
#define DMA_CHAN_WRITE_2_LL_BASE                (DMA_CHAN_WRITE_1_LL_BASE + DMA_LL_SIZE)
/* Write Chan 3  LL Base = 0x8004000700 */
#define DMA_CHAN_WRITE_3_LL_BASE                (DMA_CHAN_WRITE_2_LL_BASE + DMA_LL_SIZE)

/* Expose whole DRAM low memory region and portion of LOW OS region used as DRAM to the host via BAR0. */
#define DRAM_MEMMAP_BEGIN                       ALIGN_4K(DMA_CHAN_WRITE_3_LL_BASE + DMA_LL_SIZE)
/* Low memory region plus portion of LOW OS region used as DRAM. */
#define DRAM_MEMMAP_SIZE                        (LOW_MEMORY_SUBREGION_SIZE + DRAM_MEMMAP_BEGIN - LOW_OS_SUBREGION_BASE)
#define DRAM_MEMMAP_END                         (DRAM_MEMMAP_BEGIN + DRAM_MEMMAP_SIZE - 1)

/* This range is used as Scratch area as storage of new FW image whilst SP updates the
correspoding Flash partition to take affect. */
#define SP_DM_SCRATCH_REGION_BEGIN              DRAM_MEMMAP_BEGIN
#define SP_DM_SCRATCH_REGION_SIZE               SIZE_4MB

/* Trace buffers for Service Processor, Master Minion, and Compute Minion are consecutive
   memory regions in DDR User Memory Sub Region, starting from SP_TRACE_BUFFER_BASE with sizes
   4KB, 1MB, and 8.125MB respectively. */
/* SP DM services Trace Buffer */
#define SP_TRACE_BUFFER_BASE                    (SP_DM_SCRATCH_REGION_BEGIN + SP_DM_SCRATCH_REGION_SIZE)
#define SP_TRACE_BUFFER_SIZE_PER_SUB_BUFFER     SIZE_4KB
#define SP_TRACE_BUFFER_SIZE                    (SP_TRACE_BUFFER_SIZE_PER_SUB_BUFFER * SP_TRACE_SUB_BUFFER_COUNT)

/* Master Minion FW Trace Buffer */
#define MM_TRACE_BUFFER_BASE                    (SP_TRACE_BUFFER_BASE + SP_TRACE_BUFFER_SIZE)
#define MM_TRACE_BUFFER_SIZE                    SIZE_1MB /* 1MB for Master Minion Trace Buffer */

/* Compute Minion FW Trace Buffer */
#define CM_SMODE_TRACE_BUFFER_BASE              (MM_TRACE_BUFFER_BASE + MM_TRACE_BUFFER_SIZE)
/* Default 4KB fixed buffer size per Hart for all Compute Worker Harts. It must be 64 byte aligned. */
#define CM_SMODE_TRACE_BUFFER_SIZE_PER_HART     SIZE_4KB
#define CM_SMODE_TRACE_BUFFER_SIZE              (CM_SMODE_TRACE_BUFFER_SIZE_PER_HART * CM_HART_COUNT)

/* SP Dev Stats Trace Buffer */
#define SP_STATS_TRACE_BUFFER_BASE              (CM_SMODE_TRACE_BUFFER_BASE + CM_SMODE_TRACE_BUFFER_SIZE)
#define SP_STATS_BUFFER_SIZE                    SIZE_1MB

/* MM Dev Stats Trace Buffer */
#define MM_STATS_TRACE_BUFFER_BASE              (SP_STATS_TRACE_BUFFER_BASE + SP_STATS_BUFFER_SIZE)
#define MM_STATS_BUFFER_SIZE                    SIZE_1MB

/* This region should be in non-Host managed UMode region.
(WARNING: Fixed address - should sync with UMode Trace) */
#define CM_UMODE_TRACE_CB_BASEADDR              (MM_STATS_TRACE_BUFFER_BASE + MM_STATS_BUFFER_SIZE)
#define CM_UMODE_TRACE_CB_SIZE                  (TRACE_CB_MAX_SIZE * CM_HART_COUNT)

/* Give (4K+64B) for VM stack pages. This 64 in addition to 4K used to distrube stacks for
different HARTs across different mem shires. The logic behind this is as follows.
Bits[8:6] of an address specify memshire number, and bit[9] the controller within memshire.
Offset by 1<<6 = 64 to distribute stack bases across different memory controllers */
#define KERNEL_UMODE_STACK_OFFSET               0x1800000ULL
#define KERNEL_UMODE_STACK_BASE                 (LOW_OS_SUBREGION_BASE + KERNEL_UMODE_STACK_OFFSET)
#define KERNEL_UMODE_STACK_SIZE                 (SIZE_4KB + SIZE_64B)
#define KERNEL_UMODE_STACK_END                  (KERNEL_UMODE_STACK_BASE - (KERNEL_UMODE_STACK_SIZE * CM_HART_COUNT))

/* U-mode user kernels entry point
   (WARNING: Fixed address - should sync compute kernels linker script)
   Note: Give a 4K offset from U-mode stacks so that we don't have any address collision. */
#define KERNEL_UMODE_ENTRY                      ALIGN_4K(KERNEL_UMODE_STACK_BASE + SIZE_4KB)

/* Define the address range in DRAM that the host runtime can explicitly manage
the range is the START to (END-1). */
#define HOST_MANAGED_DRAM_START                 KERNEL_UMODE_ENTRY
#define HOST_MANAGED_DRAM_SIZE_MAX              0x800000000ULL /* 32 GB */

/************************/
/* Compile-time checks  */
/************************/
#ifndef __ASSEMBLER__

/* Ensure the shared message buffers don't overlap with the S-stacks */
static_assert((CM_UMODE_TRACE_CFG_BASEADDR + CM_UMODE_TRACE_CFG_SIZE) < FW_SMODE_STACK_END,
              "S-stack / message buffer region collision");

/* Ensure that DDR low OS regions dont cross the define limit */
static_assert((CM_UMODE_TRACE_CB_BASEADDR + CM_UMODE_TRACE_CB_SIZE) < KERNEL_UMODE_STACK_END,
              "DDR OS memory sub regions crossing limits");

/* Ensure that fixed U-mode Trace CB address has not been changed. */
static_assert(CM_UMODE_TRACE_CB_BASEADDR == 0x8004F23000ULL,
              "U-mode Trace CB address is changed, it needs to be adjusted in U-mode Trace");

/* Ensure that fixed U-mode kernels entry address has not been changed. */
static_assert(KERNEL_UMODE_ENTRY == 0x8005801000ULL,
              "Kernel U-mode entry is changed, it needs to be adjusted in linker script");

#endif /* __ASSEMBLER__ */

#endif /* _LAYOUT_H_ */
