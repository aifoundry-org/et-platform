// SYSEMU -shires 0x400000001 -minions 0x1 -single_thread

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

    uint64_t gprs[32] = {0};

    Read_All_GPR(0, gprs);

    Resume_Harts();

    EXPECTX(gprs[8], ==, 0x50);
    EXPECTX(gprs[9], ==, 0x51);
    EXPECTX(gprs[18], ==, 0x52);
    EXPECTX(gprs[19], ==, 0x53);
    EXPECTX(gprs[20], ==, 0x54);
    EXPECTX(gprs[21], ==, 0x55);
    EXPECTX(gprs[22], ==, 0x56);
    EXPECTX(gprs[23], ==, 0x57);
    EXPECTX(gprs[24], ==, 0x58);
    EXPECTX(gprs[25], ==, 0x59);
    EXPECTX(gprs[26], ==, 0x5A);
    EXPECTX(gprs[27], ==, 0x5B);

    return 0;
}


#else /* Minions */

uint32_t g_flag DATA = 0;

int main()
{
    asm volatile("li s0,0x50\n"
                 "li s1,0x51\n"
                 "li s2,0x52\n"
                 "li s3,0x53\n"
                 "li s4,0x54\n"
                 "li s5,0x55\n"
                 "li s6,0x56\n"
                 "li s7,0x57\n"
                 "li s8,0x58\n"
                 "li s9,0x59\n"
                 "li s10,0x5A\n"
                 "li s11,0x5B\n");
    amoorgw((uint64_t)&g_flag, 1);

    return 0;
}


#endif
