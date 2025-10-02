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

DDR_INIT_FILE="src/memshire_ddr_init_functions.c"
SRC_DIR_MS_HEADERS="$REPOROOT/dv/tests/memshire/tcl_tests/tcl_scripts/lib/"

SCRIPT_HOME="$(dirname $0)"
source "$SCRIPT_HOME/helper.sh"
INIT_ENV || exit 1

# PARSE_TCL
# $1: tcl file
# $2: result header file
function PARSE_TCL_DEF()
{
  TMP_FILE=$(mktemp --suffix=.h)
  cat "$1" | awk '{                                                       \
    if($0 ~ "^[[:blank:]]*$")                                             \
      print "";                                                           \
    else if($0 ~ "^#.*") {                                                \
      print "//"substr($0, 2);                                            \
    }                                                                     \
    else {                                                                \
      match($0, /set.*\((.*)\)([[:blank:]]+)(0x[0-9|a-f|A-F]+)[[:blank:]]*;/, arr);   \
      print "#define "arr[1]arr[2]arr[3];                                 \
    }                                                                     \
  }' > $TMP_FILE
  UPDATE_FILE $TMP_FILE "$2"
  rm -f $TMP_FILE
}

PARSE_TCL_DEF "$SRC_DIR_MS_HEADERS/ddrc_reg_def.tcl" "$ETSOC_HAL_HOME/include/hwinc/ddrc_reg_def.h"
PARSE_TCL_DEF "$SRC_DIR_MS_HEADERS/ms_reg_def.tcl"   "$ETSOC_HAL_HOME/include/hwinc//ms_reg_def.h"

NAKED_FILE=$(mktemp --suffix=.c)
ANNOTATED_FILE=$(mktemp --suffix=.c)
${REPOROOT}/test/scripts/parse_pcode.pl -c ${REPOROOT}/dv/tests/memshire/pcode/ddr_init_functions.pcode > $NAKED_FILE
${REPOROOT}/test/scripts/annotate_ddrc_regs.pl $NAKED_FILE > $ANNOTATED_FILE
UPDATE_FILE "$ANNOTATED_FILE" "$ETSOC_HAL_HOME/$DDR_INIT_FILE"
rm $NAKED_FILE $ANNOTATED_FILE
