#!/bin/bash
# Copyright (c) 2025 Ainekko, Co.
# SPDX-License-Identifier: Apache-2.0

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
