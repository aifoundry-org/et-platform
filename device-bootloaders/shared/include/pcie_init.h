/*-------------------------------------------------------------------------
* Copyright (C) 2019, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------
*/

#ifndef PCIE_INIT_H
#define PCIE_INIT_H

#include <stdbool.h>
#include <stdint.h>

#include "etsoc_hal/inc/rm_esr.h"
#include "etsoc_hal/inc/hal_device.h"
#include "pcie_device.h"
#include "layout.h"
#include "io.h"

void PCIe_release_pshire_from_reset(void);
void PCIe_init(bool expect_link_up);

#endif
