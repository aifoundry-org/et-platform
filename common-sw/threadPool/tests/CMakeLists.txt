#------------------------------------------------------------------------------
# Copyright (C) 2021, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

include(GoogleTest)
enable_testing()
include(CompilerWarnings)
include(CompilerSanitizers)
find_package(GTest REQUIRED)

add_executable(testThreadPool testThreadPool.cpp)
target_compile_features(testThreadPool PRIVATE cxx_std_17)
target_link_libraries(testThreadPool 
  PRIVATE
    hostUtils::threadPool
    GTest::gtest)

target_set_project_warnings(testThreadPool)
target_add_sanitizers(testThreadPool)


gtest_discover_tests(testThreadPool
  TEST_PREFIX threadPool:
  TEST_LIST DISCOVERED_TESTS
)