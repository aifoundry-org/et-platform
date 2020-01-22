#------------------------------------------------------------------------------
# Copyright (C) 2019, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

# Rules for building external cmake dependencies
# FIXME add support for building the the crypto library SW-1772

## Firmware Code signing tools
# Find the package or else build it from source

if(BUILD_FLASH_IMAGE)

  ExternalProject_Add(code-signing-tools
    PREFIX code_signing_tools
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../firmware-tools/code-signing-tools/
    BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/code-signing-tools
    LOG_UPDATE TRUE
    LOG_PATCH TRUE

    # CONFIGURE_COMMAND ""
    # Compile the tools for x86
    CMAKE_ARGS
    -DCMAKE_INSTALL_PREFIX=${DEVFW_EXTERNAL_INSTALL_PREFIX}
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    -DCMAKE_C_COMPILER=gcc
    ${EXTERNAL_PROJECT_ENABLE_CCACHE}
    ${EXTERNAL_PROJECT_CCACHE}
    LOG_TEST TRUE
    BUILD_ALWAYS 1
    )

  ExternalProject_Add(flash-tool
    PREFIX flash-tool
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../firmware-tools/esperanto-flash-tool
    BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/esperanto-flash-tool
    LOG_UPDATE TRUE
    LOG_PATCH TRUE
    # Compile the tools for x86
    CMAKE_ARGS
    -DCMAKE_INSTALL_PREFIX=${DEVFW_EXTERNAL_INSTALL_PREFIX}
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    -DCMAKE_C_COMPILER=gcc
    ${EXTERNAL_PROJECT_ENABLE_CCACHE}
    ${EXTERNAL_PROJECT_CCACHE}
    LOG_TEST TRUE
    BUILD_ALWAYS 1
    )

  set(EXTERNAL_PROJECT_DEPENDENCIES "code-signing-tools;flash-tool"  PARENT_SCOPE)

endif(BUILD_FLASH_IMAGE)
