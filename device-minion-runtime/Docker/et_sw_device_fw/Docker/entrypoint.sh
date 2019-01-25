#!/bin/bash

#------------------------------------------------------------------------------
# Copyright (C) 2019, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------


# set -x

if [[ -z "$UID" ]]; then
    echo Undefined venv "$UID"
    exit 1
fi;

if [[ -z "$GID" ]]; then
    echo Undefined venv "$GID"
    exit 1
fi;

if [[ -z "$DOCKER_GID" ]]; then
    echo Undefined venv "$DOCKER_GID"
    exit 1
fi;

if [[ -z "$USERNAME" ]]; then
    echo Undefined vevn "$USERNAME"
    exit 1
fi;

if [[ -z "$SRC_DIR" ]]; then
    echo Undefined vevn "$SRC_DIR"
    exit 1
fi;

HOME_ARG="-M "
if [[ -z "$CREATE_HOME" ]]; then
    echo Undefined vevn "$CREATE_HOME"
    exit 1
else
    HOME_ARG="-m -d /home/$USERNAME"
fi;

groupadd  --gid $GID $USERNAME
# Crete a docker group that matches that on the host, necessary for talking
# with the docker daemon, remove it if it already exists
groupdel docker
groupadd -o --gid $DOCKER_GID docker
useradd --uid $UID --gid $GID $USERNAME -p test ${HOME_ARG}
usermod -aG docker $USERNAME

echo "$USERNAME ALL=(ALL) NOPASSWD: ALL" > /etc/sudoers.d/$USERNAME

cd $SRC_DIR

source scl_source enable devtoolset-7

PATH=$PATH:$SRC_DIR:$SRC_DIR/bin:$SRC_DIR/scripts:/esperanto/bin:$SRC_DIR/infra_tools/aws_helpers

# If prompt defined drop the user to the prompt
if [[ -n "$PROMPT" ]]; then
    exec gosu $USERNAME bash
    exit 0;
fi;

if [[ -n "$AWS" ]]; then
    exec gosu $USERNAME /entrypoint-aws-chpt-regress.sh $1 $2 $3 "$4"
else
    # Execute the rest of the commands as is
    exec gosu $USERNAME bash -c "$@"
fi
