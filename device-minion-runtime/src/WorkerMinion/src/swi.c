#include "ipi.h"
#include "shire.h"

void swi_handler(void);

void swi_handler(void)
{
    // We got a software interrupt handed down from M-mode.
    // M-mode already cleared the IPI - Go check mailboxes
}
