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

/*
 * Initially written by emmanuel.marie@esperantotech.com
 * Moved from esperanto-soc to device-firmware repo by doug.crowder@esperantotech.com
 */

#=========================================================================
# Bootrom code for Minion core
#=========================================================================

.section .text.init, "ax", @progbits
.global _boot

_boot:
    # init some csr regs
    csrwi satp, 0
    csrr  t0, mstatus
    li    t1, 0x6000
    or    t0, t0, t1
    csrw  mstatus, t0 # set fs=11 (dirty)
    csrrw x0, fcsr, x0 # clear floating point status
    csrrw x0, mip, x0 # clear all pending interrupts

    # initialize trap vector
    # la    t0, mtrap_vector
    # csrw  mtvec, t0

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
    # gp set later by crt.S
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

#ifdef __riscv_flen
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
#endif

    la a1, _start
    jalr x0, a1
