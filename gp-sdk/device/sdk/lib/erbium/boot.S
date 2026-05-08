/*-------------------------------------------------------------------------
* Copyright (c) 2026 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------*/

/* Erbium M-mode first-stage.
 *
 * Lands in the .mboot section, placed by the linker at
 *   MRAM_BASE + MBOOT_OFFSET
 * (defaults: 0x40000040). The first MBOOT_OFFSET bytes of MRAM are
 * reserved for a runtime-supplied config payload and stay empty in
 * the ELF.
 *
 * The whole sequence is composed from <erbium/boot.h> macros so a
 * downstream kernel can swap individual steps by writing its own
 * _boot in any source file (the linker resolves the local definition
 * before scanning this archive).
 */

#include "erbium/boot.h"

  .section .mboot, "ax", @progbits
  .global _boot
_boot:
  et_boot_init_mmode_csrs
  et_boot_set_per_hart_stack
  et_boot_zero_regfile
  et_boot_drop_to_umode _start
