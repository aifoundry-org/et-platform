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

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/../include)
include_directories(${GLOG_INCLUDE_DIRS})

link_directories(${RT_EXTERNAL_INSTALL_PREFIX}/lib64
  ${RT_EXTERNAL_INSTALL_PREFIX}/lib)

set(COMMON_LIBRARIES
  etrt_static
  ${GLOG_LIBRARIES}
  )

add_executable(runtime-tester runtime_tester.cc)
message("External dependencies: " ${external_project_dependencies})
add_dependencies(runtime-tester ${external_project_dependencies})
target_link_libraries(runtime-tester ${COMMON_LIBRARIES})
