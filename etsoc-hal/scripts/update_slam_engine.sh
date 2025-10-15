#!/bin/bash
# Copyright (c) 2025 Ainekko, Co.
# SPDX-License-Identifier: Apache-2.0

SRC_SLAM_ENGINE_DIR="$SLAM_ENGINE_ROOT"

SCRIPT_HOME="$(dirname $0)"
source "$SCRIPT_HOME/helper.sh"
INIT_ENV_BASE "slam_engine" "$SLAM_ENGINE_ROOT" || exit 1

FILE="slam_engine.h"
UPDATE_FILE "$SRC_SLAM_ENGINE_DIR/$FILE"  "$ETSOC_HAL_HOME/include/$FILE"

FILE="slam_engine.c"
UPDATE_FILE "$SRC_SLAM_ENGINE_DIR/$FILE"  "$ETSOC_HAL_HOME/src/$FILE"

