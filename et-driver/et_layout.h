#ifndef __ET_LAYOUT_H
#define __ET_LAYOUT_H

//TODO: Keep in sync with device-firmware/src/shared/include/layout.h
//TODO: Single source between these two files

//Constants for DRAM layout. Addresses in SoC memory space.

#define KERNEL_UMODE_STACK_BASE 0x8005000000ULL

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
#define HOST_MANAGED_DRAM_START KERNEL_UMODE_STACK_BASE
#define HOST_MANAGED_DRAM_END DMA_CHAN_READ_0_LL_BASE

// This range is mapped to the host via BAR0. The host can write the DMA configuration
// linked list, but should never touch the stacks for the SoC processors in the first
// part of DRAM.
#define DRAM_MEMMAP_BEGIN KERNEL_UMODE_STACK_BASE
#define DRAM_MEMMAP_END 0x87FFFFFFFF
#define DRAM_MEMMAP_SIZE (DRAM_MEMMAP_END - DRAM_MEMMAP_BEGIN + 1)

#endif
