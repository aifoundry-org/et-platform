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

#include "etsoc_ddr_region_map.h"

// clang-format off

/* Aligns an address to the next 64-byte cache line */
#define CACHE_LINE_ALIGN(x)        (((x + 63U) / 64U) * 64U)

/* Architectural parameters */
#define MASTER_SHIRE               32
#define NUM_SHIRES                 33
#define NUM_MASTER_SHIRES          1
#define NUM_COMPUTE_SHIRES         (NUM_SHIRES - NUM_MASTER_SHIRES)
#define HARTS_PER_SHIRE            64
#define NUM_HARTS                  (NUM_SHIRES * HARTS_PER_SHIRE)
#define MASTER_SHIRE_COMPUTE_HARTS 32
#define MAX_SIMULTANEOUS_KERNELS   4
#define CM_HART_COUNT              (NUM_COMPUTE_SHIRES * HARTS_PER_SHIRE + MASTER_SHIRE_COMPUTE_HARTS)

/* MM-CM messages related defines */
#define MESSAGE_FLAG_SIZE            8
#define MESSAGE_BUFFER_SIZE         64
#define BROADCAST_MESSAGE_CTRL_SIZE 64
#define CM_MM_IFACE_CIRCBUFFER_SIZE 4096
#define CM_MM_MESSAGE_COUNTER_SIZE  64
#define KERNEL_LAUNCH_FLAG_SIZE     64

/* FW trace related defines */
#define TRACE_CB_MAX_SIZE           64

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
/*                    (Base Address: 0x80000000)                     */
/*     - user                - base-offset   - size                  */
/*     CM Unicast buff       0x0             0x5000 (20K)            */
/*     CM Unicast locks      0x5000          0x140 (320 bytes)       */
/*     CM Kernel flags       0x5140          0x100 (256 bytes)       */
/*     MM SQ prefetch buffer 0x5200          0x2400 (9K)             */
/*********************************************************************/
#define CM_MM_IFACE_UNICAST_CIRCBUFFERS_BASE_OFFSET  0x0
#define CM_MM_IFACE_UNICAST_CIRCBUFFERS_BASE_ADDR    ETSOC_SCP_GET_SHIRE_ADDR(MASTER_SHIRE, CM_MM_IFACE_UNICAST_CIRCBUFFERS_BASE_OFFSET)
#define CM_MM_IFACE_UNICAST_CIRCBUFFERS_SIZE         ((1 + MAX_SIMULTANEOUS_KERNELS) * CM_MM_IFACE_CIRCBUFFER_SIZE)

#define CM_MM_IFACE_UNICAST_LOCKS_BASE_OFFSET        CM_MM_IFACE_UNICAST_CIRCBUFFERS_SIZE
#define CM_MM_IFACE_UNICAST_LOCKS_BASE_ADDR          ETSOC_SCP_GET_SHIRE_ADDR(MASTER_SHIRE, CM_MM_IFACE_UNICAST_LOCKS_BASE_OFFSET)
#define CM_MM_IFACE_UNICAST_LOCKS_SIZE               ((1 + MAX_SIMULTANEOUS_KERNELS) * 64) /* Slot 0 is for Thread 0 (Dispatcher), rest for Kernel Workers */

#define CM_KERNEL_LAUNCHED_FLAG_BASE_OFFSET          CM_MM_IFACE_UNICAST_LOCKS_SIZE
#define CM_KERNEL_LAUNCHED_FLAG_BASEADDR             ETSOC_SCP_GET_SHIRE_ADDR(MASTER_SHIRE, CM_KERNEL_LAUNCHED_FLAG_BASE_OFFSET)
#define CM_KERNEL_LAUNCHED_FLAG_BASEADDR_SIZE        (MAX_SIMULTANEOUS_KERNELS * KERNEL_LAUNCH_FLAG_SIZE)

#define MM_SQ_PREFETCHED_BUFFER_BASE_OFFSET          (CM_KERNEL_LAUNCHED_FLAG_BASE_OFFSET + CM_KERNEL_LAUNCHED_FLAG_BASEADDR_SIZE)
#define MM_SQ_PREFETCHED_BUFFER_BASEADDR             ETSOC_SCP_GET_SHIRE_ADDR(MASTER_SHIRE, MM_SQ_PREFETCHED_BUFFER_BASE_OFFSET)
#define MM_SQ_PREFETCHED_BUFFER_SIZE                 (MM_SQ_COUNT_MAX * MM_SQ_SIZE_MAX)

