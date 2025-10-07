ARG BASE_OS=ubuntu
ARG BASE_VERSION=24.04

FROM et-riscv-gnu-toolchain:${BASE_OS}-${BASE_VERSION} AS build
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential cmake pkg-config git \
    libjson-c-dev \
    libgtest-dev \
    libgoogle-glog-dev libgflags-dev libgmock-dev \
    lz4 liblz4-dev \
    libboost-all-dev \
    libcap-dev libcap2-bin \
    doxygen graphviz \
    nlohmann-json3-dev \
    python3-sphinx python3-sphinx-rtd-theme python3-breathe python3-jsonschema \
    libfmt-dev \
    xxd

WORKDIR /workspace
COPY . .

RUN mkdir -p /build

WORKDIR /build
RUN cmake /workspace \
    -DTOOLCHAIN_DIR=/opt/et \
    -DCMAKE_INSTALL_PREFIX=/opt/et \
    -DCMAKE_BUILD_TYPE=Release

RUN cmake --build . -j${nproc}
RUN cmake --install .

FROM ${BASE_OS}:${BASE_VERSION}

RUN apt-get update && apt-get install -y \
       libfmt[0-9]+ \
       libgoogle-glog[0-9]* \
       libgflags[0-9]+ \
    && rm -rf /var/lib/apt/lists/*

COPY --from=build /opt/et /opt/et

RUN echo "/opt/et/lib" > /etc/ld.so.conf.d/et-libs.conf \
    && ldconfig

ENV PATH="/opt/et/bin:${PATH}"
