// SYSEMU -shires 0x400000001 -minions 0x01010101 -single_thread

#include "diag.h"


#ifdef SPIO


int main()
{
    if (!Setup_Debug())
        failf("unable to setup debug");

    Select_Harts(0, 0x0001000100010001);

    for (unsigned neigh = 0; neigh < 4; ++neigh) {
        uint64_t hastatus0 = read_hastatus0(0, neigh);
        EXPECTX(hastatus0, ==, 0x10000);
    }

    if (!Halt_Harts())
        failf("unable to halt harts");

    for (unsigned neigh = 0; neigh < 4; ++neigh) {
        uint64_t hastatus0 = read_hastatus0(0, neigh);
        EXPECTX(hastatus0, ==, 0x1);
    }

    if (!Resume_Harts())
        failf("unable to resume harts");

    for (unsigned neigh = 0; neigh < 4; ++neigh) {
        uint64_t hastatus0 = read_hastatus0(0, neigh);
        EXPECTX(hastatus0, ==, 0x10000);
    }

    return 0;
}


#else /* Minions */

int main()
{
    for (int i = 0; i < 10000; ++i)
        asm volatile("addi x0,x0,0");

    return 0;
}


#endif
