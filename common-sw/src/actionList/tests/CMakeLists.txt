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