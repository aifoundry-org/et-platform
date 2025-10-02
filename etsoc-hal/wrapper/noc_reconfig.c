/***********************************************************************
*
* Copyright (C) 2021 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*
************************************************************************/

#include <stdint.h>
#include "slam_engine.h"

#include "hal_noc_reconfig.h"
#include "hwinc/sp_pshire_regbus.h"
#include "hwinc/hal_device.h"

#pragma GCC diagnostic push

#include "../src/noc_reconfig_memshire.c"
#include "../src/noc_reconfig_minshire.c"
#include "../src/noc_reconfig_pshire.c"

#pragma GCC diagnostic pop
