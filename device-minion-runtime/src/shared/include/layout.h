#ifndef LAYOUT_H
#define LAYOUT_H

#ifndef __ASSEMBLER__
#include <assert.h>
#endif

// aligns an address to the next 64-byte cache line
#define CACHE_LINE_ALIGN(x) (((x + 63U) / 64U) * 64U)

// Architectural parameters
#define MASTER_SHIRE 32
#define NUM_SHIRES 33
#define HARTS_PER_SHIRE 64
#define NUM_HARTS (NUM_SHIRES * HARTS_PER_SHIRE)
#define MAX_SIMULTANEOUS_KERNELS 4

#define MESSAGE_FLAG_SIZE 8
#define MESSAGE_BUFFER_SIZE 64

// Hardware memory region layout
#define M_CODE_REGION_BASE 0x8000000000ULL /* MCODE region is  2M from 0x8000000000 to 0x80001FFFFF */
#define M_CODE_REGION_SIZE 0x0000200000ULL

#define M_DATA_REGION_BASE 0x8000200000ULL /* MDATA region is  6M from 0x8000200000 to 0x80007FFFFF */
#define M_DATA_REGION_SIZE 0x0000600000ULL

#define S_CODE_REGION_BASE 0x8000800000ULL /* SCODE region is  8M from 0x8000800000 to 0x8000FFFFFF */
#define S_CODE_REGION_SIZE 0x0000800000ULL

#define S_DATA_REGION_BASE 0x8001000000ULL /* SDATA region is 48M from 0x8001000000 to 0x8003FFFFFF */
#define S_DATA_REGION_SIZE 0x0003000000ULL

#define U_CODE_REGION_BASE 0x8004000000ULL /* UCODE region is  4K from 0x8004000000 to 0x8004001000 */
#define U_CODE_REGION_SIZE 0x0000001000ULL

// Software layout
#define FW_MACHINE_MMODE_ENTRY  (M_CODE_REGION_BASE + 0x1000ULL) // Default reset vector
#define FW_MMODE_STACK_BASE     (M_DATA_REGION_BASE + M_DATA_REGION_SIZE)
#define FW_MMODE_STACK_SIZE     768 // Large enough for reentrant M-mode traps, but small enough so all 2112 stacks + code fit in 2MB region

#define FW_MASTER_SMODE_ENTRY   S_CODE_REGION_BASE
#define FW_WORKER_SMODE_ENTRY   (S_CODE_REGION_BASE + 0x400000ULL) // SCODE + 4M

// SDATA is divded into 3 16MB regions: Master data, worker data, and shared message buffers + S-stacks.
// Shared message buffers placed at the beginning of 3rd region
// S-mode stacks grow from the end of the 3rd region

#define FW_WORKER_TO_MASTER_MESSAGE_FLAGS            CACHE_LINE_ALIGN(S_DATA_REGION_BASE + 0x2000000) /* SDATA_REGION_BASE + 32M */
#define FW_WORKER_TO_MASTER_MESSAGE_FLAGS_SIZE       (NUM_SHIRES * MESSAGE_FLAG_SIZE)

#define FW_WORKER_TO_MASTER_MESSAGE_BUFFERS          CACHE_LINE_ALIGN(FW_WORKER_TO_MASTER_MESSAGE_FLAGS + FW_WORKER_TO_MASTER_MESSAGE_FLAGS_SIZE)
#define FW_WORKER_TO_MASTER_MESSAGE_BUFFERS_SIZE     (NUM_HARTS * MESSAGE_BUFFER_SIZE)

#define FW_MASTER_TO_WORKER_MESSAGE_BUFFERS          CACHE_LINE_ALIGN(FW_WORKER_TO_MASTER_MESSAGE_BUFFERS + FW_WORKER_TO_MASTER_MESSAGE_BUFFERS_SIZE)
#define FW_MASTER_TO_WORKER_MESSAGE_BUFFERS_SIZE     (NUM_HARTS * MESSAGE_BUFFER_SIZE)

