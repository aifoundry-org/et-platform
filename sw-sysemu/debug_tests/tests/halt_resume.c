// SYSEMU -shires 0x400000001 -minions 0x1 -single_thread

#include "diag.h"


#ifdef SPIO

int main()
{
    if (!Setup_Debug())
        failf("unable to setup debug");

    Select_Harts(0, 0x1);

    uint64_t hastatus0 = read_hastatus0(0, 0);
    EXPECTX(hastatus0, ==, 0x10000);


    if (!Halt_Harts())
        failf("unable to halt hart");

    hastatus0 = read_hastatus0(0, 0);
    EXPECTX(hastatus0, ==, 0x1);


    if (!Resume_Harts())
        failf("unable to resume hart");

    hastatus0 = read_hastatus0(0, 0);
    EXPECTX(hastatus0, ==, 0x10000);

    return 0;
}


#else /* Minions */

int main()
{
    for (int i = 0; i < 1000; ++i)
        asm volatile("addi x0,x0,0");

    return 0;
}


#endif
