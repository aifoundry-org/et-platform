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

if("${devicefw_CODE_SIGNING_TOOLS_PROVIDER}" STREQUAL "module")

  set(code_signing_tools_INSTALL ${DEVFW_EXTERNAL_INSTALL_PREFIX})
  execute_process(COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/code-signing-tools)
  execute_process(COMMAND ${CMAKE_COMMAND} -H${CMAKE_CURRENT_SOURCE_DIR}/../firmware-tools/code-signing-tools/
                     -DCMAKE_INSTALL_PREFIX=${code_signing_tools_INSTALL}
                     -DCMAKE_C_COMPILER=gcc
                     -DGIT_CONFIG_OPTIONS=${GIT_CONFIG_OPTIONS}
                     ${EXTERNAL_PROJECT_CCACHE}
                     ${EXTERNAL_PROJECT_ENABLE_CCACHE}
    RESULT_VARIABLE result
    ERROR_VARIABLE err
    OUTPUT_VARIABLE out
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/code-signing-tools)
  if(result)
    message(FATAL_ERROR "CMake step for code-signing-tools failed ${result}: ${err} ${out}")
  endif()
  # Build the device_api target
  execute_process(COMMAND ${CMAKE_COMMAND} --build . --config ${CMAKE_BUILD_TYPE} --target install
    RESULT_VARIABLE result
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/code-signing-tools)
  if(result)
    message(FATAL_ERROR "Build step for code-signing-tools failed: ${result}")
  endif()
  message(STATUS "Building: code-signing-tools...DONE")

  # Force the search location to be the one where we have installed the package
  set(EsperantoCodeSigningTools_DIR ${code_signing_tools_INSTALL}/lib/cmake/EsperantoCodeSigningTools)
  find_package(EsperantoCodeSigningTools CONFIG REQUIRED
    PATHS "${code_signing_tools_INSTALL}/lib/cmake/EsperantoCodeSigningTools"
    NO_CMAKE_PATH
    NO_CMAKE_ENVIRONMENT_PATH
    NO_DEFAULT_PATH
    )

else()
  find_package(EsperantoCodeSigningTools CONFIG REQUIRED)
endif()
message("Code Signing Tools Found: ${EsperantoCodeSigningTools_FOUND}")

get_target_property(CRYPTO_HEADERS_INTERFACE_PATH crypto_api INTERFACE_INCLUDE_DIRECTORIES)
set(CRYPTO_HEADERS ${CRYPTO_HEADERS_INTERFACE_PATH} PARENT_SCOPE)

get_target_property(PKCS11_HEADERS_INTERFACE_PATH pkcs11 INTERFACE_INCLUDE_DIRECTORIES)
set(PKCS11_HEADERS ${PKCS11_HEADERS_INTERFACE_PATH} PARENT_SCOPE)


if(BUILD_FLASH_IMAGE)

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

  set(EXTERNAL_PROJECT_DEPENDENCIES "flash-tool"  PARENT_SCOPE)

endif(BUILD_FLASH_IMAGE)
