#------------------------------------------------------------------------------
# Copyright (C) 2020, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------


include(ExternalProject)
include(GoogleTest)

enable_testing()

find_package(deviceLayer REQUIRED)
find_package(GTest REQUIRED)
find_package(gflags REQUIRED)
find_package(glog REQUIRED)
find_package(Threads REQUIRED)
find_package(EsperantoDeviceMinionRuntime REQUIRED)
find_package(EsperantoBootLoader REQUIRED)
find_package(esperantoTrace REQUIRED)

get_property(BOOTROM_TRAMPOLINE_TO_BL2_ELF TARGET EsperantoBootLoader::BootromTrampolineToBL2.elf PROPERTY LOCATION)
get_property(BL2_ELF TARGET EsperantoBootLoader::ServiceProcessorBL2_fast-boot.elf PROPERTY LOCATION)
get_property(BL2_ELF_TF TARGET EsperantoBootLoader::ServiceProcessorBL2_testframework.elf PROPERTY LOCATION)
get_property(MASTER_MINION_ELF TARGET EsperantoDeviceMinionRuntime::MasterMinion.elf PROPERTY LOCATION)
get_property(MASTER_MINION_ELF_TF TARGET EsperantoDeviceMinionRuntime::MasterMinionTF.elf PROPERTY LOCATION)
get_property(MACHINE_MINION_ELF TARGET EsperantoDeviceMinionRuntime::MachineMinion.elf PROPERTY LOCATION)
get_property(WORKER_MINION_ELF TARGET EsperantoDeviceMinionRuntime::WorkerMinion.elf PROPERTY LOCATION)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
configure_file(Autogen.h.in Autogen.h)

# Helper to create tests
function(create_test)
    cmake_parse_arguments(TEST
        ""
        "TARGET"
        "SOURCES;ARGS;PROPERTIES"
        ${ARGN}
    )

    add_executable(${TEST_TARGET} ${TEST_SOURCES})
    target_compile_features(${TEST_TARGET} PRIVATE cxx_std_17)
    target_include_directories(${TEST_TARGET}
        PRIVATE 
            $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
    )
    target_link_libraries(${TEST_TARGET}
        PRIVATE
            deviceManagement::DM
            deviceLayer::deviceLayer
            GTest::GTest
            GTest::Main
            gflags::gflags
            ${CMAKE_DL_LIBS}
    )

    gtest_discover_tests(${TEST_TARGET}
        EXTRA_ARGS ${TEST_ARGS}
        TEST_PREFIX deviceManagement:
        PROPERTIES ${TEST_PROPERTIES}
        TEST_LIST DISCOVERED_TESTS
    )
endfunction()

if(BUILD_PCIE_LOOPBACK_DRIVER)
    set(LOOPBACK_DRIVER "true")
else()
    set(LOOPBACK_DRIVER "false")
endif()

# All tests should be created using the helper

###############
# BASIC TESTS #
###############
create_test(
    TARGET     TestDevMgmtApiFuncSyncCmdsSysEmu
    SOURCES    TestDevMgmtApiFuncSyncCmdsSysEmu.cpp TestDevMgmtApiSyncCmds.cpp
    ARGS       ""
    PROPERTIES "LABELS;SYSEMU;LABELS;MGMT;LABELS;FUNCTIONAL;LABELS;SYSTEM;TIMEOUT;120"
)

create_test(
    TARGET     TestDevMgmtApiFuncSyncCmdsPcie
    SOURCES    TestDevMgmtApiFuncSyncCmdsPcie.cpp TestDevMgmtApiSyncCmds.cpp
    ARGS       --loopback_driver=${LOOPBACK_DRIVER}
    PROPERTIES "LABELS;FC;LABELS;BEMU;LABELS;PCIE;LABELS;MGMT;LABELS;FUNCTIONAL;LABELS;SYSTEM;LABELS;LOOPBACK;LABELS;BRINGUP-DROP2;TIMEOUT;600"
)

create_test(
    TARGET     TestDevMgmtApiFuncSyncCmdsPcieFullBoot
    SOURCES    TestDevMgmtApiFuncSyncCmdsPcieFullBoot.cpp TestDevMgmtApiSyncCmds.cpp
    ARGS       ""
    PROPERTIES "LABELS;FC;LABELS;PCIE-FULLBOOT;LABELS;MGMT;LABELS;FUNCTIONAL;LABELS;SYSTEM;LABELS;BRINGUP-DROP5;TIMEOUT;120"
)

create_test(
    TARGET     TestDevMgmtApiFuncSyncCmdsPcieSysEmu
    SOURCES    TestDevMgmtApiFuncSyncCmdsPcieSysEmu.cpp TestDevMgmtApiSyncCmds.cpp
    ARGS       ""
    PROPERTIES "LABELS;PCIE-SYSEMU;LABELS;MGMT;LABELS;FUNCTIONAL;LABELS;SYSTEM;TIMEOUT;600"
)

configure_file(
    TestDevMgmtApiLoopbackMixAllCmdsPcie.sh.in
    ${CMAKE_CURRENT_BINARY_DIR}/TestDevMgmtApiLoopbackMixAllCmdsPcie.sh @ONLY
)

set(TEST_MINOR 1)
foreach (TIME_IN_MIN 5 15 30 45 60)
    set(TEST_NAME deviceManagement:TestDevMgmtApiLoopbackMixAllCmdsPcie.${TIME_IN_MIN}minTest_4_${TEST_MINOR})
    add_test(
        NAME              ${TEST_NAME}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMAND           TestDevMgmtApiLoopbackMixAllCmdsPcie.sh ${TIME_IN_MIN}minute
    )

    set_tests_properties(${TEST_NAME}
        PROPERTIES
        DEPENDS TestDevMgmtApiFuncSyncCmdsPcieSysEmu
    )
    MATH(EXPR TEST_MINOR "${TEST_MINOR}+1")
endforeach()
