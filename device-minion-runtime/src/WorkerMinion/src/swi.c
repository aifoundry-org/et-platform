#include "mm_iface.h"

void swi_handler(void);

// If we are here it means we were executing Userspace code, because in S-mode, IPIs (Supervisor Software Interrupts) are masked,
void swi_handler(void)
{
    // Handle messages from MM
    mm_iface_process();
}


