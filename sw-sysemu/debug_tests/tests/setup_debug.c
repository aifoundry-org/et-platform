// SYSEMU -shires 0x400000001 -minions 0x1 -single_thread

#include "diag.h"

/***** Service processor *****/
#ifdef SPIO

#define MAX_RETRIES 5


int main()
{
    if (!Setup_Debug())
        failf("unable to setup debug");

    return 0;
}


/***** Minions *****/
#else


int main()
{
    for (int i = 0; i < 1000; ++i)
        asm volatile("addi x0,x0,0");

    return 0;
}


#endif
