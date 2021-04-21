#ifndef LAYOUT_H
#define LAYOUT_H

#ifndef __ASSEMBLER__
#include <assert.h>
#endif

//TODO: Keep in sync with sw-platform/virtual-platform/kernel/et-pcie-driver/et_layout.h
//TODO: Single source between these two files

// aligns an address to the next 64-byte cache line
#define CACHE_LINE_ALIGN(x) (((x + 63U) / 64U) * 64U)

// Architectural parameters
#define MASTER_SHIRE             32
#define NUM_SHIRES               33
#define NUM_MASTER_SHIRES        1
#define NUM_COMPUTE_SHIRES       (NUM_SHIRES - NUM_MASTER_SHIRES)
#define HARTS_PER_SHIRE          64
#define NUM_HARTS                (NUM_SHIRES * HARTS_PER_SHIRE)
#define MAX_SIMULTANEOUS_KERNELS 4

#define MESSAGE_FLAG_SIZE            8
#define MESSAGE_BUFFER_SIZE         64
#define BROADCAST_MESSAGE_CTRL_SIZE 64
#define CM_MM_IFACE_CIRCBUFFER_SIZE 4096

// Hardware memory region layout
#define M_CODE_REGION_BASE 0x8000000000ULL /* MCODE region is  2M from 0x8000000000 to 0x80001FFFFF */
#define M_CODE_REGION_SIZE 0x0000200000ULL

#define M_DATA_REGION_BASE 0x8000200000ULL /* MDATA region is  6M from 0x8000200000 to 0x80007FFFFF */
#define M_DATA_REGION_SIZE 0x0000600000ULL

#define S_CODE_REGION_BASE 0x8000800000ULL /* SCODE region is  8M from 0x8000800000 to 0x8000FFFFFF */
#define S_CODE_REGION_SIZE 0x0000800000ULL

#define S_DATA_REGION_BASE 0x8001000000ULL /* SDATA region is 48M from 0x8001000000 to 0x8003FFFFFF */
#define S_DATA_REGION_SIZE 0x0003000000ULL

#define LOW_OS_REGION_BASE 0x8004000000ULL /* LOW OS region is 4032M from 0x8004000000 to 0x80FFFFFFFF */
#define LOW_OS_REGION_SIZE 0x00FC000000ULL

#define LOW_MEM_REGION_BASE 0x8100000000ULL /* LOW MEM region is 28G from 0x8100000000 to 0x87FFFFFFFF */
#define LOW_MEM_REGION_SIZE 0x0700000000ULL

// Software layout
#define FW_MACHINE_MMODE_ENTRY (M_CODE_REGION_BASE + 0x1000ULL) // Default reset vector
#define FW_MMODE_STACK_BASE    (M_DATA_REGION_BASE + M_DATA_REGION_SIZE)
#define FW_MMODE_STACK_SCRATCH_REGION_SIZE 64 /* Used by trap handler. 64B is the offset to distribute stack bases across memory controllers. */
#define FW_MMODE_STACK_SIZE    (1024 + FW_MMODE_STACK_SCRATCH_REGION_SIZE) // Large enough for re-entrant M-mode traps, but small enough so all 2112 stacks + data fit in 6MB region

#define FW_MASTER_SMODE_ENTRY S_CODE_REGION_BASE
#define FW_WORKER_SMODE_ENTRY (S_CODE_REGION_BASE + 0x400000ULL) // SCODE + 4M

// SDATA is divded into 3 16MB regions: Master data, worker data, and Minion FW boot config + FW MM-CM shared Sdata (shared message buffers, etc) + S-stacks.
// Shared message buffers placed at the beginning of 3rd region
// S-mode stacks grow from the end of the 3rd region

#define FW_MINION_FW_BOOT_CONFIG                          CACHE_LINE_ALIGN(S_DATA_REGION_BASE + 0x2000000) /* SDATA_REGION_BASE + 32M */
#define FW_MINION_FW_BOOT_CONFIG_SIZE                     64

#define FW_GLOBAL_UART_LOCK_ADDR                          CACHE_LINE_ALIGN(FW_MINION_FW_BOOT_CONFIG + FW_MINION_FW_BOOT_CONFIG_SIZE)
#define FW_GLOBAL_UART_LOCK_SIZE                          64

