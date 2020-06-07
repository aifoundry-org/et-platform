#------------------------------------------------------------------------------
# Copyright (C) 2019, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

add_library(etrt_CodeManagement OBJECT
  CodeModule.cc
  CodeModule.h
  CodeRegistry.cc
  ELFSupport.cc
  ELFSupport.h
  Kernel.cc
  Library.cc
  ModuleManager.cc
  ModuleManager.h
  UberKernel.cc
  )

target_include_directories(etrt_CodeManagement PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR}
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/src
  ${PROJECT_BINARY_DIR}/include
  ${PROJECT_BINARY_DIR}/src
  ${EXTERNAL_INCLUDE_DIRECTORIES}
  )

add_sanitizers(etrt_CodeManagement)

if(MINION_RUNTIME_FOUND)
  target_include_directories(etrt_CodeManagement PRIVATE "${ESPERANTO_FW_INCLUDE_DIR}/")
endif(MINION_RUNTIME_FOUND)
