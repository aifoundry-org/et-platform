/*-------------------------------------------------------------------------
* Copyright (C) 2019, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------
*/

/* minion_rt_helpers */
#include "layout.h"

/*
 * Initially written by emmanuel.marie@esperantotech.com
 * Moved from esperanto-soc to device-firmware repo by doug.crowder@esperantotech.com
 */

#=========================================================================
# Boot code for Minion core
#=========================================================================

.section .text.init, "ax", @progbits
.global _start

_start:
    csrwi matp, 0       # disable machine VM
    csrwi satp, 0       # disable supervisor VM
    li    t0, 0x6000
    csrs  mstatus, t0   # set floating point status fs=11 (dirty)
    csrwi fcsr, 0       # clear floating point status
    #csrwi mip, 0       # clear all pending interrupts

    addi x1,  x0, 0
    # sp set later
    # gp set later
    addi x4,  x0, 0
    addi x5,  x0, 0
    addi x6,  x0, 0
    addi x7,  x0, 0
    addi x8,  x0, 0
    addi x9,  x0, 0
    addi x10, x0, 0
    addi x11, x0, 0
    addi x12, x0, 0
    addi x13, x0, 0
    addi x14, x0, 0
    addi x15, x0, 0
    addi x16, x0, 0
    addi x17, x0, 0
    addi x18, x0, 0
    addi x19, x0, 0
    addi x20, x0, 0
    addi x21, x0, 0
    addi x22, x0, 0
    addi x23, x0, 0
    addi x24, x0, 0
    addi x25, x0, 0
    addi x26, x0, 0
    addi x27, x0, 0
    addi x28, x0, 0
    addi x29, x0, 0
    addi x30, x0, 0
    addi x31, x0, 0

    fcvt.s.w f0,  x0
    fcvt.s.w f1,  x0
    fcvt.s.w f2,  x0
    fcvt.s.w f3,  x0
    fcvt.s.w f4,  x0
    fcvt.s.w f5,  x0
    fcvt.s.w f6,  x0
    fcvt.s.w f7,  x0
    fcvt.s.w f8,  x0
    fcvt.s.w f9,  x0
    fcvt.s.w f10, x0
    fcvt.s.w f11, x0
    fcvt.s.w f12, x0
    fcvt.s.w f13, x0
    fcvt.s.w f14, x0
    fcvt.s.w f15, x0
    fcvt.s.w f16, x0
    fcvt.s.w f17, x0
    fcvt.s.w f18, x0
    fcvt.s.w f19, x0
    fcvt.s.w f20, x0
    fcvt.s.w f21, x0
    fcvt.s.w f22, x0
    fcvt.s.w f23, x0
    fcvt.s.w f24, x0
    fcvt.s.w f25, x0
    fcvt.s.w f26, x0
    fcvt.s.w f27, x0
    fcvt.s.w f28, x0
    fcvt.s.w f29, x0
    fcvt.s.w f30, x0
    fcvt.s.w f31, x0

    # Initialize the global pointer
.option push
.option norelax
    la gp, __global_pointer$
.option pop

    # Initialize the stack pointer for each thread
    csrr  t0, mhartid
    li    t1, FW_MMODE_STACK_SIZE
    mul   t0, t0, t1
    li    sp, FW_MMODE_STACK_BASE
    sub   sp, sp, t0

    # Set mscratch to the stack pointer. Used by the trap handler
    csrw  mscratch, sp

    # we compile with -fno-zero-initialized-in-bss, therefore, no BSS is present

    # Jump to main()
    j     main
