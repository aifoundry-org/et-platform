#!/bin/bash
# Copyright (c) 2025 Ainekko, Co.
# SPDX-License-Identifier: Apache-2.0

HWINC_DIR="include/hwinc"
SRC_DIR_REGSPEC="$REGSPEC_ROOT/et_soc_a0_regs/inc"
SRC_DIR_HAL_DEVICE="$REPOROOT/dv/tests/ioshire/sw/inc"

GIT_REV_FILE="$HWINC_DIR/git_rev.txt"

# $1: path to the file to copy
CP_FILES()
{
  echo $1
  DST_FILE="$HWINC_DIR/$(basename $1)"

  if [ ! -f "$1" ]; then
    if [ -h "$1" ]; then
      echo "$1 is a broken symbolic link, skipped"
    else
      echo "$1 not found, skipped"
    fi
    return
  fi

  cp -L "$1" "$DST_FILE"
  chmod a-x "$DST_FILE"
}

SCRIPT_HOME="$(dirname $0)"
source "$SCRIPT_HOME/helper.sh"
INIT_ENV || exit 1

if [ -z "$REGSPEC_ROOT" ] || [ ! -d "$REGSPEC_ROOT" ]; then
  echo "Please run '. ./env.sh in regspec first'"
  exit 1
fi

cd "$REGSPEC_ROOT"
GIT_COMMIT_REGSPEC=$(git rev-parse HEAD)
cd $OLDPWD

echo "Checking files to copy..."
rm -f "$ETSOC_HAL_HOME/$HWINC_DIR"/*.h

# Whitelist copying files
for FILE in "$SRC_DIR_REGSPEC"/*.h
do
  CP_FILES "$FILE"
done

CP_FILES "$SRC_DIR_HAL_DEVICE/hal_device.h"
CP_FILES "$SRC_DIR_HAL_DEVICE/esr_region.h"
CP_FILES "$SRC_DIR_HAL_DEVICE/local_interrupts.h"
CP_FILES "$SRC_DIR_HAL_DEVICE/pu_plic_intr_device.h"
CP_FILES "$SRC_DIR_HAL_DEVICE/sp_exceptions.h"
CP_FILES "$SRC_DIR_HAL_DEVICE/spio_plic_intr_device.h"

echo "// From commit $GIT_COMMIT_REGSPEC in regspec repo" > "$ETSOC_HAL_HOME/$GIT_REV_FILE"
echo "// From commit $GIT_COMMIT in esperanto-soc repo" >> "$ETSOC_HAL_HOME/$GIT_REV_FILE"
echo "Sync up to commit id: $GIT_COMMIT_REGSPEC for regspec"
echo "Sync up to commit id: $GIT_COMMIT for esperanto-soc"
echo
echo "Update all other files in addition to regspec"
$SCRIPT_HOME/update_ddr_init.sh
$SCRIPT_HOME/update_pll_tables.sh
$SCRIPT_HOME/update_minion_pll.sh
$SCRIPT_HOME/update_noc_reconfig.sh
echo "Done."
echo "Please do a 'git status' to review the changes before commit and push"
