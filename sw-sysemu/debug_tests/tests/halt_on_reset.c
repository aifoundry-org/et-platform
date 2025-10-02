// SYSEMU -shires 0x400000001 -minions 0x1 -single_thread -max_cycles 10000

#include "diag.h"
#include "atomics.h"


#ifdef SPIO

#define MAX_RETRIES 5


int main()
{
    if (!Setup_Debug())
        failf("failed to setup debug");


    /* Wait for first iteration */
    while (amoorgw(ADDR_counter, 0) < 1)
        ;

    Select_Harts(0, 0x1);
    Halt_On_Reset(1);

    uint64_t hastatus0 = read_hastatus0(0, 0);
    EXPECTX(hastatus0, ==, 0x00010000);


    if (!Reset_Harts())
        failf("unable to reset hart");

    hastatus0 = read_hastatus0(0, 0);
    EXPECTX(hastatus0, ==, 0x0001000000000001);


    Ack_Have_Reset();

    hastatus0 = read_hastatus0(0, 0);
    EXPECTX(hastatus0, ==, 0x00000001);


    if (!Resume_Harts())
        failf("unable to resume hart");

    hastatus0 = read_hastatus0(0, 0);
    EXPECTX(hastatus0, ==, 0x00010000);


    /* Wait for second iteration */
    while (amoorgw(ADDR_counter, 0) < 2)
        ;

    return 0;
}


#else /* Minions */

uint32_t g_counter DATA = 0;

int main()
{
    const uint64_t addr = (uint64_t)&g_counter;

    uint32_t cnt = amoaddgw(addr, 1);

    /* Needs to be reset to increase the counter twice */
    while (amoorgw(addr, 0) < 2)
        ;

    return 0;
}


#endif
