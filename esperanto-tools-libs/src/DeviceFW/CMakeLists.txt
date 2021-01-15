#------------------------------------------------------------------------------
# Copyright (C) 2019, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

add_library(etrt_DeviceFW OBJECT
  DeviceFW.cc
  DeviceFW.h
  FWManager.cc
  FWManager.h
  )

target_include_directories(etrt_DeviceFW PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR}
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/src
  ${PROJECT_BINARY_DIR}/include
  ${PROJECT_BINARY_DIR}/src
  ${ESPERANTO_DEVICE_MINION_RUNTIME_INCLUDE_DIR}
  ${EXTERNAL_INCLUDE_DIRECTORIES}
  )

add_sanitizers(etrt_DeviceFW)
