// SYSEMU -shires 0x400000001 -minions 0x1 -single_thread

#include "diag.h"

#include "atomics.h"
#include "encoding.h"

#ifdef SPIO


int main()
{
    if (!Setup_Debug())
        failf("unable to setup debug");

    Select_Harts(0, 0x1);
    Halt_Harts();

    for (uint64_t i = 0; i < 32; ++i) {
        uint64_t old = Read_GPR(0, i);
        Write_GPR(0, i, i);
        uint64_t val = Read_GPR(0, i);
        EXPECTX(val, ==, i);
        Write_GPR(0, i, old);
    }

    Resume_Harts();

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
