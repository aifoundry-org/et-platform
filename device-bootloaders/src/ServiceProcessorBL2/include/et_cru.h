/* Header generated from ipxact (2019-01-31 03:59:18) */
#ifndef ET_CRU_H
#define ET_CRU_H

#include <stdint.h>

struct et_cru {
    volatile uint32_t CM_PLL0_CTRL;                          /* 0x00000000 */
    volatile uint32_t CM_PLL1_CTRL;                          /* 0x00000004 */
    volatile uint32_t CM_PLL2_CTRL;                          /* 0x00000008 */
    volatile uint32_t CM_PLL3_CTRL;                          /* 0x0000000C */
    volatile uint32_t CM_PLL4_CTRL;                          /* 0x00000010 */
    volatile uint32_t dummy0[3];
    volatile uint32_t CM_PLL0_STATUS;                        /* 0x00000020 */
    volatile uint32_t CM_PLL1_STATUS;                        /* 0x00000024 */
    volatile uint32_t CM_PLL2_STATUS;                        /* 0x00000028 */
    volatile uint32_t CM_PLL3_STATUS;                        /* 0x0000002C */
    volatile uint32_t CM_PLL4_STATUS;                        /* 0x00000030 */
    volatile uint32_t dummy1[3];
    volatile uint32_t CM_PLL0_BYPASS;                        /* 0x00000040 */
    volatile uint32_t dummy2[3];
    volatile uint32_t CM_DVFS;                               /* 0x00000050 */
    volatile uint32_t dummy3[3];
    volatile uint32_t CM_CLK_GATE_50MHZ;                     /* 0x00000060 */
    volatile uint32_t CM_CLK_GATE_24MHZ;                     /* 0x00000064 */
    volatile uint32_t CM_CLK_GATE_120MHZ;                    /* 0x00000068 */
    volatile uint32_t CM_CLK_GATE_200MHZ;                    /* 0x0000006C */
};

#endif /* ET_CRU_H */
