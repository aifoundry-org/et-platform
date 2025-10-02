// SYSEMU -shires 0x400000001 -minions 0x1 -single_thread -max_cycles 10000

#include "diag.h"

#include "atomics.h"
#include "encoding.h"

#ifdef SPIO


int main()
{
    if (!Setup_Debug())
        failf("unable to setup debug");

    while (!amoandgw(ADDR_flag, 1))
        ;

    Select_Harts(0, 0x1);
    Halt_Harts();

    /* Write s0 = 1 */
    Write_GPR(0, 8, 1);

    Resume_Harts();

    return 0;
}


#else /* Minions */

uint32_t g_flag DATA = 0;

int main()
{
    amoorgw((uint64_t)&g_flag, 1);

    /* Infinite loop until s0 != 0 */
    asm volatile("   addi s0,x0,0\n"
                 "1: beqz s0,1b\n");

    return 0;
}


#endif
