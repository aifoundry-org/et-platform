#------------------------------------------------------------------------------
# Copyright (C) 2020, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

find_package(GTest REQUIRED)

find_package(hostUtils REQUIRED)

#helper to create tests
function(create_test)
    set(options )
    set(oneValueArgs TARGET)
    set(multiValueArgs SOURCES ARGS PROPERTIES)
    cmake_parse_arguments(TEST "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    add_executable(${TEST_TARGET} ${TEST_SOURCES})
    target_compile_features(${TEST_TARGET} PRIVATE cxx_std_17)
    target_include_directories(${TEST_TARGET} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
    target_link_libraries(${TEST_TARGET}
        PRIVATE
            deviceLayer::deviceLayer
            hostUtils::logging
            GTest::gtest
            GTest::gmock
    )
    gtest_discover_tests(${TEST_TARGET}
        EXTRA_ARGS ${TEST_ARGS}
        TEST_PREFIX device-layer:
        PROPERTIES ${TEST_PROPERTIES} TEST_DISCOVERY_TIMEOUT 60
        TEST_LIST DISCOVERED_TESTS
    )
endfunction()

#all tests should be created using the helper
create_test(
    TARGET     simple
    SOURCES    simple.cpp
    PROPERTIES "LABELS;Generic;LABELS;Basic;TIMEOUT;120"
)
