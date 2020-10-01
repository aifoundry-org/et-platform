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

.extern __bss_start
.extern __bss_end
.extern __stack_top

    .section .text.init, "ax", @progbits

    .global bl2_entry
bl2_entry:
    # disable interrupts
    csrci mstatus, 8

    # initialize trap vector
    la    t0, mtrap_vector
    csrw  mtvec, t0

    # initialize global pointer
.option push
.option norelax
    la   gp, __global_pointer$
.option pop

    # initialize stack pointer
    la   sp,  __stack_top

    # zero out the bss section
    la   t0, __bss_start
    la   t1, __bss_end
    bge  t0, t1, bss_loop_end
1:  sd   x0, 0(t0)
    addi t0, t0, 8
    blt  t0, t1, 1b

bss_loop_end:
    la   t2,  bl2_main
    jalr t2
