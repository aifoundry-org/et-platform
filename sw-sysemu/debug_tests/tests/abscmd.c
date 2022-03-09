// SYSEMU -shires 0x400000001 -minions 0x1 -single_thread

#include "diag.h"

#include "encoding.h"

#ifdef SPIO

#define MAX_RETRIES 5


int main()
{
    if (!Setup_Debug())
        failf("unable to setup debug");

    Select_Harts(0, 0x1);
    Halt_Harts();

    write_abscmd(0, 0, EBREAK << 32 | CSRWI(CSR_DDATA0, 13));
    TRY_UNTIL(ready, MAX_RETRIES)
    {
        uint64_t hastatus1 = read_hastatus1(0, 0);
        if (!(hastatus1 & 0x1))
            break;
    }
    if (!ready)
        failf("timed out waiting");

    uint64_t hastatus1 = read_hastatus1(0, 0);
    EXPECTX(hastatus1, ==, 0);

    Resume_Harts();

    uint64_t data0 = read_nxdata0(0, 0);
    EXPECT(data0, ==, 13);

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
