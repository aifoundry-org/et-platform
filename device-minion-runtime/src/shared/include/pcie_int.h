/*------------------------------------------------------------------------- 
* Copyright (C) 2019, Esperanto Technologies Inc. 
* The copyright to the computer program(s) herein is the 
* property of Esperanto Technologies.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies or 
* in accordance with the terms and conditions stipulated in the 
* agreement/contract under which the program(s) have been supplied. 
*------------------------------------------------------------------------- 
*/

#ifndef PCIE_INT_H
#define PCIE_INT_H

#include <stdint.h>

typedef enum {
    pcie_int_none = 0,
    pcie_int_legacy,
    pcie_int_msi,
    pcie_int_msix
} pcie_int_t;

/* Returns the int type the host has configured. The host should pick exactly one
 * type to enable at once. If the link is not up yet, no interrupts will be enabled.
 * As the device is being configured when the host driver is loaded, the interrupt
 * type might change.
 */
pcie_int_t pcie_get_int_type(void);

/* Returns the number of interrupt vectors currently enabled by the host for the
 * given int_type.
 */
uint32_t pcie_get_int_vecs(pcie_int_t int_type);

/* Interrupt the host via whatever mechanism it has enabled (MSI, MSI-X, or legacy)
 * For MSI and MSI-X, we request multiple interrupt vectors (i.e. one per mailbox).
 * The host can give us up to the number of vectors requested. Use pcie_get_int_vecs
 * to determine the number of vectors.
 * Returns 0 on success, negative on failure
 */
int pcie_interrupt_host(uint32_t vec);

#endif
