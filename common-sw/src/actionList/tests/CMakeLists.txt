# Copyright (c) 2025 Ainekko, Co.
# SPDX-License-Identifier: Apache-2.0

include(GoogleTest)
enable_testing()
include(CompilerWarnings)
include(CompilerSanitizers)
find_package(GTest REQUIRED)

add_executable(testActionList testActionList.cpp)
target_compile_features(testActionList PRIVATE cxx_std_17)
target_link_libraries(testActionList PRIVATE
    hostUtils::actionList
    GTest::gtest
    GTest::gmock)

target_set_project_warnings(testActionList)
target_add_sanitizers(testActionList)    

gtest_discover_tests(testActionList
  TEST_PREFIX actionList:
  TEST_LIST DISCOVERED_TESTS
)