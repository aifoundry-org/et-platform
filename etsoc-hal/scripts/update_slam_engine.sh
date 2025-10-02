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

SRC_SLAM_ENGINE_DIR="$SLAM_ENGINE_ROOT"

SCRIPT_HOME="$(dirname $0)"
source "$SCRIPT_HOME/helper.sh"
INIT_ENV_BASE "slam_engine" "$SLAM_ENGINE_ROOT" || exit 1

FILE="slam_engine.h"
UPDATE_FILE "$SRC_SLAM_ENGINE_DIR/$FILE"  "$ETSOC_HAL_HOME/include/$FILE"

FILE="slam_engine.c"
UPDATE_FILE "$SRC_SLAM_ENGINE_DIR/$FILE"  "$ETSOC_HAL_HOME/src/$FILE"

