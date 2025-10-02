/* Header generated from ipxact (2019-01-09 14:47:11) */
#ifndef __DW_APB_WDT_H
#define __DW_APB_WDT_H

#include "DW_apb_wdt_macro.h"

struct DW_apb_wdt {
    volatile uint32_t WDT_CR;                                /* 0x00000000 */
    volatile uint32_t WDT_TORR;                              /* 0x00000004 */
    volatile uint32_t WDT_CCVR;                              /* 0x00000008 */
    volatile uint32_t WDT_CRR;                               /* 0x0000000C */
    volatile uint32_t WDT_STAT;                              /* 0x00000010 */
    volatile uint32_t WDT_EOI;                               /* 0x00000014 */
    volatile uint32_t dummy0[51];
    volatile uint32_t WDT_COMP_PARAM_5;                      /* 0x000000E4 */
    volatile uint32_t WDT_COMP_PARAM_4;                      /* 0x000000E8 */
    volatile uint32_t WDT_COMP_PARAM_3;                      /* 0x000000EC */
    volatile uint32_t WDT_COMP_PARAM_2;                      /* 0x000000F0 */
    volatile uint32_t WDT_COMP_PARAM_1;                      /* 0x000000F4 */
    volatile uint32_t WDT_COMP_VERSION;                      /* 0x000000F8 */
    volatile uint32_t WDT_COMP_TYPE;                         /* 0x000000FC */
};

#endif /* __DW_APB_WDT_H */

