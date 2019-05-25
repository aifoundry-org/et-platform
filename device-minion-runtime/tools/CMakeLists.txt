#------------------------------------------------------------------------------
# Copyright (C) 2019, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

# Add teh simulator as an external project so that it builds using the host (x86)
# toolchain
include(ExternalProject)

ExternalProject_Add(sw-sysemu
  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/sw-sysemu
  CONFIGURE_COMMAND ""
  BUILD_COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/sysemu-build && $(MAKE) -C ${CMAKE_CURRENT_SOURCE_DIR}/sw-sysemu/sys_emu/ sysemu_OBJDIR=${CMAKE_CURRENT_BINARY_DIR}/sysemu-build libfpu_OBJDIR=${CMAKE_CURRENT_BINARY_DIR}/sysemu-build-fpu
  BUILD_BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/sysemu-build/sys_emu
  INSTALL_COMMAND ""
  )
