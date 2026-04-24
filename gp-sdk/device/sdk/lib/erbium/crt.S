/*-------------------------------------------------------------------------
* Copyright (c) 2026 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------*/

/* Erbium U-mode entry.
 *
 * _start is the first instruction reached in U-mode after the M-mode
 * first-stage (boot.S) issues mret. sp was already set by boot.S and
 * carries through the mode change; gp was wiped with the rest of the
 * GPR file, so re-establish it here. */

#include "erbium/crt.h"

  .text
  .global _start
_start:
  et_crt_init_gp
  call  main
  et_exit a0
