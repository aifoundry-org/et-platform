/* Header generated from ipxact (2019-01-09 14:47:11) */
#ifndef __DW_APB_TIMERS_H
#define __DW_APB_TIMERS_H

#include "./DW_apb_timers_macro.h"

struct DW_apb_timers {
    volatile uint32_t TIMER1LOADCOUNT;                       /* 0x00000000 */
    volatile uint32_t TIMER1CURRENTVAL;                      /* 0x00000004 */
    volatile uint32_t TIMER1CONTROLREG;                      /* 0x00000008 */
    volatile uint32_t TIMER1EOI;                             /* 0x0000000C */
    volatile uint32_t TIMER1INTSTAT;                         /* 0x00000010 */
    volatile uint32_t TIMER2LOADCOUNT;                       /* 0x00000014 */
    volatile uint32_t TIMER2CURRENTVAL;                      /* 0x00000018 */
    volatile uint32_t TIMER2CONTROLREG;                      /* 0x0000001C */
    volatile uint32_t TIMER2EOI;                             /* 0x00000020 */
    volatile uint32_t TIMER2INTSTAT;                         /* 0x00000024 */
    volatile uint32_t TIMER3LOADCOUNT;                       /* 0x00000028 */
    volatile uint32_t TIMER3CURRENTVAL;                      /* 0x0000002C */
    volatile uint32_t TIMER3CONTROLREG;                      /* 0x00000030 */
    volatile uint32_t TIMER3EOI;                             /* 0x00000034 */
    volatile uint32_t TIMER3INTSTAT;                         /* 0x00000038 */
    volatile uint32_t TIMER4LOADCOUNT;                       /* 0x0000003C */
    volatile uint32_t TIMER4CURRENTVAL;                      /* 0x00000040 */
    volatile uint32_t TIMER4CONTROLREG;                      /* 0x00000044 */
    volatile uint32_t TIMER4EOI;                             /* 0x00000048 */
    volatile uint32_t TIMER4INTSTAT;                         /* 0x0000004C */
    volatile uint32_t TIMER5LOADCOUNT;                       /* 0x00000050 */
    volatile uint32_t TIMER5CURRENTVAL;                      /* 0x00000054 */
    volatile uint32_t TIMER5CONTROLREG;                      /* 0x00000058 */
    volatile uint32_t TIMER5EOI;                             /* 0x0000005C */
    volatile uint32_t TIMER5INTSTAT;                         /* 0x00000060 */
    volatile uint32_t TIMER6LOADCOUNT;                       /* 0x00000064 */
    volatile uint32_t TIMER6CURRENTVAL;                      /* 0x00000068 */
    volatile uint32_t TIMER6CONTROLREG;                      /* 0x0000006C */
    volatile uint32_t TIMER6EOI;                             /* 0x00000070 */
    volatile uint32_t TIMER6INTSTAT;                         /* 0x00000074 */
    volatile uint32_t TIMER7LOADCOUNT;                       /* 0x00000078 */
    volatile uint32_t TIMER7CURRENTVAL;                      /* 0x0000007C */
    volatile uint32_t TIMER7CONTROLREG;                      /* 0x00000080 */
    volatile uint32_t TIMER7EOI;                             /* 0x00000084 */
    volatile uint32_t TIMER7INTSTAT;                         /* 0x00000088 */
    volatile uint32_t TIMER8LOADCOUNT;                       /* 0x0000008C */
    volatile uint32_t TIMER8CURRENTVAL;                      /* 0x00000090 */
    volatile uint32_t TIMER8CONTROLREG;                      /* 0x00000094 */
    volatile uint32_t TIMER8EOI;                             /* 0x00000098 */
    volatile uint32_t TIMER8INTSTAT;                         /* 0x0000009C */
    volatile uint32_t TimersIntStatus;                       /* 0x000000A0 */
    volatile uint32_t TimersEOI;                             /* 0x000000A4 */
    volatile uint32_t TimersRawIntStatus;                    /* 0x000000A8 */
    volatile uint32_t TIMERS_COMP_VERSION;                   /* 0x000000AC */
    volatile uint32_t TIMER1LOADCOUNT2;                      /* 0x000000B0 */
    volatile uint32_t TIMER2LOADCOUNT2;                      /* 0x000000B4 */
    volatile uint32_t TIMER3LOADCOUNT2;                      /* 0x000000B8 */
    volatile uint32_t TIMER4LOADCOUNT2;                      /* 0x000000BC */
    volatile uint32_t TIMER5LOADCOUNT2;                      /* 0x000000C0 */
    volatile uint32_t TIMER6LOADCOUNT2;                      /* 0x000000C4 */
    volatile uint32_t TIMER7LOADCOUNT2;                      /* 0x000000C8 */
    volatile uint32_t TIMER8LOADCOUNT2;                      /* 0x000000CC */
};

#endif /* __DW_APB_TIMERS_H */

