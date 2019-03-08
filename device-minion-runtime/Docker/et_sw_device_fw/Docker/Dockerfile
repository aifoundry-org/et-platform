#syntax=docker/dockerfile:experimental
#------------------------------------------------------------------------------
# Copyright (C) 2019, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

ARG ET_SW_BASE=828856276785.dkr.ecr.us-west-2.amazonaws.com/et-sw-base:latest

FROM $ET_SW_BASE

LABEL Description="This image contains any build dependencies for the SW Platform project"

# Install json-c-devel library
RUN yum install -y --setopt=skip_missing_names_on_install=False \
    json-c-devel \
    openssl-devel \
    python36-devel \
  && yum clean all

RUN pip install \
    pyelftools \
    bitstring

ARG GIT_SHA
LABEL git_sha="$GIT_SHA"

COPY ./entrypoint.sh  /

ENTRYPOINT [ "/entrypoint.sh" ]
