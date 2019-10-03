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
    bitstring \
    xxhash

ENV OPENSSL=1.1.1b

# Install OpenSSL
RUN cd /tmp \
  && wget https://www.openssl.org/source/openssl-${OPENSSL}.tar.gz \
  && tar zxvf openssl-${OPENSSL}.tar.gz  \
  && cd openssl-${OPENSSL} \
  && ./config --prefix=/usr  \
  && make \
  && make install_sw \
  && rm -rf /tmp/*

# Install Utimaco tools
RUN cd /tmp \
  && wget http://sc-artifactory1.esperanto.ai/artifactory/sw-ci-inputs/tools/SecurityServer-V4.21.1.0.zip \
  && unzip SecurityServer-V4.21.1.0.zip \
  && chmod -R a+r Software/Linux/* \
  && chmod a+x Software/Linux/x86-64/Administration/csadm \
  && chmod a+x Software/Linux/x86-64/Crypto_APIs/CXI/bin/cxitool \
  && chmod a+x Software/Linux/x86-64/Crypto_APIs/PKCS11_R2/bin/p11tool2 \
  && chmod a+x Software/Linux/x86-32/Administration/csadm \
  && chmod a+x Software/Linux/x86-32/Crypto_APIs/CXI/bin/cxitool \
  && chmod a+x Software/Linux/x86-32/Crypto_APIs/PKCS11_R2/bin/p11tool2 \
  && mkdir /opt/Utimaco \
  && cp -R Software/Linux/x86-64/* /opt/Utimaco \
  && rm -rf /tmp/*

ARG GIT_SHA
LABEL git_sha="$GIT_SHA"

COPY ./entrypoint.sh  /

ENTRYPOINT [ "/entrypoint.sh" ]
