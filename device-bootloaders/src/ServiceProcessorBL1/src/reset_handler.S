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

#=========================================================================
# Reset handler code for Service Processor core
#=========================================================================

.section .text.init, "ax", @progbits
.global _reset_handler

_reset_handler:
    # reset the Supervisor Address Translation and Protection Register to turn virtual memory off
    csrwi satp, 0

    # set the FS field of the mstatus register to dirty
    csrr  t0, mstatus
    li    t1, 0x6000
    or    t0, t0, t1
    csrw  mstatus, t0

    # clear floating point status
    csrrw x0, fcsr, x0

    # clear all pending interrupts
    csrrw x0, mip, x0

    # initialize trap vector
    la    t0, mtrap_vector
    csrw  mtvec, t0

###################################################################\\
#  * FIXME *                                                        #
#####################################################################
# Uncomment mtime/mtimecmp initialization when soc_tb, shiretop_tb  #
# support ESR accesses without notifying BAD accesses to mem region #
#####################################################################
## set mtime to 0, different for each thread
#  csrr  t0, mhartid
#  andi  t0, t0, ~63
#  slli  t0, t0, 16
#  li    a1, 0x01C03001D8  # mtime address
#  or    a1, a1, t0
#  sd    x0, 0(a1)
## set mtimecmp to "infinity"
#  li    t0, -1
#  sd    t0, 8(a1)
###################################################################//

    addi x1,  x0, 0
    # sp set later by loader
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

# initialize global pointer
.option push
.option norelax
    la gp, __global_pointer$
.option pop

    la a1, _init_handler
    jalr x0, a1
