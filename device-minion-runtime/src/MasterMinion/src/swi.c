#include "swi.h"

void swi_handler(void);

bool swi_flag;

void swi_handler(void)
{
    // We got a software interrupt handed down from M-mode.
    // M-mode already cleared the software interrupt.

    // Set swi_flag
    swi_flag = true;

    // Clear pending software interrupt
    asm volatile ("csrci sip, 0x2");
}
