# ET Platform

This is the main repository for the AINekko's ET platform.

AINekko's ET is an open-source manycore platform for parallel
computing acceleration.  It is built on the legacy of Esperanto
Technologies ET-SOC1 chip.

On first approximation, the ET Platform is a RISCV, manycore architecture.

The ETSOC1 contains 1088 compute cores (called _minions_). Each minion has
two `rv64imfc` RISCV HARTs with vendor-specific vector and tensor extensions.

There's an extra RISCV core on board, called the Service Processor, that is used
for chip bring up and runtime management.

For a full understanding of the ETSOC1 architecture check the [ETSOC1 Programmer's Reference Manual](https://github.com/aifoundry-org/et-man/blob/main/ET%20Programmer's%20Reference%20Manual.pdf).

## What's in this repo?

This repo contains the full compute acceleration platform, from
host runtime library to firmware running inside the device.

The main software components are:

  - [`esperanto-tools-libs`](https://github.com/aifoundry-org/et-platform/tree/master/esperanto-tools-libs): The Host Runtime Library.
  - [`device-layer`](https://github.com/aifoundry-org/et-platform/tree/master/devicelayer): The Device Abstraction Library.
  - [`sw-sysemu`](https://github.com/aifoundry-org/et-platform/tree/master/sw-sysemu): A Software Simulator for the ETSOC1.
  - [`et-driver`](https://github.com/aifoundry-org/et-platform/tree/master/et-driver): The ETSOC1 Linux Kernel Driver.
  - [`device-bootloaders`](https://github.com/aifoundry-org/et-platform/tree/master/et-driver): The Service Processor Firmware.
  - [`device-api`](https://github.com/aifoundry-org/et-platform/tree/master/device-api): The low level Interface between Host and ETSOC1.
  - [`device-minion-runtime`](https://github.com/aifoundry-org/et-platform/tree/master/device-minion-runtime): Machine and Supervisor Mode Software running on compute minions.
  - [`device-management-application`](https://github.com/aifoundry-org/et-platform/tree/master/device-management-application): Utilities to manage and monitor the ET Platform.

## How to build ET Platform

### 1. Create install directory

Start by creating a user writable `/opt/et`.

```
  $ sudo mkdir /opt/et
  $ sudo chown $(whoami) /opt/et
```

### 2. Install the ET RISCV GNU Toolchain

The next step is installing the full toolchain for _minions_ (gcc + newlib).

On Ubuntu 24.04, the required packages are:

```
  $ sudo apt-get install autoconf automake autotools-dev curl python3 python3-pip python3-tomli libmpc-dev libmpfr-dev libgmp-dev gawk build-essential bison flex texinfo gperf libtool patchutils bc zlib1g-dev libexpat-dev ninja-build git cmake libglib2.0-dev libslirp-dev
```

Once all the required libraries are installed, you can proceed to build the toolchain:

```
  $ git clone https://github.com/aifoundry-org/riscv-gnu-toolchain
  $ cd riscv-gnu-toolchain
  $ ./configure --prefix=/opt/et --with-arch=rv64imfc --with-abi=lp64f \
                  --with-languages=c,c++ --with-cmodel=medany
  $ make -j $(nproc)
  $ cd ..
```

This will install `riscv-unknown-elf-gcc` and other toolchain utilities in `/opt/et/bin`

### 3. Build the ET Platform.

Now that the ET RISCV Toolchain is installed in `/opt/et`, we can proceed to compiling the full ET-Platform.

Start by installing the required packages. For Ubuntu 24.04, the command is:

```
  $ sudo apt-get install
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
```

Then, it is time to fetch and build:

```
  $ git clone git@github.com:aifoundry-org/et-platform.git
  $ mkdir et-platform/build
  $ cd et-platform/build
  $ cmake -DTOOLCHAIN_DIR=/opt/et -DCMAKE_INSTALL_PREFIX=/opt/et ..
  $ make -j $(nproc)
```

This will compile all the ET-Platform software and install it in `/opt/et`.

### 4. Run a simple test.

Try to run a simple test with:

```
  $ /opt/et/bin/it_test_code_loading
```

This will load and execute kernels on minions, using the software simulator.

