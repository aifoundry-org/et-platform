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

# Install Docker so that we can build the client#1 inside a container from this image
RUN yum-config-manager \
    --add-repo \
    https://download.docker.com/linux/centos/docker-ce.repo


# Add gitlab and barcino do the list of known hosts
RUN mkdir -p -m 0700 ~/.ssh && ssh-keyscan barcino >> ~/.ssh/known_hosts \
    && mkdir -p -m 0700 ~/.ssh && ssh-keyscan gitlab >> ~/.ssh/known_hosts


# Minion riscv toolchain

ENV RISCV=/esperanto/minion

RUN --mount=type=ssh cd /tmp / \
    && git clone --depth 1 --shallow-submodules git@barcino:esperanto/riscv-gnu-toolchain-fork.git \
    && cd riscv-gnu-toolchain-fork \
    && git submodule update --depth=50  --init --recursive riscv-binutils riscv-gcc riscv-glibc riscv-newlib \
    && cd /tmp/riscv-gnu-toolchain-fork \
    && mkdir -p build \
    && cd build \
    && ../configure --prefix="$RISCV" \
            --with-arch=rv64imaf \
            --with-abi=lp64f \
            --disable-multilib \
            --with-cmodel=medany \
            --disable-gcc-checking \
            --disable-gdb \
    && make -j $(nproc) \
    && make -j $(nproc) install \
    && cd .. \
    && mkdir -p build_bfd_opcodes \
    && cd build_bfd_opcodes \
    && CC_FOR_TARGET=riscv64-unknown-elf-gcc ../riscv-binutils/configure \
            --target=riscv64-unknown-elf \
            --prefix="$RISCV" \
            --disable-werror \
            --with-expat=yes \
            --enable-shared \
            --enable-install-libbfd \
            --enable-install-libiberty \
            --disable-gdb \
            --disable-sim \
            --disable-libdecnumber \
            --disable-libreadline \
     && make -j $(nproc) all-opcodes all-libiberty all-bfd \
     && make install-opcodes install-libiberty install-bfd \
     && rm -rf /tmp/*

RUN pip install pyelftools

ARG GIT_SHA
LABEL git_sha="$GIT_SHA"

COPY ./entrypoint.sh  /

ENTRYPOINT [ "/entrypoint.sh" ]
