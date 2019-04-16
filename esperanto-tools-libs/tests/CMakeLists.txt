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

link_directories(${GTEST_DIR}/lib64)

add_compile_options(-g3 -std=c++11 -O0)
set(COMMON_TEST_LIBRARIES
  etrt
  libgtest_main.a
  libgtest.a
  pthread)


add_executable(test-error-or test_error_or.cc)
add_dependencies(test-error-or googletest)
target_link_libraries(test-error-or ${COMMON_TEST_LIBRARIES})
gtest_discover_tests(test-error-or TEST_PREFIX runtime:)
