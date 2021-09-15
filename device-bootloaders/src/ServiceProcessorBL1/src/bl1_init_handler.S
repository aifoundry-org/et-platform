.extern __bss_start
.extern __bss_end
.extern __stack_base
.extern __data_end
.extern __code_start

#ifndef STACK_SIZE_LOG2
#define STACK_SIZE_LOG2 12
#endif

.global _init_handler

_init_handler:
    # set the stack pointer
    la   sp,  __stack_base

    # R_SP_MISC region is 4K at 0x52029000, SP_BYPASS_CACHE is 32 bits at offset 0x14
    li   t0, 0x52029014
    lw   t1, 0(t0)
    # mask out all the bits except [0] Bypass ICache and [1] Bypass DCache
    and  t1, t1, 3
    # If the ICache and DCache are already enabled (bypass bits are cleared) we can skip to init_bss
    beqz t1, init_bss

    ################################################################
    # patch SRAM parity error bug if L1 is not enabled in BootROM
init_after_bl1_data:
    # zero out after .data/.sdata sections till end of the SRAM at 0x40500000
    la   x26, __data_end
    li   x27, 0x40500000
    bge  x26, x27, after_bl1_data_loop_end
1:  sw   x0,  0(x26)
    addi x26, x26, 4
    blt  x26, x27, 1b

after_bl1_data_loop_end:

init_area_before_rom_data:
    # zero out the area from start of SRAM at 0x40400000 till rom_data at 0x404003c0
    li   x26, 0x40400000
    la   x27, 0x404003c0
    bge  x26, x27, area_before_rom_data_loop_end
1:  sw   x0,  0(x26)
    addi x26, x26, 4
    blt  x26, x27, 1b

area_before_rom_data_loop_end:

init_area_before_bl1:
    # zero out the area from end of rom_data at 0x404089b0(0x404003c0 + 0x85f0) till BL1 code section
    li   x26, 0x404089b0
    la   x27, __code_start
    bge  x26, x27, area_before_bl1_loop_end
1:  sw   x0,  0(x26)
    addi x26, x26, 4
    blt  x26, x27, 1b

area_before_bl1_loop_end:

init_pu_sram_region1:
    # zero out the area of PU SRAM at 0x20004000 till 0x20040000
    #    R_PU_SRAM_MM_MX:   0x00_2000_4000 - 0x00_2000_4FFF, 4KB
    #    R_PU_MBOX_MM_MX:   0x00_2000_5000 - 0x00_2000_5FFF, 4KB
    #    R_PU_MBOX_MM_SP:   0x00_2000_6000 - 0x00_2000_6FFF, 4KB
    #    R_PU_MBOX_PC_MM:   0x00_2000_7000 - 0x00_2000_7FFF, 4KB
    #    R_PU_SRAM:         0x00_2000_8000 - 0x00_2003_FFFF, 224KB
    li   x26, 0x20004000
    li   x27, 0x20040000
    bge  x26, x27, pu_sram_region1_loop_end
1:  sw   x0,  0(x26)
    addi x26, x26, 4
    blt  x26, x27, 1b

pu_sram_region1_loop_end:

init_pu_sram_region2:
    # zero out the area of PU SRAM at 0x30000000 till 0x30004000
    #    R_PU_MBOX_MX_SP:   0x00_3000_0000 - 0x00_3000_0FFF, 4KB
    #    R_PU_MBOX_PC_MX:   0x00_3000_1000 - 0x00_3000_1FFF, 4KB
    #    R_PU_MBOX_SPARE:   0x00_3000_2000 - 0x00_3000_2FFF, 4KB
    #    R_PU_MBOX_PC_SP:   0x00_3000_3000 - 0x00_3000_3FFF, 4KB
    li   x26, 0x30000000
    li   x27, 0x30004000
    bge  x26, x27, pu_sram_region2_loop_end
1:  sw   x0,  0(x26)
    addi x26, x26, 4
    blt  x26, x27, 1b

pu_sram_region2_loop_end:
    # patch end here
    ################################################################

    # before turning the L1 cache on, synchronize the instruction and data streams
    FENCE
    # Enable L1 I and D caches by clearing [0] Bypass ICache and [1] Bypass DCache
    sw   zero, 0(t0)
    # after turning the L1 cache on, synchronize the instruction and data streams
    FENCE

init_bss:
    # zero out the bss section
    la   x26, __bss_start
    la   x27, __bss_end
    bge  x26, x27, bss_loop_end
1:  sw   x0,  0(x26)
    addi x26, x26, 4
    blt  x26, x27, 1b

bss_loop_end:

    la   a1,  bl1_main
    jalr x0,  a1

.section stack, "aw", @nobits
.space (1<<STACK_SIZE_LOG2),0