/*****************************************************************/
/*              - Low MCODE Region Layout (2M) -                 */
/*                (Base Address: 0x8000000000)                   */
/*     - user            - base-offset   - size                  */
/*     Machine FW Entry  0x1000          0x1FF000 (2044K)        */
/*****************************************************************/
 /* Default reset vector */
#define FW_MACHINE_MMODE_ENTRY              (LOW_MCODE_SUBREGION_BASE + 0x1000ULL)

/*****************************************************************/
/*              - Low MDATA Region Layout (6M) -                 */
/*                (Base Address: 0x8000200000)                   */
/*     - user            - base-offset   - size                  */
/*     Machine FW data   0x0             0x600000 (6M)           */
/*****************************************************************/
#define FW_MMODE_STACK_BASE                 (LOW_MDATA_SUBREGION_BASE + LOW_MDATA_SUBREGION_SIZE)
/* Used by trap handler. 64B is the offset to distribute stack bases across memory controllers. */
#define FW_MMODE_STACK_SCRATCH_REGION_SIZE  64
/* Large enough for re-entrant M-mode traps, but small enough so all 2112 stacks + data fit in 6MB region */
#define FW_MMODE_STACK_SIZE                 (1024 + FW_MMODE_STACK_SCRATCH_REGION_SIZE)

/*****************************************************************/
/*              - Low SCODE Region Layout (8M) -                 */
/*                (Base Address: 0x8000800000)                   */
/*     - user            - base-offset   - size                  */
/*     Master FW Entry   0x0             0x400000 (4M)           */
/*     Worker FW Entry   0x1000000       0x400000 (4M)           */
/*****************************************************************/
#define FW_MASTER_SMODE_ENTRY               LOW_SCODE_SUBREGION_BASE
#define FW_WORKER_SMODE_ENTRY               (LOW_SCODE_SUBREGION_BASE + 0x400000ULL) /* SCODE + 4M */

/*****************************************************************/
/*              - Low SDATA Region Layout (48M) -                */
/*                (Base Address: 0x8001000000)                   */
/*     - user            - base-offset   - size                  */
/*     Master FW data    0x0             0x1000000 (16M)         */
/*     Worker FW data    0x1000000       0x1000000 (16M)         */
/*     Master FW shared  0x2000000       0x1000000 (16M)         */
/*****************************************************************/
/* SDATA is divded into 3 16MB regions: Master data, worker data,
and Minion FW boot config + FW MM-CM shared Sdata (shared message buffers, etc) + S-stacks.
Shared message buffers placed at the beginning of 3rd region
S-mode stacks grow from the end of the 3rd region */
/* SDATA_REGION_BASE + 32M */
#define FW_MINION_FW_BOOT_CONFIG                          CACHE_LINE_ALIGN(LOW_SDATA_SUBREGION_BASE + 0x2000000)
#define FW_MINION_FW_BOOT_CONFIG_SIZE                     64

#define FW_GLOBAL_UART_LOCK_ADDR                          CACHE_LINE_ALIGN(FW_MINION_FW_BOOT_CONFIG + FW_MINION_FW_BOOT_CONFIG_SIZE)
#define FW_GLOBAL_UART_LOCK_SIZE                          64

#define FW_MASTER_TO_WORKER_BROADCAST_MESSAGE_BUFFER      CACHE_LINE_ALIGN(FW_GLOBAL_UART_LOCK_ADDR + FW_GLOBAL_UART_LOCK_SIZE)
#define FW_MASTER_TO_WORKER_BROADCAST_MESSAGE_BUFFER_SIZE MESSAGE_BUFFER_SIZE

#define FW_MASTER_TO_WORKER_BROADCAST_MESSAGE_CTRL        CACHE_LINE_ALIGN(FW_MASTER_TO_WORKER_BROADCAST_MESSAGE_BUFFER + FW_MASTER_TO_WORKER_BROADCAST_MESSAGE_BUFFER_SIZE)
#define FW_MASTER_TO_WORKER_BROADCAST_MESSAGE_CTRL_SIZE   BROADCAST_MESSAGE_CTRL_SIZE

#define FW_CM_TRACE_CB_BASEADDR                           CACHE_LINE_ALIGN(FW_MASTER_TO_WORKER_BROADCAST_MESSAGE_CTRL + FW_MASTER_TO_WORKER_BROADCAST_MESSAGE_CTRL_SIZE)
#define FW_CM_TRACE_CB_SIZE                               (TRACE_CB_MAX_SIZE * CM_HART_COUNT)

