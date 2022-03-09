// SYSEMU -shires 0x400000001 -minions 0x1 -single_thread -max_cycles 10000

#include "diag.h"


#ifdef SPIO


int main()
{
    if (!Setup_Debug())
        failf("failed to setup debug");

    Select_Harts(0, 0x1);
    uint64_t resethalt = (read_hactrl(0, 0) >> 32) & 0xFFFF;
    EXPECTX(resethalt, ==, 0x0);

    Halt_On_Reset(1);
    resethalt = (read_hactrl(0, 0) >> 32) & 0xFFFF;
    EXPECTX(resethalt, ==, 0x1);

    Halt_On_Reset(0);
    resethalt = (read_hactrl(0, 0) >> 32) & 0xFFFF;
    EXPECTX(resethalt, ==, 0x0);

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
