/*-------------------------------------------------------------------------
 * Copyright (c) 2026 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-----------------------------------------------------------------------*/

#include "erbium/isa/layout.h"

/* Provided by the soc1sim linker script; both live inside the
 * .heap0 (NOLOAD) output section so they are section-relative and
 * relocate with the rest of the kernel image.
 *
 */
extern char heap0_start[];
extern char heap0_end[];

const heap_region_t __heap_regions[] = {
    { heap0_start, heap0_end },
    { NULL, NULL },
};
