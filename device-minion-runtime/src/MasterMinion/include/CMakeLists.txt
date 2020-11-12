#------------------------------------------------------------------------------
# Copyright (C) 2020, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

add_library(mm_dev_intf INTERFACE)
target_include_directories(mm_dev_intf INTERFACE
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/esperanto-fw/mm_dev_intf>"
  )

install(
  TARGETS mm_dev_intf
  EXPORT EsperantoDeviceMinionRuntimeTargets
  LIBRARY
    DESTINATION lib
    COMPONENT mm_dev_intf
  PUBLIC_HEADER
    DESTINATION ${CMAKE_INSTALL_INCLUDE_DIR}/esperanto-fw/mm_dev_intf
    COMPONENT mm_dev_intf
  )

install(
  FILES
    mm_dev_intf_reg.h
  DESTINATION
    ${CMAKE_INSTALL_INCLUDEDIR}/esperanto-fw/mm_dev_intf
  COMPONENT mm_dev_intf
  )
