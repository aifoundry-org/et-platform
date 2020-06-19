#------------------------------------------------------------------------------
# Copyright (C) 2020, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.5)

get_property(SP_ROM_HELPERS_PATH
  TARGET  EsperantoServiceProcessorROM::sp_rom_helpers
  PROPERTY INTERFACE_INCLUDE_DIRECTORIES)

set(CMAKE_C_COMPILER "gcc")
set(CMAKE_C_FLAGS    "-Wall")

add_executable(sp-bl1-data-gen main.c)

include_directories(${SP_ROM_HELPERS_PATH})
include_directories(${CRYPTO_HEADERS})

install(TARGETS sp-bl1-data-gen
    RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin/
    COMPONENT tools
)
