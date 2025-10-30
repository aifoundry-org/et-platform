# ET Platform Docker Build Environment
# This Dockerfile creates a complete build environment for the ET Platform
# including the RISC-V GNU toolchain and all necessary dependencies.

FROM ubuntu:24.04

# Prevent interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Set up the installation directory
ENV ET_INSTALL_DIR=/opt/et
ENV PATH="${ET_INSTALL_DIR}/bin:${PATH}"

# Install base dependencies for RISC-V toolchain
RUN apt-get update && apt-get install -y \
    autoconf \
    automake \
    autotools-dev \
    curl \
    python3 \
    python3-pip \
    python3-tomli \
    libmpc-dev \
    libmpfr-dev \
    libgmp-dev \
    gawk \
    build-essential \
    bison \
    flex \
    texinfo \
    gperf \
    libtool \
    patchutils \
    bc \
    zlib1g-dev \
    libexpat-dev \
    dos2unix \
    ninja-build \
    git \
    cmake \
    libglib2.0-dev \
    libslirp-dev \
    && rm -rf /var/lib/apt/lists/*

# Install ET Platform build dependencies
RUN apt-get update && apt-get install -y \
    pkg-config \
    libjson-c-dev \
    libgtest-dev \
    libgoogle-glog-dev \
    libgflags-dev \
    libgmock-dev \
    lz4 \
    liblz4-dev \
    libboost-all-dev \
    libcap-dev \
    libcap2-bin \
    doxygen \
    graphviz \
    nlohmann-json3-dev \
    python3-sphinx \
    python3-sphinx-rtd-theme \
    python3-breathe \
    python3-jsonschema \
    libfmt-dev \
    xxd \
    && rm -rf /var/lib/apt/lists/*

# Create the ET install directory
RUN mkdir -p ${ET_INSTALL_DIR} && chmod 755 ${ET_INSTALL_DIR}

# Build and install the RISC-V GNU Toolchain
WORKDIR /tmp/toolchain
RUN git clone https://github.com/aifoundry-org/riscv-gnu-toolchain && \
    cd riscv-gnu-toolchain && \
    ./configure \
        --prefix=${ET_INSTALL_DIR} \
        --with-arch=rv64imfc \
        --with-abi=lp64f \
        --with-languages=c,c++ \
        --with-cmodel=medany && \
    make -j$(nproc) && \
    cd /tmp && \
    rm -rf /tmp/toolchain

# Set working directory for the ET Platform source
WORKDIR /workspace

# Copy the ET Platform source code
COPY . /workspace/

# Normalize line endings for scripts copied from Windows hosts
RUN find /workspace -type f -name '*.py' -exec dos2unix {} +

# Create build directory
RUN mkdir -p /workspace/build

# Configure the build
WORKDIR /workspace/build
RUN cmake \
    -DTOOLCHAIN_DIR=${ET_INSTALL_DIR} \
    -DCMAKE_INSTALL_PREFIX=${ET_INSTALL_DIR} \
    ..

# Build the ET Platform (use ARG to allow override)
ARG BUILD_JOBS=4
RUN make -j${BUILD_JOBS}

# Set the default working directory
WORKDIR /workspace

# Default command: run a shell
CMD ["/bin/bash"]

# Labels for metadata
LABEL maintainer="ET Platform Team"
LABEL description="ET Platform Build Environment with RISC-V Toolchain"
LABEL version="1.0"