#define FW_MASTER_TO_WORKER_BROADCAST_MESSAGE_BUFFER CACHE_LINE_ALIGN(FW_MASTER_TO_WORKER_MESSAGE_BUFFERS + FW_MASTER_TO_WORKER_MESSAGE_BUFFERS_SIZE)
#define FW_MASTER_TO_WORKER_BROADCAST_MESSAGE_BUFFER_SIZE MESSAGE_BUFFER_SIZE

#define FW_SMODE_STACK_BASE     (S_DATA_REGION_BASE + S_DATA_REGION_SIZE)
#define FW_SMODE_STACK_SIZE     4096 /* 4K stack * 2112 stacks = 8448KB */

#ifndef __ASSEMBLER__
// Ensure the shared message buffers don't overlap with the S-stacks
static_assert((FW_MASTER_TO_WORKER_BROADCAST_MESSAGE_BUFFER + FW_MASTER_TO_WORKER_BROADCAST_MESSAGE_BUFFER_SIZE) < (FW_SMODE_STACK_BASE - (NUM_HARTS * FW_SMODE_STACK_SIZE)), "S-stack / message buffer region collision");
#endif

// Place kernel configs in DRAM so U-mode kernel can read them
#define FW_MASTER_TO_WORKER_KERNEL_CONFIGS           (U_CODE_REGION_BASE + U_CODE_REGION_SIZE)
#define FW_MASTER_TO_WORKER_KERNEL_CONFIGS_SIZE      0x1000

#define KERNEL_UMODE_STACK_BASE 0x8005000000ULL
#define KERNEL_UMODE_STACK_SIZE 4096 // 4K for VM stack pages

#ifndef __ASSEMBLER__
// Ensure the shared kernel configs don't overlap with the U-stacks
static_assert((FW_MASTER_TO_WORKER_KERNEL_CONFIGS + FW_MASTER_TO_WORKER_BROADCAST_MESSAGE_BUFFER_SIZE) < (KERNEL_UMODE_STACK_BASE - (NUM_HARTS * KERNEL_UMODE_STACK_SIZE)), "U-stack / kenrel config region collision");
#endif

#define KERNEL_UMODE_ENTRY      KERNEL_UMODE_STACK_BASE + 0x100000000ULL // TODO FIXME offset to stay above4GB  OS region for now, see SW-1235

//Storage for DMA configuration linked lists.
//Store at end of R_L3_DRAM region, 64MB at 0x87FC800000 - 0x87FFFFFFFF
//For each DMA channel, reserve 8MB for the list. Chosen arbitrarily to balance mem
//useage vs likely need. Each entry is 24 bytes, so 349525 entries max per list. If the
//host system is totally fragmented (each entry tracks 4kB - VERY unlkely), can DMA
//~1.4GB without modifying the list.
#define DMA_LL_SIZE 0x800000

#define DMA_CHAN_READ_0_LL_BASE 0x87FC000000
#define DMA_CHAN_READ_1_LL_BASE 0x87FC800000
#define DMA_CHAN_READ_2_LL_BASE 0x87FD000000
#define DMA_CHAN_READ_3_LL_BASE 0x87FD800000
#define DMA_CHAN_WRITE_0_LL_BASE 0x87FE000000
#define DMA_CHAN_WRITE_1_LL_BASE 0x87FE800000
#define DMA_CHAN_WRITE_2_LL_BASE 0x87FF000000
#define DMA_CHAN_WRITE_3_LL_BASE 0x87FF800000

// Define the address range in DRAM that the host runtime can explicitly manage
// the range is the START to (END-1)
#define HOST_MANAGED_DRAM_START KERNEL_UMODE_ENTRY
#define HOST_MANAGED_DRAM_END DMA_CHAN_READ_0_LL_BASE

#endif
