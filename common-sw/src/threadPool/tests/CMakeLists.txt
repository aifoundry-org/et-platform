# Copyright (c) 2025 Ainekko, Co.
# SPDX-License-Identifier: Apache-2.0

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