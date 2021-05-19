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
find_package(glog REQUIRED)

find_package(deviceApi REQUIRED)
find_package(EsperantoBootLoader REQUIRED)
find_package(EsperantoDeviceMinionRuntime REQUIRED)
find_package(EsperantoTestKernels REQUIRED)

set(KERNELS_DIR "${ESPERANTO_TEST_KERNELS_LIB_DIR}/esperanto-fw/kernels")

add_library(testHelper
    testHelper/DevOpsApiCmd.cpp
    testHelper/TestDevOpsApi.cpp
)

target_compile_features(testHelper PRIVATE cxx_std_17)
target_link_libraries(testHelper
    PUBLIC
        deviceApi::deviceApi
        deviceLayer::deviceLayer
        logging::logging
        glog::glog
        gflags::gflags
        GTest::gtest
)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "9")
    target_link_libraries(testHelper PRIVATE stdc++fs)
endif ()


#helper to create tests
function(create_test)
    set(options )
    set(oneValueArgs TARGET)
    set(multiValueArgs SOURCES ARGS PROPERTIES)
    cmake_parse_arguments(TEST "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    add_executable(${TEST_TARGET} ${TEST_SOURCES})
    target_compile_features(${TEST_TARGET} PRIVATE cxx_std_17)
    target_include_directories(${TEST_TARGET} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(${TEST_TARGET} PRIVATE testHelper)

    # set internal execution timeout value relative to actual timeout. Internal timeout
    # is smaller for wraping up the execution and printing the test summary
    string(REGEX MATCH "[T][I][M][E][O][U][T][0-9]*" TIMEOUT_ENTRY ${TEST_PROPERTIES})
    string(REGEX REPLACE "[T][I][M][E][O][U][T]" "" TIMEOUT_VAL ${TIMEOUT_ENTRY})
    if(TIMEOUT_VAL STREQUAL "")
        # default timeout value
        set(TIMEOUT_VAL "120")
    endif()
    set(TEST_ARGS ${TEST_ARGS} --exec_timeout=${TIMEOUT_VAL})

    gtest_discover_tests(${TEST_TARGET}
        EXTRA_ARGS ${TEST_ARGS}
        TEST_PREFIX device-layer:
        PROPERTIES ${TEST_PROPERTIES}
        TEST_LIST DISCOVERED_TESTS
    )
endfunction()

if(BUILD_PCIE_LOOPBACK_DRIVER)
    set(LOOPBACK_DRIVER "true")
else()
    set(LOOPBACK_DRIVER "false")
endif()

#all tests should be created using the helper

###############
# BASIC TESTS #
###############
create_test(
    TARGET     TestDevOpenClosePcie
    SOURCES    pcie/basic/TestDevOpenClosePcie.cpp
    ARGS       "--loopback_driver=${LOOPBACK_DRIVER}"
    PROPERTIES "LABELS;BEMU;LABELS;PCIE;LABELS;OPS;LABELS;FUNCTIONAL;LABEL;UNIT;LABELS;LOOPBACK;TIMEOUT;120"
)

create_test(
    TARGET     TestDevOpsApiFuncBasicCmdsPcie
    SOURCES    pcie/basic/TestDevOpsApiFuncBasicCmdsPcie.cpp TestDevOpsApiBasicCmds.cpp
    ARGS       "--loopback_driver=${LOOPBACK_DRIVER}"
    PROPERTIES "LABELS;FC;LABELS;BEMU;LABELS;PCIE;LABELS;OPS;LABELS;FUNCTIONAL;LABEL;SYSTEM;LABELS;LOOPBACK;TIMEOUT;120"
)

create_test(
    TARGET     TestDevOpsApiFuncDmaCmdsPcie
    SOURCES    pcie/basic/TestDevOpsApiFuncDmaCmdsPcie.cpp TestDevOpsApiDmaCmds.cpp
    ARGS       "--loopback_driver=${LOOPBACK_DRIVER}"
    PROPERTIES "LABELS;FC;LABELS;BEMU;LABELS;PCIE;LABELS;OPS;LABELS;FUNCTIONAL;LABEL;SYSTEM;LABELS;LOOPBACK;TIMEOUT;120"
)

create_test(
    TARGET     TestDevOpsApiFuncKernelCmdsPcie
    SOURCES    pcie/basic/TestDevOpsApiFuncKernelCmdsPcie.cpp TestDevOpsApiKernelCmds.cpp
    ARGS       "--kernels_dir=${KERNELS_DIR}"
    PROPERTIES "LABELS;FC;LABELS;BEMU;LABELS;PCIE;LABELS;OPS;LABELS;FUNCTIONAL;LABEL;SYSTEM;TIMEOUT;120"
)

create_test(
    TARGET     TestDevOpsApiFuncTraceCmdsPcie
    SOURCES    pcie/basic/TestDevOpsApiFuncTraceCmdsPcie.cpp TestDevOpsApiTraceCmds.cpp
    ARGS       "--kernels_dir=${KERNELS_DIR}"
    PROPERTIES "LABELS;FC;LABELS;BEMU;LABELS;PCIE;LABELS;OPS;LABELS;FUNCTIONAL;LABEL;SYSTEM;TIMEOUT;120"
)

create_test(
    TARGET     TestDevOpsApiStressBasicCmdsPcie
    SOURCES    pcie/basic/TestDevOpsApiStressBasicCmdsPcie.cpp TestDevOpsApiBasicCmds.cpp
    ARGS       "--loopback_driver=${LOOPBACK_DRIVER}"
    PROPERTIES "LABELS;FC;LABELS;BEMU;LABELS;PCIE;LABELS;OPS;LABELS;STRESS;LABEL;SYSTEM;LABELS;LOOPBACK;TIMEOUT;240"
)

create_test(
    TARGET     TestDevOpsApiStressDmaCmdsPcie
    SOURCES    pcie/basic/TestDevOpsApiStressDmaCmdsPcie.cpp TestDevOpsApiDmaCmds.cpp
    ARGS       "--loopback_driver=${LOOPBACK_DRIVER}"
    PROPERTIES "LABELS;FC;LABELS;BEMU;LABELS;PCIE;LABELS;OPS;LABELS;STRESS;LABEL;SYSTEM;LABELS;LOOPBACK;TIMEOUT;240"
)

create_test(
    TARGET     TestDevOpsApiStressKernelCmdsPcie
    SOURCES    pcie/basic/TestDevOpsApiStressKernelCmdsPcie.cpp TestDevOpsApiKernelCmds.cpp
    ARGS       "--kernels_dir=${KERNELS_DIR}"
    PROPERTIES "LABELS;FC;LABELS;BEMU;LABELS;PCIE;LABELS;OPS;LABELS;STRESS;LABEL;SYSTEM;TIMEOUT;240"
)

create_test(
    TARGET     TestDevOpenCloseSysEmu
    SOURCES    sysemu/TestDevOpenCloseSysEmu.cpp
    ARGS       ""
    PROPERTIES "LABELS;SYSEMU;LABELS;OPS;LABELS;FUNCTIONAL;LABEL;UNIT;TIMEOUT;120"
)

create_test(
    TARGET     TestDevOpsApiFuncBasicCmdsSysEmu
    SOURCES    sysemu/TestDevOpsApiFuncBasicCmdsSysEmu.cpp TestDevOpsApiBasicCmds.cpp
    ARGS       ""
    PROPERTIES "LABELS;SYSEMU;LABELS;OPS;LABELS;FUNCTIONAL;LABEL;SYSTEM;TIMEOUT;120"
)

create_test(
    TARGET     TestDevOpsApiFuncDmaCmdsSysEmu
    SOURCES    sysemu/TestDevOpsApiFuncDmaCmdsSysEmu.cpp TestDevOpsApiDmaCmds.cpp
    ARGS       ""
    PROPERTIES "LABELS;SYSEMU;LABELS;OPS;LABELS;FUNCTIONAL;LABEL;SYSTEM;TIMEOUT;120"
)

create_test(
    TARGET     TestDevOpsApiFuncKernelCmdsSysEmu
    SOURCES    sysemu/TestDevOpsApiFuncKernelCmdsSysEmu.cpp TestDevOpsApiKernelCmds.cpp
    ARGS       "--kernels_dir=${KERNELS_DIR}"
    PROPERTIES "LABELS;SYSEMU;LABELS;OPS;LABELS;FUNCTIONAL;LABEL;SYSTEM;TIMEOUT;120"
)

create_test(
    TARGET     TestDevOpsApiFuncTraceCmdsSysEmu
    SOURCES    sysemu/TestDevOpsApiFuncTraceCmdsSysEmu.cpp TestDevOpsApiTraceCmds.cpp
    ARGS       ""
    PROPERTIES "LABELS;SYSEMU;LABELS;OPS;LABELS;FUNCTIONAL;LABEL;SYSTEM;TIMEOUT;120"
)

create_test(
    TARGET     TestDevOpsApiStressBasicCmdsSysEmu
    SOURCES    sysemu/TestDevOpsApiStressBasicCmdsSysEmu.cpp TestDevOpsApiBasicCmds.cpp
    ARGS       ""
    PROPERTIES "LABELS;SYSEMU;LABELS;OPS;LABELS;STRESS;LABEL;SYSTEM;TIMEOUT;120"
)

create_test(
    TARGET     TestDevOpsApiStressDmaCmdsSysEmu
    SOURCES    sysemu/TestDevOpsApiStressDmaCmdsSysEmu.cpp TestDevOpsApiDmaCmds.cpp
    ARGS       ""
    PROPERTIES "LABELS;SYSEMU;LABELS;OPS;LABELS;STRESS;LABEL;SYSTEM;TIMEOUT;120"
)

create_test(
    TARGET     TestDevOpsApiStressKernelCmdsSysEmu
    SOURCES    sysemu/TestDevOpsApiStressKernelCmdsSysEmu.cpp TestDevOpsApiKernelCmds.cpp
    ARGS       "--kernels_dir=${KERNELS_DIR}"
    PROPERTIES "LABELS;SYSEMU;LABELS;OPS;LABELS;STRESS;LABEL;SYSTEM;TIMEOUT;500"
)

#################
# NIGHTLY TESTS #
#################
create_test(
    TARGET     TestDevOpsApiNightlyBasicCmdsPcie
    SOURCES    pcie/nightly/TestDevOpsApiNightlyBasicCmdsPcie.cpp TestDevOpsApiBasicCmds.cpp
    ARGS       ""
    PROPERTIES "LABELS;NIGHTLY;LABELS;PCIE;LABELS;OPS;LABELS;STRESS;LABEL;SYSTEM;TIMEOUT;900"
)

create_test(
    TARGET     TestDevOpsApiNightlyDmaCmdsPcie
    SOURCES    pcie/nightly/TestDevOpsApiNightlyDmaCmdsPcie.cpp TestDevOpsApiDmaCmds.cpp
    ARGS       ""
    PROPERTIES "LABELS;NIGHTLY;LABELS;PCIE;LABELS;OPS;LABELS;STRESS;LABEL;SYSTEM;TIMEOUT;240"
)

create_test(
    TARGET     TestDevOpsApiNightlyKernelCmdsPcie
    SOURCES    pcie/nightly/TestDevOpsApiNightlyKernelCmdsPcie.cpp TestDevOpsApiKernelCmds.cpp
    ARGS       "--kernels_dir=${KERNELS_DIR}"
    PROPERTIES "LABELS;NIGHTLY;LABELS;PCIE;LABELS;OPS;LABELS;STRESS;LABEL;SYSTEM;TIMEOUT;600"
)
