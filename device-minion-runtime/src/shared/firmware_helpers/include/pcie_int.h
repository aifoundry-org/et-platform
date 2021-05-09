/***********************************************************************
*
* Copyright (C) 2019 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*
************************************************************************/
#ifndef PCIE_INT_H
#define PCIE_INT_H

#include <stdint.h>

/* Interrupt the host via whatever mechanism it has enabled (MSI, MSI-X, or legacy)
 * For MSI and MSI-X, we request multiple interrupt vectors (i.e. one per CQ).
 * The host can give us up to the number of vectors requested. Use pcie_get_int_vecs
 * to determine the number of vectors.
 * Returns 0 on success, negative on failure
 */
int pcie_interrupt_host(uint32_t vec);

#endif
