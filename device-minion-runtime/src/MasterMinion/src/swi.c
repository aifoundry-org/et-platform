#include "swi.h"

void swi_handler(void);

volatile bool swi_flag __attribute__((aligned(64))) = false;

void swi_handler(void)
{
    // We got a software interrupt handed down from M-mode.
    // M-mode already cleared the software interrupt.

    // Clear pending software interrupt
    asm volatile("csrci sip, 0x2");

    // Set swi_flag
    swi_flag = true;
}