#define CM_MM_HART_MESSAGE_COUNTER                        CACHE_LINE_ALIGN(FW_CM_TRACE_CB_BASEADDR + FW_CM_TRACE_CB_SIZE)
#define CM_MM_HART_MESSAGE_COUNTER_SIZE                   (CM_MM_MESSAGE_COUNTER_SIZE * NUM_HARTS)

#define FW_SMODE_STACK_BASE                               (LOW_SDATA_SUBREGION_BASE + LOW_SDATA_SUBREGION_SIZE)
#define FW_SMODE_STACK_SCRATCH_REGION_SIZE                64 /* Used by trap handler. 64B is the offset to distribute stack bases across memory controllers. */
#define FW_SMODE_STACK_SIZE                               (4096 + FW_SMODE_STACK_SCRATCH_REGION_SIZE) /* (4K + 64B) stack * 2112 stacks = 8580KB */

/*****************************************************************/
/*              - Low OS Region Layout (4032M) -                 */
/*                (Base Address: 0x8004000000)                   */
/*     - user            - base-offset   - size                  */
/*     U-mode stacks     0x0             0x1000000 (16M)         */
/*     DMA Linked Lists  0x1000000       0x4000000 (64M)         */
/*****************************************************************/
/* Give 4K for VM stack pages
Bits[8:6] of an address specify memshire number, and bit[9] the controller within memshire.
Offset by 1<<6 = 64 to distribute stack bases across different memory controllers */
#define KERNEL_UMODE_STACK_BASE           0x8005000000ULL
#define KERNEL_UMODE_STACK_SIZE           (4096 + 64)

/* Storage for DMA configuration linked lists. Store at the end of U-mode stack base
For each DMA channel, reserve 8MB for the list. Chosen arbitrarily to balance mem
useage vs likely need. Each entry is 24 bytes, so 349525 entries max per list. If the
host system is totally fragmented (each entry tracks 4kB - VERY unlkely), can DMA
~1.4GB without modifying the list. */
#define DMA_LL_SIZE                       0x800000
#define DMA_CHAN_READ_0_LL_BASE           KERNEL_UMODE_STACK_BASE
#define DMA_CHAN_READ_1_LL_BASE           CACHE_LINE_ALIGN(DMA_CHAN_READ_0_LL_BASE + DMA_LL_SIZE)
#define DMA_CHAN_READ_2_LL_BASE           CACHE_LINE_ALIGN(DMA_CHAN_READ_1_LL_BASE + DMA_LL_SIZE)
#define DMA_CHAN_READ_3_LL_BASE           CACHE_LINE_ALIGN(DMA_CHAN_READ_2_LL_BASE + DMA_LL_SIZE)
#define DMA_CHAN_WRITE_0_LL_BASE          CACHE_LINE_ALIGN(DMA_CHAN_READ_3_LL_BASE + DMA_LL_SIZE)
#define DMA_CHAN_WRITE_1_LL_BASE          CACHE_LINE_ALIGN(DMA_CHAN_WRITE_0_LL_BASE + DMA_LL_SIZE)
#define DMA_CHAN_WRITE_2_LL_BASE          CACHE_LINE_ALIGN(DMA_CHAN_WRITE_1_LL_BASE + DMA_LL_SIZE)
#define DMA_CHAN_WRITE_3_LL_BASE          CACHE_LINE_ALIGN(DMA_CHAN_WRITE_2_LL_BASE + DMA_LL_SIZE)

/*****************************************************************/
/*              - Low Memory Region Layout (28G) -               */
/*                (Base Address: 0x8100000000)                   */
/*     - user            - base-offset   - size                  */
/*     Sub-regions       0x0             0x1000000 (16M)         */
/*     Host-managed      0x1000000       0x2FF000000 (12272M)    */
/*     Not-used          0x300000000     0x400000000 (16G)       */
/* NOTE: "Not-used" region is accessible but not used currently. */
/*****************************************************************/
/* Expose whole DRAM low memory region to the host via BAR0. */
#define DRAM_MEMMAP_BEGIN                 LOW_MEMORY_SUBREGION_BASE
#define DRAM_MEMMAP_END                   (LOW_MEMORY_SUBREGION_BASE + LOW_MEMORY_SUBREGION_SIZE - 1)
#define DRAM_MEMMAP_SIZE                  (DRAM_MEMMAP_END - DRAM_MEMMAP_BEGIN + 1)

/* This range is used as Scratch area as storage of new FW image whilst SP updates the
correspoding Flash partition to take affect. */
#define SP_DM_SCRATCH_REGION_BEGIN        (LOW_MEM_SUB_REGIONS_BASE + 0x0)
#define SP_DM_SCRATCH_REGION_SIZE         0x400000U

