#------------------------------------------------------------------------------
# Copyright (C) 2019, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

include(ExternalProject)
include(GoogleTest)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall -Werror -g3 -O0")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/../include)
include_directories(${GLOG_INCLUDE_DIRS})

include_directories(${EXTERNAL_INCLUDE_DIRECTORIES})
link_directories(${RT_EXTERNAL_INSTALL_PREFIX}/lib64
  ${RT_EXTERNAL_INSTALL_PREFIX}/lib
  ${DEPENDENCIES_PATH}/lib64
  ${DEPENDENCIES_PATH}/lib
  )

set(COMMON_LIBRARIES
   etrt
  )

add_executable(runtime-tester runtime_tester.cc)
message("External dependencies: " ${external_project_dependencies})
add_dependencies(runtime-tester ${external_project_dependencies})
target_link_libraries(runtime-tester ${COMMON_LIBRARIES})

## Build static library
install(TARGETS runtime-tester
  EXPORT EsperantoRuntimeTargets
  RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
  LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
  ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
  COMPONENT et-runtime
  )

add_custom_target(gen-python-tools ALL
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/trace_printer.py
            ${CMAKE_CURRENT_BINARY_DIR}/trace_printer.py
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          ${CMAKE_CURRENT_BINARY_DIR}/../src/Tracing/etrt_trace_pb2.py
          ${CMAKE_CURRENT_BINARY_DIR}/etrt_trace_pb2.py
  DEPENDS etrt-tracing-gen-code
            )
