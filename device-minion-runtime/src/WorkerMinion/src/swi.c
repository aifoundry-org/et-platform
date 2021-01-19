#include "mm_iface.h"

void swi_handler(void);

// If we are here it means we were executing Userspace code, because in S-mode, IPIs (Supervisor Software Interrupts) are masked,
void swi_handler(void)
{
    // We got a software interrupt (IPI) handed down from M-mode.
    // M-mode already cleared the MSIP (Machine Software Interrupt Pending)
    // Clear Supervisor Software Interrupt Pending (SSIP)
    asm volatile("csrci sip, 0x2");

    // Handle messages from MM
    MM_To_CM_Iface_Process();
}


