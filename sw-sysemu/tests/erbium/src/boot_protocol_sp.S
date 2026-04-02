/*-------------------------------------------------------------------------
* Copyright (c) 2026 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------*/

/*
* Test: --payload_sp sets the initial stack pointer
*
* This is a standalone assembly test (no boot.S) that checks SP at the
* very first instruction.  Must be run with:
*   --boot_elf <this_elf> --payload_sp 200c100
*
* The boot protocol sets SP before the hart starts executing, so we
* check it immediately at entry before anything can overwrite it.
*/

  .section .text.init, "ax", @progbits
  .globl _boot
_boot:
  li    t0, 0x200c100
  bne   sp, t0, fail

  /* PASS */
  li    a7, 0x1feed000
  csrw  0x8d0, a7       /* validation0 */
  j     done

fail:
  /* FAIL */
  li    a7, 0x50bad000
  csrw  0x8d0, a7       /* validation0 */

done:
  wfi
  j     done
