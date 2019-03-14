#!/bin/bash

git submodule update --init --recursive \
    infra_tools \
    src/MasterMinion/device_api \
    tools/sw-sysemu
