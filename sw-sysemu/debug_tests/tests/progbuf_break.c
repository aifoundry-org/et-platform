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

    uint64_t kernel = Read_GPR(0, 8); // s0

    Write_CSR(0, CSR_TDATA1, 0x28c000000000107c);
    Write_CSR(0, CSR_TDATA2, kernel);

    Resume_Harts();

    while (amoorgw(ADDR_launched, 0) < 1)
        ;

    /* The test should have halted at the kernel */
    uint64_t hastatus0 = read_hastatus0(0, 0);
    EXPECTX(hastatus0, ==, 0x1);

    uint64_t dpc = Read_CSR(0, CSR_DPC);
    EXPECTX(dpc, ==, kernel);


    /* resume without clearing the breakpoint should break again */
    Resume_Harts();

    TRY_UNTIL(halted, 5)
    {
        hastatus0 = read_hastatus0(0, 0);
        if (hastatus0 == 0x1)
            break;
    }
    if (!halted)
        failf("expected breakpoint to trigger again");


    /* clear the breakpoint this time */
    Write_CSR(0, CSR_TDATA1, 0x0);

    Resume_Harts();

    return 0;
}


#else /* Minions */

uint32_t g_launched DATA = 0;


void kernel()
{
    for (int i = 0; i < 1000; ++i)
        asm volatile("addi x0,x0,0");
}


void launch_kernel()
{
    amoaddgw((uint64_t)&g_launched, 1);
    kernel();
}


int main()
{
    asm volatile("la s0, kernel\n"
                 "wfi\n");

    launch_kernel();

    return 0;
}


#endif