/* Trace buffers for Service Processor, Master Minion, and Compute Minion are consecutive
   memory regions in DDR User Memory Sub Region, starting from (BAR0 + 0x400000) with sizes
   4KB, 1MB, and (4KB * 2080) respectively. */
/* SP DM services Trace Buffer */
#define SP_TRACE_BUFFER_BASE              (SP_DM_SCRATCH_REGION_BEGIN + SP_DM_SCRATCH_REGION_SIZE)
#define SP_TRACE_BUFFER_SIZE              0x1000 /* 4KB for SP DM services Trace Buffer */

/* Master Minion FW Trace Buffer */
#define MM_TRACE_BUFFER_BASE              (SP_TRACE_BUFFER_BASE + SP_TRACE_BUFFER_SIZE)
#define MM_TRACE_BUFFER_SIZE              0x100000 /* 1MB for Master Minion Trace Buffer */

/* Compute Minion FW Trace Buffer */
#define CM_TRACE_BUFFER_BASE              (MM_TRACE_BUFFER_BASE + MM_TRACE_BUFFER_SIZE)
/* Default 4KB fixed buffer size per Hart for all Compute Worker Harts. It must be 64 byte aligned. */
#define CM_TRACE_BUFFER_SIZE_PER_HART     0x1000
#define CM_TRACE_BUFFER_SIZE              (CM_TRACE_BUFFER_SIZE_PER_HART * CM_HART_COUNT)

/* CM UMode Trace config region. */
#define CM_UMODE_TRACE_CFG_BASEADDR       CACHE_LINE_ALIGN(CM_TRACE_BUFFER_BASE + CM_TRACE_BUFFER_SIZE)
#define CM_UMODE_TRACE_CFG_SIZE           64

/* NOTE: Keep it's value in sync with device-software/et-common-libs/src/trace/trace_umode.c
         This region should be in non-Host managed UMode region. */
#define CM_UMODE_TRACE_CB_BASEADDR        CACHE_LINE_ALIGN(CM_UMODE_TRACE_CFG_BASEADDR + CM_UMODE_TRACE_CFG_SIZE)
#define CM_UMODE_TRACE_CB_SIZE            (TRACE_CB_MAX_SIZE * CM_HART_COUNT)

/* Reserved area for DDR low memory sub regions */
#define LOW_MEM_SUB_REGIONS_BASE          LOW_MEMORY_SUBREGION_BASE
#define LOW_MEM_SUB_REGIONS_SIZE          0x0001000000ULL

/* U-mode user kernels entry point
(Fixed address - should sync kernels linker script if this is changed) */
#define KERNEL_UMODE_ENTRY                (LOW_MEM_SUB_REGIONS_BASE + LOW_MEM_SUB_REGIONS_SIZE)

/* Define the address range in DRAM that the host runtime can explicitly manage
the range is the START to (END-1) */
#define HOST_MANAGED_DRAM_START           KERNEL_UMODE_ENTRY
#define HOST_MANAGED_DRAM_END             0x8400000000ULL
#define HOST_MANAGED_DRAM_SIZE            (HOST_MANAGED_DRAM_END - HOST_MANAGED_DRAM_START)

/************************/
/* Compile-time checks  */
/************************/
#ifndef __ASSEMBLER__

/* Ensure the shared message buffers don't overlap with the S-stacks */
static_assert((CM_MM_HART_MESSAGE_COUNTER + CM_MM_HART_MESSAGE_COUNTER_SIZE) <
              (FW_SMODE_STACK_BASE - (NUM_HARTS * FW_SMODE_STACK_SIZE)),
              "S-stack / message buffer region collision");

/* Ensure that DDR low memory sub regions dont cross the define limit */
static_assert((CM_UMODE_TRACE_CB_BASEADDR + CM_UMODE_TRACE_CB_SIZE) < (LOW_MEM_SUB_REGIONS_BASE + LOW_MEM_SUB_REGIONS_SIZE),
              "DDR low memory sub regions crossing limits");

/* Ensure that DDR low memory regions dont overlap U-mode kernels entry */
static_assert(((LOW_MEM_SUB_REGIONS_BASE + LOW_MEM_SUB_REGIONS_SIZE) - 1) < KERNEL_UMODE_ENTRY,
              "DDR low memory / Kernel U-mode entry region collision");

#endif /* __ASSEMBLER__ */

#endif /* _LAYOUT_H_ */
