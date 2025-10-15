/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
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
