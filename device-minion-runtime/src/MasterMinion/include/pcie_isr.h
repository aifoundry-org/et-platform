#ifndef PCIE_ISR_H
#define PCIE_ISR_H

#include <stdbool.h>

extern volatile bool pcie_interrupt_flag;

void pcie_isr(void);

#endif