#define FW_MASTER_TO_WORKER_BROADCAST_MESSAGE_BUFFER      CACHE_LINE_ALIGN(FW_GLOBAL_UART_LOCK_ADDR + FW_GLOBAL_UART_LOCK_SIZE)
#define FW_MASTER_TO_WORKER_BROADCAST_MESSAGE_BUFFER_SIZE MESSAGE_BUFFER_SIZE

#define FW_MASTER_TO_WORKER_BROADCAST_MESSAGE_CTRL        CACHE_LINE_ALIGN(FW_MASTER_TO_WORKER_BROADCAST_MESSAGE_BUFFER + FW_MASTER_TO_WORKER_BROADCAST_MESSAGE_BUFFER_SIZE)
#define FW_MASTER_TO_WORKER_BROADCAST_MESSAGE_CTRL_SIZE   BROADCAST_MESSAGE_CTRL_SIZE

#define CM_MM_IFACE_UNICAST_CIRCBUFFERS_BASE_ADDR         CACHE_LINE_ALIGN(FW_MASTER_TO_WORKER_BROADCAST_MESSAGE_CTRL + FW_MASTER_TO_WORKER_BROADCAST_MESSAGE_CTRL_SIZE)
#define CM_MM_IFACE_UNICAST_CIRCBUFFERS_SIZE              ((1 + MAX_SIMULTANEOUS_KERNELS) * CM_MM_IFACE_CIRCBUFFER_SIZE) // Slot 0 is for Thread 0 (Dispatcher), rest for Kernel Workers

#define CM_MM_IFACE_UNICAST_LOCKS_BASE_ADDR               CACHE_LINE_ALIGN(CM_MM_IFACE_UNICAST_CIRCBUFFERS_BASE_ADDR + CM_MM_IFACE_UNICAST_CIRCBUFFERS_SIZE)
#define CM_MM_IFACE_UNICAST_LOCKS_SIZE                    ((1 + MAX_SIMULTANEOUS_KERNELS) * 64) // Slot 0 is for Thread 0 (Dispatcher), rest for Kernel Workers

#define FW_SMODE_STACK_BASE                               (S_DATA_REGION_BASE + S_DATA_REGION_SIZE)
#define FW_SMODE_STACK_SCRATCH_REGION_SIZE                64 /* Used by trap handler. 64B is the offset to distribute stack bases across memory controllers. */
#define FW_SMODE_STACK_SIZE                               (4096 + FW_SMODE_STACK_SCRATCH_REGION_SIZE) /* (4K + 64B) stack * 2112 stacks = 8580KB */

// Give 4K for VM stack pages
// Bits[8:6] of an address specify memshire number, and bit[9] the controller within memshire. Offset by 1<<6 = 64 to distribute stack bases across different memory controllers
#define KERNEL_UMODE_STACK_BASE 0x8005000000ULL
#define KERNEL_UMODE_STACK_SIZE (4096 + 64)

/*****************************************************************/
/*              - Low Memory Region Layout (28G) -               */
/*                (Base Address: 0x8100000000)                   */
/*     - user            - base-offset   - size                  */
/*     Sub-regions       0x0             0x600000 (6M)           */
/*     Host-managed      0x600000        0x2FBA00000 (12218M)    */
/*     Reserved          0x2FC000000     0x4000000 (64M)         */
/*     Not-used          0x300000000     0x400000000 (16G)       */
/* NOTE: "Not-used" region is accessible but not used currently. */
/*****************************************************************/

/* Reserved area for DDR low memory sub regions */
#define LOW_MEM_SUB_REGIONS_BASE  LOW_MEM_REGION_BASE
#define LOW_MEM_SUB_REGIONS_SIZE  0x0000600000ULL

/* U-mode user kernels entry point
(Fixed address - should sync kernels linker script if this is changed) */
#define KERNEL_UMODE_ENTRY        0x8100600000ULL

