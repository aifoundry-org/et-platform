/***********************************************************************/
/*! \copyright
  Copyright (c) 2026 Ainekko, Co.
  SPDX-License-Identifier: Apache-2.0
*/
/***********************************************************************/
/*! \file layout.h
    \brief Erbium memory-layout description for user-mode kernels.

    Exposes a NULL-terminated list of heap regions available to the
    kernel (anything past the stack, up to the end of MRAM). A
    user-supplied allocator can walk `__heap_regions` to discover
    memory it is allowed to use; the backend (erbium vs
    erbium-soc1sim) decides how the list is populated.
*/
/***********************************************************************/

#ifndef _ERBIUM_ISA_LAYOUT_H_
#define _ERBIUM_ISA_LAYOUT_H_

#include <stddef.h>

#include "hwinc/top.h"   /* ERBIUM_TOP_*_BASE / *_SIZE */

#ifdef __cplusplus
extern "C" {
#endif

/* Compile-time region bases / sizes, aliased onto the RDL-generated
 * hwinc/top.h symbols. The runtime heap-walking API
 * (`__heap_regions[]`, below) is the preferred way for kernels to
 * discover memory; these defines are here for freestanding code
 * that needs raw region addresses (e.g. M-mode glue, bring-up
 * snippets, or anything addressing SRAM / BOOTROM / ESR by name). */
#define LAYOUT_MAIN_MEM_BASE  ERBIUM_TOP_MRAM_BASE
#define LAYOUT_MAIN_MEM_SIZE  ERBIUM_TOP_MRAM_SIZE
#define LAYOUT_SRAM_BASE      ERBIUM_TOP_SRAM_BASE
#define LAYOUT_SRAM_SIZE      ERBIUM_TOP_SRAM_SIZE
#define LAYOUT_BOOTROM_BASE   ERBIUM_TOP_BOOTROM_BASE
#define LAYOUT_BOOTROM_SIZE   ERBIUM_TOP_BOOTROM_SIZE
#define LAYOUT_ESR_BASE       ERBIUM_TOP_CPU_REGISTERS_BASE
#define LAYOUT_ESR_SIZE       ERBIUM_TOP_CPU_REGISTERS_SIZE

typedef struct {
    void *start;
    void *end;
} heap_region_t;

/* Byte size of a region. Defined as a macro so a NULL-sentinel entry
 * evaluates to 0 without a division or other UB. */
#define heap_region_size(r)  ((size_t)((const uint8_t *)(r)->end - \
                                       (const uint8_t *)(r)->start))

/* NULL-terminated list of heap regions available to user code.
   The final entry { NULL, NULL } is the sentinel. */
extern const heap_region_t __heap_regions[];

#ifdef __cplusplus
}
#endif

#endif /* _ERBIUM_ISA_LAYOUT_H_ */
