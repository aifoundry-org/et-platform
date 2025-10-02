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

SRC_SW_IP_DIR="$REPOROOT/dv/common/sw/ip/"

SCRIPT_HOME="$(dirname $0)"
source "$SCRIPT_HOME/helper.sh"
INIT_ENV || exit 1

FILE="lvdpll_defines.h"
UPDATE_FILE "$SRC_SW_IP_DIR/inc/$FILE"  "$ETSOC_HWINC/$FILE"     #source already has license header

# Functions exposed by hal_minion_pll.h
FILE="sp_minion_shire_pll_dll_config.c"
UPDATE_FILE "$SRC_SW_IP_DIR/src/$FILE"  "$ETSOC_HAL_HOME/src/$FILE" \
  "/^#include.*$/d;"

# Functions exposed by minion_lvdpll_program.h directly
FILE="minion_lvdpll_program.c"
UPDATE_FILE "$SRC_SW_IP_DIR/src/$FILE"  "$ETSOC_HAL_HOME/src/$FILE" \
  "/^#include.*$/d;"

# Patch locally to expose minion_lvdpll_program.h directly
FILE="minion_lvdpll_program.h"
UPDATE_FILE "$SRC_SW_IP_DIR/inc/$FILE"  "$ETSOC_HWINC/$FILE" \
  "s/^#include[[:space:]]+\"esr_sw.h\"/#include \"esr.h\"/;                                                          \
   s/^#include[[:space:]]+\"esr_region_sw.h\"/#include \"hwinc\/esr_region.h\"/;                                     \
   s/^#include[[:space:]]+\"etsoc_shire_other_esr.h\"/#include \"hwinc\/etsoc_shire_other_esr.h\"/;                  \
   s/^#include[[:space:]]+\"lvdpll_defines.h\"/#include \"hwinc\/lvdpll_defines.h\"/;                                \
   s/^#include[[:space:]]+\"movellus_lvdpll_modes_config.h\"/#include \"hwinc\/lvdpll_modes_config.h\"/;             \
   s/^#include[[:space:]]+\"dvfs_movellus_lvdpll_modes_config.h\"/#include \"hwinc\/dvfs_lvdpll_modes_config.h\"/;   \
  "
