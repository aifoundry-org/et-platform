/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

#include <stdint.h>
#include <stdbool.h>
#include "hwinc/etsoc_shire_other_esr.h"
#include "hwinc/esr_region.h"
#include "esr.h"

#include "hwinc/lvdpll_defines.h"
#include "hwinc/lvdpll_modes_config.h"
#include "hwinc/dvfs_lvdpll_modes_config.h"

#include "hwinc/minion_lvdpll_program.h"

#pragma GCC diagnostic push

#include "../src/minion_lvdpll_program.c"

#pragma GCC diagnostic pop
