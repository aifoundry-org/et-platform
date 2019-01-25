Device Firmware Project
=========================

This project implements the device firmware running on the Esperanto SOC

The project implements the following components:

    \todo add reference to Master Minion Documentation
    \todo add add reference to Slave Minion Documentation


Following we have the basic steps to build the basic "flavor" of the repo and generating
the repo's documentation

## Initialize the repo

### Install the necessary git-hooks

\todo add instructions how to setup the git-hooks

### Update the git submodules

Run:

    ./scripts/init-submodules.sh

## Create a docker container to work in

In the repo's TOT run the following command:

    ./device-firmware.py docker prompt et-sw-device-fw

The above should automatically compute the correct version of the et-sw-platform docker image
to use, instantiate a container for the user and return an active prompt for the user to use.
The container automatically mounts inside it the repo code and any code that the user builds
will persist once it exits the container.

For more details about docker see the related documentation section of the sw-platform repo.

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
