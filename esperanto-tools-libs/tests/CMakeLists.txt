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

include_directories(${GTEST_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/../include)

link_directories(${GTEST_DIR}/lib64)

add_compile_options(-g3 -O0 -std=c++17)
set(COMMON_TEST_LIBRARIES
  etrt_static
  libgtest_main.a
  libgtest.a
  ${GLOG_LIBRARIES}
  ${FMTLIB_LIBRARIES}
  pthread)

set(TEST_LIST test_error_or.cc
  test_device_manager.cc
  test_device_target_card_proxy.cc
  )


foreach(TEST_FILE ${TEST_LIST})
  get_filename_component(TEST_TARGET ${TEST_FILE} NAME_WE)
  add_executable(${TEST_TARGET} ${TEST_FILE})
  add_dependencies(${TEST_TARGET} googletest)
  add_dependencies(${TEST_TARGET} ${external_project_dependencies})
  target_link_libraries(${TEST_TARGET} ${COMMON_TEST_LIBRARIES})
  target_include_directories(${TEST_TARGET} PUBLIC ${GFLAGS_INCLUDE_DIRS})
  target_include_directories(${TEST_TARGET} PUBLIC ${GLOG_INCLUDE_DIRS})
  gtest_discover_tests(${TEST_TARGET} TEST_PREFIX runtime:)

endforeach(TEST_FILE)
