/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------*/

/* Stack size per thread (log2), must match linker script */
#define STACK_SIZE_LOG2 6

  .section bootrom, "ax", @progbits
  .global _boot
_boot:
  /* Signal start of boot via validation0 CSR */
  li   a7, 0xDEAD0001
  csrw validation0, a7

  /* Initialize CSRs */
  csrwi satp, 0
  csrr  t0, mstatus
  li    t1, 0x6000
  or    t0, t0, t1
  csrw  mstatus, t0       /* set fs=11 (dirty) */
  csrrw x0, fcsr, x0
  csrrw x0, mip, x0
  csrwi tensor_mask, 0

  /* Wake MRAM from deep sleep (clear mram_dsleep_en in PowerDomainReq) */
  li    t0, 0x02000038
  sw    zero, 0(t0)

  /* Set mtvec to default trap handler (at 0x02009000) */
  la    t0, default_trap_handler
  csrw  mtvec, t0

  /* Clear mscratch (no trap expected initially) */
  csrw  mscratch, zero

  /* Set stack pointer (different for each thread) */
  la    sp, __stack_base
  csrr  a1, mhartid
  slli  a1, a1, STACK_SIZE_LOG2
  sub   sp, sp, a1

  /* Zero integer registers */
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

  /* Zero floating-point registers */
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

  /* Zero matrix registers */
  li zero, 0xf
  mova.m.x zero

  /* Signal end of boot via validation0 CSR */
  li   a7, 0xDEAD0002
  csrw validation0, a7

  /* Jump to C entry point */
  la a1, _start
  jalr x0, a1
