#!/bin/bash

git submodule update --init --recursive \
    infra_tools \
    src/MasterMinion/device_api \
    src/ServiceProcessorROM \
    firmware-tools
