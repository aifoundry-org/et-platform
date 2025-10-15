#!/bin/bash
# Copyright (c) 2025 Ainekko, Co.
# SPDX-License-Identifier: Apache-2.0

# $SCRIPT_HOME is required before 'source' this file
if [ -z "$SCRIPT_HOME" ]; then
  echo "Need SCRIPT_HOME to work properly"
  exit 1
fi

LICENSE_HEADER="$SCRIPT_HOME/license_header.h"

# Init environment, bare version
# Input:
#   $1: REPOSITORY_NAME
#   $2: REPOSITORY_PATH
# Export:
#   ETSOC_HAL_HOME:   path to etsoc_hal
#   ETSOC_HWINC:      path to hwinc
#   GIT_COMMIT:       git commit hash of the repository
#   REPOSITORY_PATH:  path to the repository
#   REPOSITORY_NAME:  name of the respository
INIT_ENV_BASE()
{
  if [ -n "$REPOSITORY_PATH" ] || [ -n "$REPOSITORY_NAME" ]; then
    echo "INIT_ENV_BASE can only run once for the main repository!"
    echo "  Existing main repository is         $REPOSITORY_NAME($REPOSITORY_PATH)"
    echo "  Intending to set main repository to $2($1)"
    return 1
  fi

  REPOSITORY_NAME="$1"
  REPOSITORY_PATH="$2"

  if [ -z "$REPOSITORY_NAME" ]; then
    echo "Please supply correct arguments for INIT_ENV_BASE!"
    return 1
  fi

  if [ ! -f "etsoc_halConfig.cmake.in" ]; then
    echo "Please run this script in etsoc_hal root directory!"
    return 1
  fi
  ETSOC_HAL_HOME=$(pwd)
  ETSOC_HWINC="$ETSOC_HAL_HOME/include/hwinc"

  if [ -z "$REPOSITORY_PATH" ] || [ ! -d "$REPOSITORY_PATH" ]; then
    echo "Please run '. ./env.sh' in $REPOSITORY_NAME repository first!"
    return 1
  fi

  cd "$REPOSITORY_PATH"
  GIT_COMMIT=$(git rev-parse HEAD)
  cd $OLDPWD

  return 0
}

# Init environment, default to esperanto-soc repository
# Export:
#   ETSOC_HAL_HOME:   path to etsoc_hal
#   ETSOC_HWINC:      path to hwinc
#   GIT_COMMIT:       git commit hash of the repository
#   REPOSITORY_PATH:  path to the repository
#   REPOSITORY_NAME:  name of the respository
INIT_ENV()
{
  INIT_ENV_BASE "esperanto-soc" "$REPOROOT"
  return $?
}

# Only update if the content is different
# Inject license header and commit hash
#
# $n: Switches
#     --no_git_checkout     Do not checkout/reset from git
#     --no_header           Do not add license header
# $n+1: Source file
# $n+2: Destination file
# $n+3: Arguments for sed, optional
UPDATE_FILE()
{
  GIT_CHECKOUT=true
  ADD_HEADER=true
  while [ -n "$1" ] && [ "${1:0:2}" == "--" ];
  do
    if [ "$1" == "--no_git_checkout" ]; then
      GIT_CHECKOUT=false
    elif [ "$1" == "--no_header" ]; then
      ADD_HEADER=false
    fi
    shift
  done

  SRC_FILE="$1"
  DST_FILE="$2"
  SED_ARGS="$3"

  if [ ! -f "$SRC_FILE" ]; then
    echo "Source not found: $SRC_FILE"
    return 1
  fi

  # patch SRC_FILE into NEW_FILE if required
  NEW_FILE=$(mktemp)
  if [ -n "$SED_ARGS" ]; then
    cat "$SRC_FILE" | sed -r 's/[[:space:]]*$//' | sed -r "$SED_ARGS" > "$NEW_FILE"
  else
    cat "$SRC_FILE" | sed -r 's/[[:space:]]*$//' > "$NEW_FILE"
  fi

  # produce OLD_FILE from DST_FILE
  OLD_FILE=$(mktemp)
  touch "$DST_FILE"
  if $GIT_CHECKOUT; then
    git checkout -q "$DST_FILE" > /dev/null 2>&1
  fi
  N_LINES_TO_SKIP=1   # 1-based for tail -n
  if $ADD_HEADER; then
    N_LINES_HEADER=$(wc -l $ETSOC_HAL_HOME/$LICENSE_HEADER | cut -d' ' -f1)
    N_LINES_TO_SKIP=$(($N_LINES_TO_SKIP + $N_LINES_HEADER))
  fi
  N_LINES_TO_SKIP=$(($N_LINES_TO_SKIP + 1))    # commit line
  tail -n +$N_LINES_TO_SKIP "$DST_FILE" > $OLD_FILE

  # check if the file is changed before updating
  diff $OLD_FILE $NEW_FILE > /dev/null
  if [ $? -ne 0 ]; then
    echo "Updating $DST_FILE"
    rm "$DST_FILE"
    if $ADD_HEADER; then
      cat "$ETSOC_HAL_HOME/$LICENSE_HEADER" >> "$DST_FILE"
    fi
    echo "// From commit $GIT_COMMIT in $REPOSITORY_NAME repository" >> "$DST_FILE"
    cat "$NEW_FILE" >> "$DST_FILE"
  else
    echo "Up-to-date: $DST_FILE"
  fi

  rm $NEW_FILE
  rm $OLD_FILE
}