//Storage for DMA configuration linked lists.
//Store at end of R_L3_DRAM 16 GB region, 64MB at 0x83FC800000 - 0x83FFFFFFFF
//For each DMA channel, reserve 8MB for the list. Chosen arbitrarily to balance mem
//useage vs likely need. Each entry is 24 bytes, so 349525 entries max per list. If the
//host system is totally fragmented (each entry tracks 4kB - VERY unlkely), can DMA
//~1.4GB without modifying the list.
#define DMA_LL_SIZE 0x800000

#define DMA_CHAN_READ_0_LL_BASE  0x83FC000000
#define DMA_CHAN_READ_1_LL_BASE  0x83FC800000
#define DMA_CHAN_READ_2_LL_BASE  0x83FD000000
#define DMA_CHAN_READ_3_LL_BASE  0x83FD800000
#define DMA_CHAN_WRITE_0_LL_BASE 0x83FE000000
#define DMA_CHAN_WRITE_1_LL_BASE 0x83FE800000
#define DMA_CHAN_WRITE_2_LL_BASE 0x83FF000000
#define DMA_CHAN_WRITE_3_LL_BASE 0x83FF800000

// Define the address range in DRAM that the host runtime can explicitly manage
// the range is the START to (END-1)
#define HOST_MANAGED_DRAM_START KERNEL_UMODE_ENTRY
#define HOST_MANAGED_DRAM_END   DMA_CHAN_READ_0_LL_BASE
#define HOST_MANAGED_DRAM_SIZE  (HOST_MANAGED_DRAM_END - HOST_MANAGED_DRAM_START)

// This range is mapped to the host via BAR0. The host can write the DMA configuration
// linked list, but should never touch the stacks for the SoC processors in the first
// part of DRAM.
#define DRAM_MEMMAP_BEGIN LOW_MEM_REGION_BASE
#define DRAM_MEMMAP_END   0x87FFFFFFFF
#define DRAM_MEMMAP_SIZE  (DRAM_MEMMAP_END - DRAM_MEMMAP_BEGIN + 1)

// This range is used as Scratch area as storage of new FW image whilst SP updates the
// correspoding Flash partition to take affect.
// TODO: SW-4611 - these will move to BAR relative addressing once SW-4611 is resolved
#define FW_UPDATE_REGION_BEGIN  0x8005120000ULL
#define FW_UPDATE_REGION_SIZE   0x400000U

/* Trace buffers for Service Processor, Master Minion, and Compute Minion are consecutive 
   memory regions in DDR User Memory Sub Region, starting from (BAR0 + 0x400000) with sizes
   4KB, 512KB, and 512KB respectively. */
/* SP DM services Trace Buffer */
#define SP_TRACE_BUFFER_BASE              (LOW_MEM_SUB_REGIONS_BASE + 0x400000)
#define SP_TRACE_BUFFER_SIZE              0x1000 /* 4KB for SP DM services Trace Buffer */

/* Master Minion FW Trace Buffer */
#define MM_TRACE_BUFFER_BASE              (SP_TRACE_BUFFER_BASE + SP_TRACE_BUFFER_SIZE)
#define MM_TRACE_BUFFER_SIZE              0x100000 /* 1MB for Master Minion Trace Buffer */

/* Compute Minion FW Trace Buffer */
#define CM_TRACE_BUFFER_BASE              (MM_TRACE_BUFFER_BASE + MM_TRACE_BUFFER_SIZE)
#define CM_TRACE_BUFFER_SIZE              0x400000 /* 4MB for Compute Minion Trace Buffer */

/************************/
/* Compile-time checks  */
/************************/
#ifndef __ASSEMBLER__

/* Ensure the shared message buffers don't overlap with the S-stacks */
static_assert((CM_MM_IFACE_UNICAST_LOCKS_BASE_ADDR + CM_MM_IFACE_UNICAST_LOCKS_SIZE) <
              (FW_SMODE_STACK_BASE - (NUM_HARTS * FW_SMODE_STACK_SIZE)),
              "S-stack / message buffer region collision");

/* Ensure that DDR low memory regions dont overlap U-mode kernels entry */
static_assert(((LOW_MEM_SUB_REGIONS_BASE + LOW_MEM_SUB_REGIONS_SIZE) - 1) < KERNEL_UMODE_ENTRY,
              "DDR low memory / Kernel U-mode entry region collision");

#endif /* __ASSEMBLER__ */

#endif
