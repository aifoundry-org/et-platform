/***********************************************************************/
/*! \copyright
  Copyright (c) 2026 Ainekko, Co.
  SPDX-License-Identifier: Apache-2.0
*/
/***********************************************************************/
/*! \file boot.h
    \brief Boot helpers for native erbium (M-mode entry).

    A pre-canned sequence of asm `.macro`s that the default boot.S
    composes into a working M-mode entry. A kernel that wants its own
    boot can pick the steps it needs and replace the rest.

    Required ordering when used end-to-end:
      1. et_boot_init_mmode_csrs
      2. et_boot_set_per_hart_stack
      3. et_boot_zero_regfile
      4. et_boot_drop_to_umode <entry>

    Steps 1-2 must run before any C code (no usable stack until step 2).
    Step 4 ends the sequence with mret and never returns.

    The native erbium variant assumes the hart enters in M-mode at
    .mboot (linker-placed at MRAM_BASE + MBOOT_OFFSET).
*/
/***********************************************************************/

#ifndef _ERBIUM_BOOT_H_
#define _ERBIUM_BOOT_H_

#ifdef __ASSEMBLER__

/* Initialize M-mode CSRs and write the "boot start" breadcrumb.
 *
 *   - validation0      <- 0xDEAD0001  (emulator breadcrumb)
 *   - satp             <- 0           (no MMU)
 *   - mstatus.FS       <- 11 (dirty)  (FPU enabled)
 *   - fcsr, mip        <- 0
 *   - tensor_mask      <- 0
 *   - menable_shadows  <- 1           (U-mode hartid shadow CSR)
 *   - mstatus.MPP      <- 00          (mret returns to U-mode)
 *
 * Clobbers: a7, t0, t1.
 */
.macro et_boot_init_mmode_csrs
  li    a7, 0xDEAD0001
  csrw  validation0, a7
  csrwi satp, 0
  csrr  t0, mstatus
  li    t1, 0x6000
  or    t0, t0, t1
  csrw  mstatus, t0              /* fs = dirty (11) */
  csrrw x0, fcsr, x0
  csrrw x0, mip, x0
  csrwi tensor_mask, 0
  csrwi menable_shadows, 1
  li    t1, 0x1800
  csrc  mstatus, t1              /* MPP = 00 (U-mode) */
.endm

/* Set the per-hart stack pointer.
 *
 *   sp = __stack_base - mhartid * STACK_SIZE
 *
 * `__stack_base` and `STACK_SIZE` are linker symbols (default in
 * erbium.ld; overridable per-kernel via -Wl,--defsym=...).
 *
 * Carries through mret into U-mode unchanged.
 *
 * Clobbers: t0, t1.
 */
.macro et_boot_set_per_hart_stack
  la    sp, __stack_base
  csrr  t0, mhartid
  lui   t1, %hi(STACK_SIZE)
  addi  t1, t1, %lo(STACK_SIZE)
  mul   t0, t0, t1
  sub   sp, sp, t0
.endm

/* Zero the integer (skip x0=zero, x2=sp), floating-point, and matrix
 * register files. Run after et_boot_set_per_hart_stack so sp is
 * preserved.
 */
.macro et_boot_zero_regfile
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

  li zero, 0xf
  mova.m.x zero
.endm

/* Set mepc to \entry, write the "boot end" breadcrumb, and mret.
 * The mode transition relies on mstatus.MPP being 00 (set by
 * et_boot_init_mmode_csrs).
 *
 * Does not return.
 *
 * Clobbers: a7, t0.
 */
.macro et_boot_drop_to_umode entry
  la    t0, \entry
  csrw  mepc, t0
  li    a7, 0xDEAD0002
  csrw  validation0, a7
  mret
.endm

#endif /* __ASSEMBLER__ */

#endif /* _ERBIUM_BOOT_H_ */
