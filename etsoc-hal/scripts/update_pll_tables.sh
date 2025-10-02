#!/bin/bash
#------------------------------------------------------------------------------
# Copyright (C) 2021, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

#source
LVDPLL_SRC="$REPOROOT/dv/common/sw/ip/inc/movellus_lvdpll_modes_config.h"
HPDPLL_SRC="$REPOROOT/dv/common/sw/ip/inc/movellus_hpdpll_modes_config.h"
DVFS_LPDPLL_SRC="$REPOROOT/dv/common/sw/ip/inc/dvfs_movellus_lvdpll_modes_config.h"

#destination
LVDPLL_TABLE_FILE="include/hwinc/lvdpll_modes_config.h"
HPDPLL_TABLE_FILE="include/hwinc/hpdpll_modes_config.h"
DVFS_LPDPLL_TABLE_FILE="include/hwinc/dvfs_lvdpll_modes_config.h"

SCRIPT_HOME="$(dirname $0)"
source "$SCRIPT_HOME/helper.sh"
INIT_ENV || exit 1

UPDATE_FILE "$LVDPLL_SRC"      "$ETSOC_HAL_HOME/$LVDPLL_TABLE_FILE"
UPDATE_FILE "$HPDPLL_SRC"      "$ETSOC_HAL_HOME/$HPDPLL_TABLE_FILE"
UPDATE_FILE "$DVFS_LPDPLL_SRC" "$ETSOC_HAL_HOME/$DVFS_LPDPLL_TABLE_FILE"
