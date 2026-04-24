/***********************************************************************/
/*! \copyright
  Copyright (c) 2026 Ainekko, Co.
  SPDX-License-Identifier: Apache-2.0
*/
/***********************************************************************/
/*! \file crt.h
    \brief U-mode crt helpers for the soc1sim backend.

    Same names and signatures as native erbium's <erbium/crt.h>. The
    shadow-installed copy of this file occupies <erbium/crt.h> on
    consumers linking erbium-soc1sim.

    `et_exit` returns to the etsoc1 firmware via
    `ecall SYSCALL_RETURN_FROM_KERNEL` so the firmware's post-launch
    FCC barrier completes cleanly across the shire.
*/
/***********************************************************************/

#ifndef _ERBIUM_SOC1SIM_CRT_H_
#define _ERBIUM_SOC1SIM_CRT_H_

#include "erbium/isa/syscall.h"

#ifdef __ASSEMBLER__

/* Same as the native variant — gp setup is privilege-independent. */
.macro et_crt_init_gp
  .option push
  .option norelax
1:auipc gp, %pcrel_hi(__global_pointer$)
  addi  gp, gp, %pcrel_lo(1b)
  .option pop
.endm

/* Return to the etsoc1 firmware:
 *   a0 = SYSCALL_RETURN_FROM_KERNEL
 *   a1 = \status            (kernel-defined return value)
 *   a2 = KERNEL_RETURN_SUCCESS
 *
 * If \status is the symbol `a0` the caller is treating the live a0
 * (typically main()'s return value) as the status — handled with an
 * explicit `mv a1, a0` shortcut below.
 *
 * Does not return on the firmware side (the syscall doesn't return
 * to the kernel); a trailing spin guards against unexpected return.
 */
.macro et_exit status
  mv    a1, \status
  li    a2, KERNEL_RETURN_SUCCESS
  li    a0, SYSCALL_RETURN_FROM_KERNEL
  ecall
1:j 1b
.endm

#else /* !__ASSEMBLER__ */

/* C-callable early exit. Issues SYSCALL_RETURN_FROM_KERNEL with
 * KERNEL_RETURN_SUCCESS; \status becomes the firmware-visible
 * return value. Does not return.
 */
static inline __attribute__((always_inline, noreturn))
void et_exit(int status)
{
    (void)syscall(SYSCALL_RETURN_FROM_KERNEL,
                  (uint64_t)(int64_t)status,
                  KERNEL_RETURN_SUCCESS,
                  0);
    __builtin_unreachable();
}

#endif /* __ASSEMBLER__ */

#endif /* _ERBIUM_SOC1SIM_CRT_H_ */
