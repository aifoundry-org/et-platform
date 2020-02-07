#------------------------------------------------------------------------------
# Copyright (C) 2020, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

add_subdirectory("include")

add_library(bootrom_helpers STATIC
  src/boot_config.c
  src/constant_memory_compare.c
  )

target_include_directories(bootrom_helpers PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/esperanto-fw/bootrom_helpers>)


install(
  TARGETS bootrom_helpers
  EXPORT esperanto-fw-targets
  LIBRARY
    DESTINATION lib
    COMPONENT boorom_helpers
  PUBLIC_HEADER
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/esperanto-fw/bootrom_helpers
    COMPONENT bootrom_helpers
)

install(
  FILES
    include/boot_config.h
    include/constant_memory_compare.h
  DESTINATION
    ${CMAKE_INSTALL_INCLUDEDIR}/esperanto-fw/bootrom_helpers
  COMPONENT bootrom_helpers
  )
