/***********************************************************************/
/*! \copyright
  Copyright (c) 2026 Ainekko, Co.
  SPDX-License-Identifier: Apache-2.0
*/
/***********************************************************************/
/*! \file boot.h
    \brief Boot helpers for the soc1sim backend (U-mode entry).

    Same names and required ordering as native erbium's <erbium/boot.h>
    so a kernel can be backend-agnostic. The shadow-installed copy
    of this file occupies <erbium/boot.h> on consumers linking
    erbium-soc1sim.

    On soc1sim the etsoc1 firmware hands control directly in U-mode
    with FPU enabled, fcsr cleared, and the GPR/FP/matrix register
    files already zero. Most boot helpers therefore reduce to no-ops;
    only the per-hart stack pointer and the jump to _start carry real
    work.
*/
/***********************************************************************/

#ifndef _ERBIUM_SOC1SIM_BOOT_H_
#define _ERBIUM_SOC1SIM_BOOT_H_

#ifdef __ASSEMBLER__

/* No-op: firmware already set up M-mode CSRs and switched to U-mode. */
.macro et_boot_init_mmode_csrs
.endm

/* Set the per-hart stack pointer.
 *
 *   sp = __stack_base - (hartid & 0x3F) * STACK_SIZE
 *
 * `hartid` is the U-mode shadow CSR (firmware enables menable_shadows
 * before handing off). The mask trims to the shire-local hart index
 * (64 harts/shire on non-master). `__stack_base` and `STACK_SIZE` are
 * linker symbols (defaults in erbium.ld).
 *
 * Clobbers: t0, t1.
 */
.macro et_boot_set_per_hart_stack
  la    sp, __stack_base
  csrr  t0, hartid
  andi  t0, t0, 0x3F
  lui   t1, %hi(STACK_SIZE)
  addi  t1, t1, %lo(STACK_SIZE)
  mul   t0, t0, t1
  sub   sp, sp, t0
.endm

/* No-op: firmware delivers a clean register file. Backend-symmetric
 * placeholder so kernel boot.S can stay unchanged across backends. */
.macro et_boot_zero_regfile
.endm

/* Plain jump to \entry — there's no privilege transition to make,
 * the hart is already in U-mode.
 *
 * Does not return.
 */
.macro et_boot_drop_to_umode entry
  j \entry
.endm

#endif /* __ASSEMBLER__ */

#endif /* _ERBIUM_SOC1SIM_BOOT_H_ */
