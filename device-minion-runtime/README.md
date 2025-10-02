Device Firmware Project  {#device_firmware}
=======================

This project implements the device firmware running on the Esperanto SOC

The system architecture is described in detail @subpage device_firmware_system_architecture

## WARNING

Currently this repo is not self contained and it should be built as part of the
sw-platform build flow

## Build the documentation

Do the following steps to build "something" i.e. the documentation:

    mkdir -p build && cd build
    cmake ..
    make doc

The generated Doxygen documentation should be under folder:

    build/doc/docs

##  Entrypoint helper script

The entrypoint command follows the same design principles as the one in the sw-platform repo,
please refer to the documentation of the sw-platform repo.
