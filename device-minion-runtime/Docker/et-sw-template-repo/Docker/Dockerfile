#------------------------------------------------------------------------------
# Copyright (C) 2019, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------


FROM ubuntu:18.04

LABEL Description="This image contains any build dependencies for the SW-template porject"

# Basic system dependencies
RUN apt-get update \
  && apt-get install -y \
     git=1:2.17.1-1ubuntu0.4 \
     gosu \
     python3.6 \
     python3-pip \
     python-pip \
     sudo \
  && rm -rf /var/lib/apt/lists/*


# Install other required python packages
RUN cd /tmp/ \
  && python3 -m pip install \
    junit-xml==1.8 \
    pexpect==4.6 \
    pytest==3.8.2 \
    boto3==1.9.25 \
    checksumdir==1.1.4 \
  # Python2 specific packages
  && pip install \
     awscli==1.16.34

# Install project specific project specific build dependencies
RUN apt-get update \
  && apt-get install -y \
    cmake=3.10.2-1ubuntu2 \
    doxygen=1.8.13-10 \
    clang-format-6.0 \
    clang-tidy-6.0 \
    graphviz \
  && rm -rf /var/lib/apt/lists/*


RUN update-alternatives --install /usr/bin/clang-format clang-format /usr/bin/clang-format-6.0 1 \
    --slave /usr/bin/clang-format-diff clang-format-diff /usr/bin/clang-format-diff-6.0 \
    --slave /usr/bin/clang-tidy clang-tidy /usr/bin/clang-tidy-6.0


ARG GIT_SHA
LABEL git_sha="$GIT_SHA"

COPY ./entrypoint.sh  /

ENTRYPOINT [ "/entrypoint.sh" ]
