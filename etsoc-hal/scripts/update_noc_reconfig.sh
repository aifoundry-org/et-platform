#!/bin/bash
# Copyright (c) 2025 Ainekko, Co.
# SPDX-License-Identifier: Apache-2.0

SRC_DIR="$REPOROOT/dv/tests/ioshire/sw"

SCRIPT_HOME="$(dirname $0)"
source "$SCRIPT_HOME/helper.sh"
INIT_ENV || exit 1

FILE="noc_reconfig_memshire.c"
UPDATE_FILE "$SRC_DIR/inc/$FILE"  "$ETSOC_HAL_HOME/src/$FILE" \
  "/^#include.*$/d; \
  "

FILE="noc_reconfig_minshire.c"
UPDATE_FILE "$SRC_DIR/inc/$FILE"  "$ETSOC_HAL_HOME/src/$FILE" \
  "/^#include.*$/d; \
  "

FILE="noc_reconfig_pshire.c"
UPDATE_FILE "$SRC_DIR/inc/$FILE"  "$ETSOC_HAL_HOME/src/$FILE" \
  "/^#include.*$/d; \
  "#
