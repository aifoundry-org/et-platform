#------------------------------------------------------------------------------
# Copyright (C) 2019, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

add_library(etrt_NewCore OBJECT
  Runtime.cpp
  RuntimeImp.cpp
  TargetSilicon.cpp
  TargetSysEmu.cpp
  MemoryManager.cpp
  EventManager.cpp
  )

target_include_directories(etrt_NewCore PRIVATE
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/src
  ${PROJECT_BINARY_DIR}/include
  ${PROJECT_BINARY_DIR}/src
  )

add_sanitizers(etrt_NewCore)

if(MINION_RUNTIME_FOUND)
  target_include_directories(etrt_NewCore PRIVATE "${ESPERANTO_FW_INCLUDE_DIR}/")
endif(MINION_RUNTIME_FOUND)
