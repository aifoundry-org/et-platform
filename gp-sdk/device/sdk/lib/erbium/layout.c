/*-------------------------------------------------------------------------
 * Copyright (c) 2026 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-----------------------------------------------------------------------*/

#include "erbium/isa/layout.h"

/* Provided by the erbium linker script:
 *   __heap_start: start of the kernel heap, section-relative (lives
 *                 inside the .heap output section).
 *   __heap_end  : end of the kernel heap, also section-relative
 *                 (inside .heap, at the last byte past region end).
 * Both taking address-of the symbol yields a linker-resolved address
 * that participates in the normal ELF relocation flow, so a runtime
 * loader that moves the kernel sees the table updated transparently.
 */
extern char __heap_start[];
extern char __heap_end[];

const heap_region_t __heap_regions[] = {
    { __heap_start, __heap_end },
    { NULL, NULL },
};
