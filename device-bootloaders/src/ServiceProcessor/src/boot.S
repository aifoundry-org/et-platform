/*-------------------------------------------------------------------------
* Copyright (C) 2018, Esperanto Technologies Inc.
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

#ifndef STACK_BASE
#define STACK_BASE 0x80A2000000
#endif
#ifndef STACK_SIZE_LOG2
#define STACK_SIZE_LOG2 14
#endif

  .section stack, "aw", @nobits
  .space (1<<STACK_SIZE_LOG2),0

  .section bootrom, "ax", @progbits
  .global _boot
_boot:
#   # init some csr regs
#   csrwi satp, 0
#   csrr  t0, mstatus
#   li    t1, 0x6000
#   or    t0, t0, t1
#   csrw  mstatus, t0 # set fs=11 (dirty)
#   csrrw x0, fcsr, x0
#   csrrw x0, mip, x0
#   # set the stack pointer, different for each thread
#   csrr  a1, mhartid
#   slli  a1, a1, STACK_SIZE_LOG2
#   li    sp, STACK_BASE
#   sub   sp, sp, a1

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

###################################################################\\
#  * FIXME *                                                        #
#####################################################################
# Enabling interrupts generates several test errors
#####################################################################
# enable interrupts
#  csrrw x0, mie, t0

  addi x1,  x0, 0
  addi x3,  x0, 0
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

  la a1, _boot 
  jalr x0, a1 # Infinite loop
