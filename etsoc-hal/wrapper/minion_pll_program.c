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
