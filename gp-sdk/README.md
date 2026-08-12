# Description

This project contains General Purpose Sdk specific code and tools. it involves  the needed material (libs, docs, exampless), to allow a customer to:
* Write, debug and profile a standalone compute Kernel 
* Execute on ET-SoC-1
* Recover results

for further information, please review companion documentation provided in docs folder.

## Building

In order to build you need to use a docker image:
```
et_docker --develop --network=host prompt
```
This image contains the et sw stack allowing buliding GP-SDK from sources.

Conceptually the build phase can be divided in:

1. Building device kernel code
```


export DEV_COMPILER=gcc8.2 #available options are gcc8.2 and clang11
mkdir device/build
cd device/build
cmake .. -DCMAKE_TOOLCHAIN_FILE=/usr/local/et/.builds/device/${DEV_COMPILER}/conan_toolchain.cmake -DADDRESS:STRING=0x8006335000  -DCMAKE_BUILD_TYPE=Release

make all
```
2. Building host examples

```
cd host/build
cmake .. -DCMAKE_TOOLCHAIN_FILE=/usr/local/et/.builds/host/conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Debug
make all

``` 

## Executing

### Preparing host runtime env.

Executing involves launching a host application that receives as a parameter the computational kernel to be executed on the device. Please, note that we need to activate the virtual conan environment for Debug or Release on the host accordignly with the BuildType used. The docker entry point by default activates Relase runtime environment, that needs explicit deactivation:

```
source  ${ET_SDK_HOME}/.builds/host/deactivate_conanrunenv-release-x86_64.sh
source  ${ET_SDK_HOME}/.builds/host/conanrunenv-debug-x86_64.sh
```

Also, if we transition from a Debug env to a Release env, we need:

```
source  ${ET_SDK_HOME}/.builds/host/deactivate_conanrunenv-debug-x86_64.sh
source  ${ET_SDK_HOME}/.builds/host/conanrunenv-release-x86_64.sh
```

#### Executing.

```
sdk/hello_world_launcher --kernel_path=../../device/build/tests/print.elf --device_type=sysemu
sdk/hello_world_launcher --kernel_path=../../device/build/tests/print.elf --device_type=silicon
```

When using `--device_type=sysemu`, the launcher will prefer the sibling `*_dbg` ELF when it exists. Those
debug-linked images are linked at the runtime-predicted device address and are the correct choice for SYSEMU
bring-up and debugging.

and we can see  traces from the device through dt2json application installed on the docker image:

```
dt2json traceKernels_dev0_0.bin -t
(...)
12362222;string;{plain_string};{"entryPoint_0,24 HELLO WORLD!!!!"}
```

## Using the Top-level CMakeLists.txt

A top level makefile has also been provided on the root that is able to configure and build host and device parts on a cmake & make command. Both host and device CMake domains are introduced through ExternalProcectAdd().

```

export DEV_COMPILER={gcc8.2,clang11} 
mkdir build; cd build
cmake .. -DADDRESS:STRING=0x8006335000
make all
```


## Building an external-project that uses GP-SDK as a library

It is common to have standalone projects using GP-SDK as an external dependency. This can be achieved by introducing a top-level CMakeLists.txt into the project and setting GP_SDK_HOME env var to the approriate location


### Top level CMakeLists.txt
```
set(GP_SDK_HOME  ""  CACHE STRING "Path to the GP-SDK")

 ....
include(ExternalProject)

ExternalProject_Add(
  host
  SOURCE_DIR "${CMAKE_SOURCE_DIR}/host"
  CMAKE_ARGS
    -DCMAKE_TOOLCHAIN_FILE=$ENV{ET_SDK_HOME}/.builds/host/conan_toolchain.cmake
    -DCMAKE_BUILD_TYPE=Release
    -DGP_SDK_HOME=${GP_SDK_HOME}
    -DBUILD_TESTS=OFF
    -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/local)

ExternalProject_Add(
  device
  SOURCE_DIR "${CMAKE_SOURCE_DIR}/device"
  CMAKE_ARGS
    -DCMAKE_TOOLCHAIN_FILE=$ENV{ET_SDK_HOME}/.builds/device/$ENV{DEV_COMPILER}/conan_toolchain.cmake
    -DCMAKE_BUILD_TYPE=Release
    -DGP_SDK_HOME=${GP_SDK_HOME}
    -DADDRESS:STRING=${ADDRESS}
    -DBUILD_TESTS=OFF
    -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/local)
```

### host CMakeLists.txt

```
add_subdirectory(${GP_SDK_HOME}/host gp-sdk-host)
add_executable(saxpy_launcher saxpy.cpp)
install(TARGETS saxpy_launcher DESTINATION bin)
```


### device CMakeLists.txt

```
add_subdirectory(${GP_SDK_HOME}/device gp-sdk-device)
add_etsoc_riscv_executable(saxpy_scalar.elf saxpy.cpp)
target_include_directories(saxpy_scalar.elf PRIVATE ../include)
target_link_libraries(saxpy_scalar.elf etsoc_crt0)
  ....
```

### Configuing and building

```
export DEV_COMPILER={gcc8.2,clang11}
mkdir build; cd build
cmake .. -DGP_SDK_HOME=<path_to-gp-sdk> -DADDRESS=0x8006335000
make


## Generating Doxygen documentation

Doxygen packages are not installed by default in the SDK docker. If needed, install them with:

### Required third-party dependencies

```
sudo apt update
sudo apt install texlive-latex-base texlive-fonts-recommended texlive-fonts-extra texlive-latex-extra
sudo apt install doxygen
```

### Generate pdf

Once the required dependencies are installed, perform the 'Building' steps the gp-sdk as explained in a previous section.

```
cd <path_to-gp-sdk>/docs/host/build/
make; cd latex; make
cd <path_to-gp-sdk>/docs/device/build/
make; cd latex; make
```

New documentation will appear as refman.pdf inside each host/build and device/build folders.

                                                  





