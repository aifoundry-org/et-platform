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

    Read_GPR(0, 0);

    Resume_Harts();

    return 0;
}


#else /* Minions */

int main()
{
    asm volatile("wfi");
    return 0;
}


#endif
