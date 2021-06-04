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

# System dependencies
find_package(DeviceLayer REQUIRED)
find_package(sw-sysemu REQUIRED)

# Esperanto dependencies
find_package(EsperantoBootLoader REQUIRED)
find_package(EsperantoDeviceMinionRuntime REQUIRED)
find_package(EsperantoTestKernels REQUIRED)
find_package(gflags REQUIRED)
find_package(OpenMP REQUIRED)


set(COMMON_DIR ${CMAKE_CURRENT_SOURCE_DIR}/common)
# Helper function to build a test binary
#
# Args
# NAME : binary name of the test
# SOURCES : list of source files
macro(build_test_binary)
    cmake_parse_arguments(TEST
        ""
        "TARGET;"
        "SOURCES;"
        ${ARGN}
    )

    add_executable(${TEST_TARGET} ${TEST_SOURCES})

    # We need PRIVATE include dirs
    get_target_property(ETRT_OBJ_INCLUDE_DIRS etrt_obj INCLUDE_DIRECTORIES)

    target_include_directories(${TEST_TARGET}
        PUBLIC
            ${ETRT_OBJ_INCLUDE_DIRS}
        PRIVATE
            ${COMMON_DIR}
    )

    target_link_libraries(${TEST_TARGET}
        PRIVATE
            runtime::etrt_static
            cereal::cereal
            gflags
            deviceLayer::deviceLayer
            sw-sysemu::sw-sysemu
            GTest::gtest            
            GTest::gmock
            stdc++fs
            OpenMP::OpenMP_CXX
    )
    
    target_set_project_warnings(${TEST_TARGET})
    target_add_sanitizers(${TEST_TARGET})
endmacro()

## Helper function for creating the different test binary targets
## Args:
##  TEST_INFO: takes a list of string where the string a column (":") separated arguments
##     to emulate tuples where the first element of the tuple is the test source file
##     and the send element is any additional test arguments
##  TEST_LABEL String with the list of labels to apply to the test
function(create_test_targets TEST_LIST TEST_PROPERTIES)
  foreach(TEST_INFO ${TEST_LIST})
    string(REPLACE ":" ";" TMP_LIST ${TEST_INFO})
    list(GET TMP_LIST 0 TEST_FILE)
    message("Test source: " ${TEST_FILE})
    list(GET TMP_LIST 1 TEST_ARGS)
    # Remove the quotes from the string that we have passed otherwise they
    # survice in the invocation of gtest and result in incorrect argument parsing
    string(REPLACE "\"" "" TEST_ARGS ${TEST_ARGS})
    # Convert the string into a cmake list (";" separated) this will result in the2
    # argument list looking like --arg1=val1;--arg2=val2 etc
    # When passed to gtest_discover_tests in EXTRA_ARGS this will result in generating
    # "--arg1=val1"  "--arg2=val2"  that is the correct way to escape the arguments.
    string(REPLACE " " ";" TEST_ARGS "${TEST_ARGS}")
    # message("Test args: " ${TEST_ARGS})
    get_filename_component(TEST_TARGET ${TEST_FILE} NAME_WE)

    build_test_binary(
      TARGET ${TEST_TARGET}
      SOURCES ${TEST_FILE}
      )

    gtest_discover_tests(${TEST_PREFIX} ${TEST_TARGET}
      EXTRA_ARGS ${TEST_ARGS}
      TEST_PREFIX runtime:
      PROPERTIES ${TEST_PROPERTIES}
      TEST_LIST DISCOVERED_TESTS
      )
  endforeach(TEST_INFO)
endfunction()

get_property(BOOTROM_TRAMPOLINE_TO_BL2_ELF TARGET EsperantoBootLoader::BootromTrampolineToBL2.elf PROPERTY LOCATION)
get_property(BL2_ELF TARGET EsperantoBootLoader::ServiceProcessorBL2_fast-boot.elf PROPERTY LOCATION)
get_property(MASTER_MINION_ELF TARGET EsperantoDeviceMinionRuntime::MasterMinion.elf PROPERTY LOCATION)
get_property(MACHINE_MINION_ELF TARGET EsperantoDeviceMinionRuntime::MachineMinion.elf PROPERTY LOCATION)
get_property(WORKER_MINION_ELF TARGET EsperantoDeviceMinionRuntime::WorkerMinion.elf PROPERTY LOCATION)

set(KERNELS_DIR "${ESPERANTO_TEST_KERNELS_LIB_DIR}/esperanto-fw/kernels")

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${BREAKPAD_INCLUDE_DIR}/breakpad)

add_subdirectory(common)
add_subdirectory(unit-tests)
add_subdirectory(integration-tests)
add_subdirectory(stress-tests)
