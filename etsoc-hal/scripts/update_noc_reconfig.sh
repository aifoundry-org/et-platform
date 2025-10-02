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
