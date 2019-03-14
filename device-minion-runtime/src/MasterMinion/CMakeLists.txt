#------------------------------------------------------------------------------
# Copyright (C) 2019, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

# Add the device_api submodule as an external project. The code-generator code needs to always
# compile for the host. CMake does not support having different targets use
# different toolchains in the same build. For simplicity submodules that are self
# contained build as external projects, otherwise a two-step process would be needded.
include(ExternalProject)

ExternalProject_Add(device-api
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/device_api
    BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/device_api
    CMAKE_ARGS parse-device-api)

set(SOURCES
    ${SHARED_SRC_DIR}/boot.S
    src/crt.S
    src/main.c
    ${SHARED_SRC_DIR}/serial.c
    src/fw_compute_code.c
    src/fw_master_code.c
    ${SHARED_SRC_DIR}/trap_handler.c
    ${SHARED_SRC_DIR}/trap_handler.S)

set(TARGET_NAME MasterMinion)
set(LINKER_SCRIPT src/sections.ld)
set(ZEBU_TARGET DDR)
set(ZEBU_FILENAME memImage)

include_directories(include)
include_directories(${SHARED_INC_DIR})

add_riscv_executable(${TARGET_NAME} ${LINKER_SCRIPT} ${ZEBU_TARGET} ${ZEBU_FILENAME} ${SOURCES})
