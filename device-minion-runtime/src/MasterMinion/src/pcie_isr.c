#include "pcie_isr.h"
#include "hal_device.h"

#include <stdint.h>

volatile bool pcie_interrupt_flag = false;

void pcie_isr(void)
{
    // We got an external interrupt from the PCI-E controller
    pcie_interrupt_flag = true;

    // Decrement PCI-E interrupt counter
    volatile uint32_t *const pcie_int_dec_ptr = (uint32_t *)(R_PU_TRG_MMIN_BASEADDR + 0x8);
    volatile uint32_t *const pcie_int_cnt_ptr = (uint32_t *)(R_PU_TRG_MMIN_BASEADDR + 0xC);

    if (*pcie_int_cnt_ptr) {
        // Let's not wrap
        *pcie_int_dec_ptr = 1;
    }
}
