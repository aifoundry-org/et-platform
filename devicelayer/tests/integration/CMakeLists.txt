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
find_package(gflags REQUIRED)
find_package(esperanto-device-api REQUIRED)
find_package(EsperantoBootLoader REQUIRED)
find_package(EsperantoDeviceMinionRuntime REQUIRED)
find_package(EsperantoTestKernels REQUIRED)

#helper to create tests
function(create_test)
  cmake_parse_arguments(TEST
    ""
    "TARGET"
    "SOURCES;ARGS;PROPERTIES"
    ${ARGN}
  )

  add_executable(${TEST_TARGET} ${TEST_SOURCES})
  target_compile_features(${TEST_TARGET} PRIVATE cxx_std_17)
  target_link_libraries(${TEST_TARGET} deviceLayer::deviceLayer GTest::GTest GTest::Main gflags)
  target_include_directories(${TEST_TARGET} PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ${ESPERANTO_DEVICE_API_INCLUDE_DIR})
  gtest_discover_tests(${TEST_TARGET}
    EXTRA_ARGS ${TEST_ARGS}
    TEST_PREFIX device-layer:
    PROPERTIES ${TEST_PROPERTIES}
    TEST_LIST DISCOVERED_TESTS
  )
endfunction()

#all tests should be created using the helper
create_test(
  TARGET     simple
  SOURCES    simple.cpp
  PROPERTIES "LABELS;Generic;LABELS;Basic;TIMEOUT;120"
)
