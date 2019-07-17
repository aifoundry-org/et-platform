#include "pcie_isr.h"

volatile bool pcie_interrupt_flag;

void pcie_isr(void)
{
    // We got an external interrupt from the PCI-E controller
    pcie_interrupt_flag = true;

    // TODO FIXME Clear pending PCI-E interrupt
}
