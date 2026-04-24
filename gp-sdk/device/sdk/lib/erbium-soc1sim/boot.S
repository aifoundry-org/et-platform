/*-------------------------------------------------------------------------
* Copyright (c) 2026 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------*/

/* Erbium-on-etsoc1 first-stage entry.
 *
 * The etsoc1 firmware hands us control in U-mode with:
 *   - sp      - points into a firmware-owned stack (replaced below);
 *   - a0..a3  - firmware-supplied kernel-launch args (dismissed in crt.S);
 *   - gp      - zeroed (crt.S re-establishes it);
 *   - GPR/FP/matrix regs cleared, FPU enabled, fcsr cleared.
 *
 * We only need to swap the firmware-supplied sp for our own
 * linker-managed per-hart stack and jump to _start; everything else
 * the firmware already did. The shared <erbium/boot.h> contract
 * still applies — most macros are no-ops on this backend.
 */

#include "erbium/boot.h"

  .section .text.init, "ax", @progbits
  .global _boot
_boot:
  et_boot_init_mmode_csrs
  et_boot_set_per_hart_stack
  et_boot_zero_regfile
  et_boot_drop_to_umode _start
