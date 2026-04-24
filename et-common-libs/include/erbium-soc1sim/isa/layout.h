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

#ifndef _ERBIUM_SOC1SIM_ISA_LAYOUT_H_
#define _ERBIUM_SOC1SIM_ISA_LAYOUT_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Compile-time region bases / sizes — NOT defined on the soc1sim
 * backend. The native-erbium side aliases these onto hwinc/top.h
 * symbols (LAYOUT_MAIN_MEM_BASE = ERBIUM_TOP_MRAM_BASE etc); on
 * soc1sim there is no equivalent because the kernel is loaded into
 * an etsoc1 DDR region whose absolute address is firmware-policy
 * (varies across launches), and there is no SRAM/BOOTROM/ESR-base
 * concept the kernel could meaningfully address.
 *
 * Kernels are expected to walk __heap_regions[] for memory; any
 * direct reference to LAYOUT_* on this backend triggers a hard
 * compile error with the message below. */
#define _ERBIUM_SOC1SIM_LAYOUT_NOT_DEFINED                                   \
    "LAYOUT_*_BASE / LAYOUT_*_SIZE are not defined on the erbium-soc1sim "   \
    "backend; walk __heap_regions[] for usable memory instead"

extern uintptr_t _layout_unavailable(void)
    __attribute__((error(_ERBIUM_SOC1SIM_LAYOUT_NOT_DEFINED)));

#define LAYOUT_MAIN_MEM_BASE  _layout_unavailable()
#define LAYOUT_MAIN_MEM_SIZE  _layout_unavailable()
#define LAYOUT_SRAM_BASE      _layout_unavailable()
#define LAYOUT_SRAM_SIZE      _layout_unavailable()
#define LAYOUT_BOOTROM_BASE   _layout_unavailable()
#define LAYOUT_BOOTROM_SIZE   _layout_unavailable()
#define LAYOUT_ESR_BASE       _layout_unavailable()
#define LAYOUT_ESR_SIZE       _layout_unavailable()

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

#endif /* _ERBIUM_SOC1SIM_ISA_LAYOUT_H_ */
