#------------------------------------------------------------------------------
# Copyright (C) 2019, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

add_library(etrt_RPCDevice OBJECT
  EmuMailBoxDev.cc
  EmuMailBoxDev.h
  SysEmuLauncher.cpp
  SysEmuLauncher.h
  TargetDeviceInfo.h.in
  TargetRPC.cc
  TargetRPC.h
  SysEmuAbslFlags.h
  TargetSysEmu.cc
  TargetSysEmu.h
  EmuVirtQueueDev.h
  EmuVirtQueueDev.cc
  RPCTarget_MM.h
  RPCTarget_MM.cc
  SysEmuTarget_MM.h
  SysEmuTarget_MM.cc
  )

target_include_directories(etrt_RPCDevice PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR}
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/src
  ${PROJECT_BINARY_DIR}/include
  ${PROJECT_BINARY_DIR}/src
  ${EXTERNAL_INCLUDE_DIRECTORIES}
  )

target_include_directories(etrt_RPCDevice PRIVATE "${ESPERANTO_DEVICE_MINION_RUNTIME_INCLUDE_DIR}/")

add_sanitizers(etrt_RPCDevice)
