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
        hostUtils::logging
        glog::glog
        gflags::gflags
        GTest::gtest
)

# create a specialized library with PCIe specific compile definition
add_library(testHelperPcie
    testHelper/DevOpsApiCmd.cpp
    testHelper/TestDevOpsApi.cpp
)
target_compile_definitions(testHelperPcie PRIVATE TARGET_PCIE)
target_compile_features(testHelperPcie PRIVATE cxx_std_17)
target_link_libraries(testHelperPcie
    PUBLIC
        deviceApi::deviceApi
        deviceLayer::deviceLayer
        hostUtils::logging
        glog::glog
        gflags::gflags
        GTest::gtest
)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "9")
    target_link_libraries(testHelper PRIVATE stdc++fs)
    target_link_libraries(testHelperPcie PRIVATE stdc++fs)
endif ()


#helper to create tests
function(create_test)
    set(options )
    set(oneValueArgs TARGET TARGET_DEF)
    set(multiValueArgs SOURCES ARGS PROPERTIES)
    cmake_parse_arguments(TEST "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    add_executable(${TEST_TARGET} ${TEST_SOURCES})
    target_compile_features(${TEST_TARGET} PRIVATE cxx_std_17)
    target_include_directories(${TEST_TARGET} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    # See if test helper PCIE library needs to be linked
    if (${TEST_TARGET_DEF} STREQUAL "TARGET_PCIE")
        target_link_libraries(${TEST_TARGET} PRIVATE testHelperPcie)
    else ()
        target_link_libraries(${TEST_TARGET} PRIVATE testHelper)
    endif ()
    target_compile_definitions(${TEST_TARGET} PUBLIC ${TEST_TARGET_DEF})

    # set internal execution timeout value relative to actual timeout. Internal timeout
    # is smaller for wraping up the execution and printing the test summary
    string(REGEX MATCH "[T][I][M][E][O][U][T][0-9]*" TIMEOUT_ENTRY ${TEST_PROPERTIES})
    string(REGEX REPLACE "[T][I][M][E][O][U][T]" "" TIMEOUT_VAL ${TIMEOUT_ENTRY})
    if(TIMEOUT_VAL STREQUAL "")
        # default timeout value
        set(TIMEOUT_VAL "120")
    endif()
    set(WRAP_UP_TIME "30")
    MATH(EXPR EXEC_TIMEOUT "${TIMEOUT_VAL}-${WRAP_UP_TIME}")
    set(TEST_ARGS ${TEST_ARGS} --exec_timeout=${EXEC_TIMEOUT})

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
    TARGET_DEF TARGET_PCIE
    SOURCES    pcie/basic/TestDevOpenClosePcie.cpp
    ARGS       ""
    PROPERTIES "LABELS;BEMU;LABELS;PCIE;LABELS;OPS;LABELS;FUNCTIONAL;LABEL;UNIT;TIMEOUT;120"
)

create_test(
    TARGET     TestDevOpsApiFuncBasicCmdsPcie
    TARGET_DEF TARGET_PCIE
    SOURCES    pcie/basic/TestDevOpsApiFuncBasicCmdsPcie.cpp TestDevOpsApiBasicCmds.cpp
    ARGS       ""
    PROPERTIES "LABELS;BEMU;LABELS;PCIE;LABELS;OPS;LABELS;FUNCTIONAL;LABEL;SYSTEM;TIMEOUT;120"
)

create_test(
    TARGET     TestDevOpsApiFuncDmaCmdsPcie
    TARGET_DEF TARGET_PCIE
    SOURCES    pcie/basic/TestDevOpsApiFuncDmaCmdsPcie.cpp TestDevOpsApiDmaCmds.cpp
    ARGS       ""
    PROPERTIES "LABELS;BEMU;LABELS;PCIE;LABELS;OPS;LABELS;FUNCTIONAL;LABEL;SYSTEM;TIMEOUT;360"
)

create_test(
    TARGET     TestDevOpsApiFuncKernelCmdsPcie
    TARGET_DEF TARGET_PCIE
    SOURCES    pcie/basic/TestDevOpsApiFuncKernelCmdsPcie.cpp TestDevOpsApiKernelCmds.cpp
    ARGS       --kernels_dir=${KERNELS_DIR}
    PROPERTIES "LABELS;BEMU;LABELS;PCIE;LABELS;OPS;LABELS;FUNCTIONAL;LABEL;SYSTEM;TIMEOUT;360"
)

create_test(
    TARGET     TestDevOpsApiFuncTraceCmdsPcie
    TARGET_DEF TARGET_PCIE
    SOURCES    pcie/basic/TestDevOpsApiFuncTraceCmdsPcie.cpp TestDevOpsApiTraceCmds.cpp
    ARGS       --enable_trace_dump=true
    PROPERTIES "LABELS;BEMU;LABELS;PCIE;LABELS;OPS;LABELS;FUNCTIONAL;LABEL;SYSTEM;TIMEOUT;120"
)

create_test(
    TARGET     TestDevOpsApiStressBasicCmdsPcie
    TARGET_DEF TARGET_PCIE
    SOURCES    pcie/basic/TestDevOpsApiStressBasicCmdsPcie.cpp TestDevOpsApiBasicCmds.cpp
    ARGS       ""
    PROPERTIES "LABELS;BEMU;LABELS;PCIE;LABELS;OPS;LABELS;STRESS;LABEL;SYSTEM;TIMEOUT;240"
)

create_test(
    TARGET     TestDevOpsApiStressDmaCmdsPcie
    TARGET_DEF TARGET_PCIE
    SOURCES    pcie/basic/TestDevOpsApiStressDmaCmdsPcie.cpp TestDevOpsApiDmaCmds.cpp
    ARGS       ""
    PROPERTIES "LABELS;FC;LABELS;BEMU;LABELS;PCIE;LABELS;OPS;LABELS;STRESS;LABEL;SYSTEM;TIMEOUT;240"
)

create_test(
    TARGET     TestDevOpsApiStressKernelCmdsPcie
    TARGET_DEF TARGET_PCIE
    SOURCES    pcie/basic/TestDevOpsApiStressKernelCmdsPcie.cpp TestDevOpsApiKernelCmds.cpp
    ARGS       --kernels_dir=${KERNELS_DIR}
    PROPERTIES "LABELS;BEMU;LABELS;PCIE;LABELS;OPS;LABELS;STRESS;LABEL;SYSTEM;TIMEOUT;240"
)

#########################
# LOOPBACK DRIVER TESTS #
#########################
if(BUILD_PCIE_LOOPBACK_DRIVER)
    set(LOOPBACK_DRIVER "true")
    create_test(
        TARGET     TestDevOpenCloseLoopbackPcie
        TARGET_DEF TARGET_PCIE
        SOURCES    pcie/loopback/TestDevOpenCloseLoopbackPcie.cpp
        ARGS       --loopback_driver=${LOOPBACK_DRIVER}
        PROPERTIES "LABELS;LOOPBACK;LABELS;PCIE;LABELS;OPS;LABELS;FUNCTIONAL;LABEL;UNIT;TIMEOUT;120"
    )

    create_test(
        TARGET     TestDevOpsApiLoopbackBasicCmdsPcie
        TARGET_DEF TARGET_PCIE
        SOURCES    pcie/loopback/TestDevOpsApiLoopbackBasicCmdsPcie.cpp TestDevOpsApiBasicCmds.cpp
        ARGS       --loopback_driver=${LOOPBACK_DRIVER}
        PROPERTIES "LABELS;LOOPBACK;LABELS;PCIE;LABELS;OPS;LABELS;STRESS;LABEL;SYSTEM;TIMEOUT;120"
    )

    create_test(
        TARGET     TestDevOpsApiLoopbackDmaCmdsPcie
        TARGET_DEF TARGET_PCIE
        SOURCES    pcie/loopback/TestDevOpsApiLoopbackDmaCmdsPcie.cpp TestDevOpsApiDmaCmds.cpp
        ARGS       --loopback_driver=${LOOPBACK_DRIVER}
        PROPERTIES "LABELS;LOOPBACK;LABELS;PCIE;LABELS;OPS;LABELS;STRESS;LABEL;SYSTEM;TIMEOUT;120"
    )

    create_test(
        TARGET     TestDevOpsApiLoopbackKernelCmdsPcie
        TARGET_DEF TARGET_PCIE
        SOURCES    pcie/loopback/TestDevOpsApiLoopbackKernelCmdsPcie.cpp TestDevOpsApiKernelCmds.cpp
        ARGS       --kernels_dir=${KERNELS_DIR} --loopback_driver=${LOOPBACK_DRIVER}
        PROPERTIES "LABELS;LOOPBACK;LABELS;PCIE;LABELS;OPS;LABELS;STRESS;LABEL;SYSTEM;TIMEOUT;120"
    )
endif()

#################
# NIGHTLY TESTS #
#################
create_test(
    TARGET     TestDevOpsApiNightlyBasicCmdsPcie
    TARGET_DEF TARGET_PCIE
    SOURCES    pcie/nightly/TestDevOpsApiNightlyBasicCmdsPcie.cpp TestDevOpsApiBasicCmds.cpp
    ARGS       ""
    PROPERTIES "LABELS;NIGHTLY;LABELS;PCIE;LABELS;OPS;LABELS;STRESS;LABEL;SYSTEM;TIMEOUT;900"
)

create_test(
    TARGET     TestDevOpsApiNightlyDmaCmdsPcie
    TARGET_DEF TARGET_PCIE
    SOURCES    pcie/nightly/TestDevOpsApiNightlyDmaCmdsPcie.cpp TestDevOpsApiDmaCmds.cpp
    ARGS       ""
    PROPERTIES "LABELS;NIGHTLY;LABELS;PCIE;LABELS;OPS;LABELS;STRESS;LABEL;SYSTEM;TIMEOUT;600"
)

create_test(
    TARGET     TestDevOpsApiNightlyKernelCmdsPcie
    TARGET_DEF TARGET_PCIE
    SOURCES    pcie/nightly/TestDevOpsApiNightlyKernelCmdsPcie.cpp TestDevOpsApiKernelCmds.cpp
    ARGS       --kernels_dir=${KERNELS_DIR}
    PROPERTIES "LABELS;NIGHTLY;LABELS;PCIE;LABELS;OPS;LABELS;STRESS;LABEL;SYSTEM;TIMEOUT;600"
)

#################
# PCIE SYSEMU TESTS #
#################
create_test(
    TARGET     TestDevOpenClosePcieSysEmu
    TARGET_DEF TARGET_PCIE
    SOURCES    pcie/sysemu/TestDevOpenClosePcieSysEmu.cpp
    ARGS       ""
    PROPERTIES "LABELS;PCIE-SYSEMU;LABELS;OPS;LABELS;FUNCTIONAL;LABEL;UNIT;TIMEOUT;120"
)

create_test(
    TARGET     TestDevOpsApiFuncBasicCmdsPcieSysEmu
    TARGET_DEF TARGET_PCIE
    SOURCES    pcie/sysemu/TestDevOpsApiFuncBasicCmdsPcieSysEmu.cpp TestDevOpsApiBasicCmds.cpp
    ARGS       ""
    PROPERTIES "LABELS;PCIE-SYSEMU;LABELS;OPS;LABELS;FUNCTIONAL;LABEL;SYSTEM;TIMEOUT;240"
)

create_test(
    TARGET     TestDevOpsApiFuncDmaCmdsPcieSysEmu
    TARGET_DEF TARGET_PCIE
    SOURCES    pcie/sysemu/TestDevOpsApiFuncDmaCmdsPcieSysEmu.cpp TestDevOpsApiDmaCmds.cpp
    ARGS       ""
    PROPERTIES "LABELS;PCIE-SYSEMU;LABELS;OPS;LABELS;FUNCTIONAL;LABEL;SYSTEM;TIMEOUT;300"
)

create_test(
    TARGET     TestDevOpsApiFuncKernelCmdsPcieSysEmu
    TARGET_DEF TARGET_PCIE
    SOURCES    pcie/sysemu/TestDevOpsApiFuncKernelCmdsPcieSysEmu.cpp TestDevOpsApiKernelCmds.cpp
    ARGS       --kernels_dir=${KERNELS_DIR}
    PROPERTIES "LABELS;PCIE-SYSEMU;LABELS;OPS;LABELS;FUNCTIONAL;LABEL;SYSTEM;TIMEOUT;300"
)

create_test(
    TARGET     TestDevOpsApiFuncTraceCmdsPcieSysEmu
    TARGET_DEF TARGET_PCIE
    SOURCES    pcie/sysemu/TestDevOpsApiFuncTraceCmdsPcieSysEmu.cpp TestDevOpsApiTraceCmds.cpp
    ARGS       --enable_trace_dump=true
    PROPERTIES "LABELS;PCIE-SYSEMU;LABELS;OPS;LABELS;FUNCTIONAL;LABEL;SYSTEM;TIMEOUT;120"
)

create_test(
    TARGET     TestDevOpsApiStressBasicCmdsPcieSysEmu
    TARGET_DEF TARGET_PCIE
    SOURCES    pcie/sysemu/TestDevOpsApiStressBasicCmdsPcieSysEmu.cpp TestDevOpsApiBasicCmds.cpp
    ARGS       ""
    PROPERTIES "LABELS;PCIE-SYSEMU;LABELS;OPS;LABELS;STRESS;LABEL;SYSTEM;TIMEOUT;300"
)

create_test(
    TARGET     TestDevOpsApiStressDmaCmdsPcieSysEmu
    TARGET_DEF TARGET_PCIE
    SOURCES    pcie/sysemu/TestDevOpsApiStressDmaCmdsPcieSysEmu.cpp TestDevOpsApiDmaCmds.cpp
    ARGS       ""
    PROPERTIES "LABELS;PCIE-SYSEMU;LABELS;OPS;LABELS;STRESS;LABEL;SYSTEM;TIMEOUT;300"
)

create_test(
    TARGET     TestDevOpsApiStressKernelCmdsPcieSysEmu
    TARGET_DEF TARGET_PCIE
    SOURCES    pcie/sysemu/TestDevOpsApiStressKernelCmdsPcieSysEmu.cpp TestDevOpsApiKernelCmds.cpp
    ARGS       --kernels_dir=${KERNELS_DIR}
    PROPERTIES "LABELS;PCIE-SYSEMU;LABELS;OPS;LABELS;STRESS;LABEL;SYSTEM;TIMEOUT;500"
)
