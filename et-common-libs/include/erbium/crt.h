/***********************************************************************/
/*! \copyright
  Copyright (c) 2026 Ainekko, Co.
  SPDX-License-Identifier: Apache-2.0
*/
/***********************************************************************/
/*! \file crt.h
    \brief U-mode crt helpers for native erbium.

    Two flavors:
      - asm `.macro` form used by the default crt.S
      - inline-C `et_exit()` for early exit from C error paths

    On native erbium there is no kernel/firmware syscall ABI yet, so
    `et_exit` ignores its status argument: completion is signaled by
    writing 0x1FEED000 to the validation0 CSR (emulator breadcrumb).
*/
/***********************************************************************/

#ifndef _ERBIUM_CRT_H_
#define _ERBIUM_CRT_H_

#ifdef __ASSEMBLER__

/* Re-establish gp via PC-relative adjust. Used at the top of _start
 * because boot.S wipes the GPR file (gp included) right before mret.
 *
 * Wraps the auipc/addi pair in `.option norelax` so the linker doesn't
 * collapse it into "mv gp, gp" once it sees gp is the destination.
 */
.macro et_crt_init_gp
  .option push
  .option norelax
1:auipc gp, %pcrel_hi(__global_pointer$)
  addi  gp, gp, %pcrel_lo(1b)
  .option pop
.endm

/* Signal kernel completion and spin.
 *
 * Writes 0x1FEED000 to validation0 (emulator breadcrumb: "this hart
 * is done — PASS"); on real hardware this is a harmless write to a
 * diagnostic CSR. Then spins so a real-hw run doesn't fall off the
 * end of .text.
 *
 * The \status argument is currently ignored — native erbium has no
 * syscall path yet — but is kept in the signature so the same name
 * works on backends that do report status.
 *
 * Does not return. Clobbers: t0.
 */
.macro et_exit status
  li    t0, 0x1FEED000
  csrw  validation0, t0
1:j 1b
.endm

#else /* !__ASSEMBLER__ */

/* C-callable early exit. Same semantics as the asm `et_exit` macro:
 * write the validation0 breadcrumb and spin. Status is ignored on
 * native erbium.
 */
static inline __attribute__((always_inline, noreturn))
void et_exit(int status)
{
    (void)status;
    __asm__ volatile(
        "li t0, 0x1FEED000\n\t"
        "csrw validation0, t0\n\t"
        "1: j 1b\n\t"
        ::: "t0", "memory");
    __builtin_unreachable();
}

#endif /* __ASSEMBLER__ */

#endif /* _ERBIUM_CRT_H_ */
