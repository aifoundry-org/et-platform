# ET Platform Docker Build Environment
# This Dockerfile creates a complete build environment for the ET Platform
# including the RISC-V GNU toolchain and all necessary dependencies.

ARG BASE_DISTRO=ubuntu
ARG DISTRO_VERSION=24.04
FROM ${BASE_DISTRO}:${DISTRO_VERSION} AS toolchain_builder

ARG DISTRO_VERSION
ARG BASE_DISTRO
ARG ET_INSTALL_DIR=/opt/et
ARG ETARCH=rv64imfc
ARG ETABI=lp64f
ARG FORCE_REBUILD=false

# Prevent interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Set up the installation directory
ENV ET_INSTALL_DIR=${ET_INSTALL_DIR}

# Create the ET install directory
RUN mkdir -p ${ET_INSTALL_DIR} && chmod 755 ${ET_INSTALL_DIR}

# Download or build and install the RISC-V GNU Toolchain
COPY ./docker/get_toolchain.sh /get_toolchain.sh
RUN DEBUG=1 /get_toolchain.sh --force ${FORCE_REBUILD} --install-deps true --repo aifoundry-org/riscv-gnu-toolchain --install-dir ${ET_INSTALL_DIR} --base-distro ${BASE_DISTRO} --distro-version ${DISTRO_VERSION} --arch ${ETARCH} --abi ${ETABI}

FROM ${BASE_DISTRO}:${DISTRO_VERSION} AS etplatform_builder
ARG ET_INSTALL_DIR=/opt/et

# Copy the RISC-V GNU Toolchain from the previous stage
COPY --from=toolchain_builder ${ET_INSTALL_DIR} ${ET_INSTALL_DIR}

# Install ET Platform build dependencies
RUN apt-get update && apt-get install -y \
    cmake \
    dos2unix \
    gcc \
    g++ \
    git \
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

# Set working directory for the ET Platform source
WORKDIR /workspace

# Copy the ET Platform source code
COPY . .

# Normalize line endings for scripts copied from Windows hosts
RUN find . -type f -name '*.py' -exec dos2unix {} +

# Create build directory
RUN mkdir -p ./build

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
