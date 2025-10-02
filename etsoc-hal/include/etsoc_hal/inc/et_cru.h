/* Header generated from ipxact (2019-01-22 19:08:02) */
#ifndef ET_CRU_H
#define ET_CRU_H

#include "et_cru_macro.h"

struct et_cru {
    volatile uint32_t CM_PLL0_CTRL;                          /* 0x00000000 */
    volatile uint32_t CM_PLL1_CTRL;                          /* 0x00000004 */
    volatile uint32_t CM_PLL2_CTRL;                          /* 0x00000008 */
    volatile uint32_t CM_PLL3_CTRL;                          /* 0x0000000C */
    volatile uint32_t CM_PLL4_CTRL;                          /* 0x00000010 */
    volatile uint32_t dummy0;                                /* 0x00000014 */
    volatile uint32_t dummy1;                                /* 0x00000018 */
    volatile uint32_t dummy2;                                /* 0x0000001C */
    volatile uint32_t CM_PLL0_STATUS;                        /* 0x00000020 */
    volatile uint32_t CM_PLL1_STATUS;                        /* 0x00000024 */
    volatile uint32_t CM_PLL2_STATUS;                        /* 0x00000028 */
    volatile uint32_t CM_PLL3_STATUS;                        /* 0x0000002C */
    volatile uint32_t CM_PLL4_STATUS;                        /* 0x00000030 */
    volatile uint32_t dummy3;                                /* 0x00000034 */
    volatile uint32_t dummy4;                                /* 0x00000038 */
    volatile uint32_t dummy5;                                /* 0x0000003C */
    volatile uint32_t CM_DVFS;                               /* 0x00000040 */
    volatile uint32_t CM_IOS_CLR;                            /* 0x00000044 */
    volatile uint32_t dummy6;                                /* 0x00000048 */
    volatile uint32_t dummy7;                                /* 0x0000004C */
    volatile uint32_t CM_CLK_10MHZ;                          /* 0x00000050 */
    volatile uint32_t CM_CLK_24MHZ;                          /* 0x00000054 */
    volatile uint32_t CM_CLK_25MHZ;                          /* 0x00000058 */
    volatile uint32_t CM_CLK_50MHZ;                          /* 0x0000005C */
    volatile uint32_t CM_CLK_100MHZ;                         /* 0x00000060 */
    volatile uint32_t CM_CLK_120MHZ;                         /* 0x00000064 */
    volatile uint32_t CM_CLK_200MHZ;                         /* 0x00000068 */
    volatile uint32_t CM_CLK_300MHZ;                         /* 0x0000006C */

};

#endif /* ET_CRU_H */

