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
find_package(fmt REQUIRED)
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

# This will be used by test case to verify f/w update support as this is CMakeLists for DM tests.
# End user can use f/w image of choice by passing it as argument to dev_mngt_service.
set(FLASH_IMG_PATH "${FLASH_INSTALL_DIR}/dev-image/production-signed-with-test-keys-pcie-phy-fw-override/flash_32Mbit.bin")
set(FLASH_TOOL ${CMAKE_INSTALL_PREFIX}/bin/esperanto_flash_tool)
set(DEVICE_SIGNED_ARTIFACTS_DIR ${CMAKE_INSTALL_PREFIX}/lib/esperanto-fw/device-artifacts/signed-artifacts/test-key-artifacts)
set(FLASH_32MBIT_TEMPLATE ${ROOT_BINARY_DIR}/device-software/device-artifacts/flash-image/src/device-artifacts-flash-image-build/dev-image/production-signed-with-test-keys-pcie-phy-fw-override/flash_32Mbit_template.json)

option(ENABLE_SANITIZER_ADDRESS "" OFF)
option(ENABLE_SANITIZER_LEAK "" OFF)
option(ENABLE_SANITIZER_UNDEFINED_BEHAVIOR "" OFF)
option(ENABLE_SANITIZER_THREAD "" OFF)
option(ENABLE_SANITIZER_MEMORY "" OFF)
include(CompilerSanitizers)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
configure_file(Autogen.h.in Autogen.h)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/Autogen.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/device-management
)

add_library(DMTests
    DevErrorEvent.cpp
    TestDevMgmtApiSyncCmds.cpp
)

target_compile_features(DMTests PRIVATE cxx_std_17)
target_include_directories(DMTests
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
        ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(DMTests
    PUBLIC
        deviceManagement::DM
        deviceLayer::deviceLayer
        fmt::fmt
        GTest::GTest
        GTest::Main
        gflags::gflags
        ${CMAKE_DL_LIBS}
)

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
    target_link_libraries(${TEST_TARGET} PRIVATE DMTests)
    target_add_sanitizers(${TEST_TARGET})

    # set internal execution timeout value relative to actual timeout. Internal timeout
    # is smaller for wraping up the execution and printing the test summary
    string(REGEX MATCH "[T][I][M][E][O][U][T][0-9]*" TIMEOUT_ENTRY ${TEST_PROPERTIES})
    string(REGEX REPLACE "[T][I][M][E][O][U][T]" "" TIMEOUT_VAL ${TIMEOUT_ENTRY})
    if(TIMEOUT_VAL STREQUAL "")
        # default timeout value
        set(TIMEOUT_VAL "60")
    endif()
    set(WRAP_UP_TIME "15")
    MATH(EXPR EXEC_TIMEOUT_MS "(${TIMEOUT_VAL}-${WRAP_UP_TIME})*1000")
    set(TEST_ARGS ${TEST_ARGS} --exec_timeout_ms=${EXEC_TIMEOUT_MS})

    gtest_discover_tests(${TEST_TARGET}
        EXTRA_ARGS ${TEST_ARGS}
        TEST_PREFIX deviceManagement:
        PROPERTIES ${TEST_PROPERTIES}
        TEST_LIST DISCOVERED_TESTS
    )
endfunction()

# All tests should be created using the helper

####################
# FUNCTIONAL TESTS #
####################
create_test(
    TARGET	FunctionalTestDevMgmtApiAssetTrackingCmds
    SOURCES	functional/FunctionalTestDevMgmtApiAssetTrackingCmds.cpp
    ARGS	""
    PROPERTIES	"LABELS;FUNCTIONAL;TIMEOUT;60"
)

create_test(
    TARGET      FunctionalTestDevMgmtApiThermalAndPowerMonitoringCmds
    SOURCES     functional/FunctionalTestDevMgmtApiThermalAndPowerMonitoringCmds.cpp
    ARGS        ""
    PROPERTIES  "LABELS;FUNCTIONAL;TIMEOUT;60"
)

create_test(
    TARGET      FunctionalTestDevMgmtApiHistoricalExtremeValCmds
    SOURCES     functional/FunctionalTestDevMgmtApiHistoricalExtremeValCmds.cpp
    ARGS        ""
    PROPERTIES  "LABELS;FUNCTIONAL;TIMEOUT;60"
)

create_test(
    TARGET      FunctionalTestDevMgmtApiErrorControlCmds
    SOURCES     functional/FunctionalTestDevMgmtApiErrorControlCmds.cpp
    ARGS        ""
    PROPERTIES  "LABELS;FUNCTIONAL;TIMEOUT;60"
)

create_test(
    TARGET      FunctionalTestDevMgmtApiLinkMgmtCmds
    SOURCES     functional/FunctionalTestDevMgmtApiLinkMgmtCmds.cpp
    ARGS        ""
    PROPERTIES  "LABELS;FUNCTIONAL;TIMEOUT;60"
)

create_test(
    TARGET      FunctionalTestDevMgmtApiPerfMgmtCmds
    SOURCES     functional/FunctionalTestDevMgmtApiPerfMgmtCmds.cpp
    ARGS        ""
    PROPERTIES  "LABELS;FUNCTIONAL;TIMEOUT;60"
)

create_test(
    TARGET      FunctionalTestDevMgmtApiFirmwareMgmtCmds
    SOURCES     functional/FunctionalTestDevMgmtApiFirmwareMgmtCmds.cpp
    ARGS        ""
    PROPERTIES  "LABELS;FUNCTIONAL;TIMEOUT;240"
)

#####################
# INTEGRATION TESTS #
#####################
create_test(
    TARGET      IntegrationTestDevMgmtApiCmds
    SOURCES     integration/IntegrationTestDevMgmtApiCmds.cpp
    ARGS        ""
    PROPERTIES  "LABELS;INTEGRATION;TIMEOUT;300"
)

create_test(
    TARGET      IntegrationTestDevMgmtApiTraceCmds
    SOURCES     integration/IntegrationTestDevMgmtApiTraceCmds.cpp
    ARGS        ""
    PROPERTIES  "LABELS;INTEGRATION;TIMEOUT;300"
)

create_test(
    TARGET      IntegrationTestDevMgmtApiFirmwareMgmtCmds
    SOURCES     integration/IntegrationTestDevMgmtApiFirmwareMgmtCmds.cpp
    ARGS        ""
    PROPERTIES  "LABELS;INTEGRATION;TIMEOUT;300"
)

#####################
# MDI DEBUG TESTS #
#####################
create_test(
    TARGET      RunControlApiTestcmds
    SOURCES     mdi_debug/RunControlApiTestcmds.cpp
    ARGS        ""
    PROPERTIES  "LABELS;MDI;TIMEOUT;300"
)

create_test(
    TARGET      StateInspectionApiTestcmds
    SOURCES     mdi_debug/StateInspectionApiTestcmds.cpp
    ARGS        ""
    PROPERTIES  "LABELS;MDI;TIMEOUT;300"
)

##################
# SECURITY TESTS #
##################
create_test(
    TARGET      SecurityTestDevMgmtApiGenericCmds
    SOURCES     security/SecurityTestDevMgmtApiGenericCmds.cpp
    ARGS        ""
    PROPERTIES  "LABELS;SECURITY;TIMEOUT;60"
)

create_test(
    TARGET      SecurityTestDevMgmtApiAssetTrackingCmds
    SOURCES     security/SecurityTestDevMgmtApiAssetTrackingCmds.cpp
    ARGS        ""
    PROPERTIES  "LABELS;SECURITY;TIMEOUT;60"
)

create_test(
    TARGET      SecurityTestDevMgmtApiThermalAndPowerMonitoringCmds
    SOURCES     security/SecurityTestDevMgmtApiThermalAndPowerMonitoringCmds.cpp
    ARGS        ""
    PROPERTIES  "LABELS;SECURITY;TIMEOUT;60"
)

create_test(
    TARGET      SecurityTestDevMgmtApiHistoricalExtremeValCmds
    SOURCES     security/SecurityTestDevMgmtApiHistoricalExtremeValCmds.cpp
    ARGS        ""
    PROPERTIES  "LABELS;SECURITY;TIMEOUT;60"
)

create_test(
    TARGET      SecurityTestDevMgmtApiErrorControlCmds
    SOURCES     security/SecurityTestDevMgmtApiErrorControlCmds.cpp
    ARGS        ""
    PROPERTIES  "LABELS;SECURITY;TIMEOUT;60"
)

create_test(
    TARGET      SecurityTestDevMgmtApiLinkMgmtCmds
    SOURCES     security/SecurityTestDevMgmtApiLinkMgmtCmds.cpp
    ARGS        ""
    PROPERTIES  "LABELS;SECURITY;TIMEOUT;60"
)

create_test(
    TARGET      SecurityTestDevMgmtApiPerfMgmtCmds
    SOURCES     security/SecurityTestDevMgmtApiPerfMgmtCmds.cpp
    ARGS        ""
    PROPERTIES  "LABELS;SECURITY;TIMEOUT;60"
)

create_test(
    TARGET      OpsNodeDependentTestDevMgmtApiFirmwareMgmtCmds
    SOURCES     opsNodeDependent/OpsNodeDependentTestDevMgmtApiFirmwareMgmtCmds.cpp
    ARGS        ""
    PROPERTIES  "LABELS;OPS_NODE_DEPENDENT;TIMEOUT;120"
)

create_test(
    TARGET      OpsNodeDependentTestDevMgmtApiTraceCmds
    SOURCES     opsNodeDependent/OpsNodeDependentTestDevMgmtApiTraceCmds.cpp
    ARGS        ""
    PROPERTIES  "LABELS;OPS_NODE_DEPENDENT;TIMEOUT;60"
)

################
# STRESS TESTS #
################
create_test(
    TARGET      StressTestDevMgmtApiFirmwareMgmtCmds
    SOURCES     stress/StressTestDevMgmtApiFirmwareMgmtCmds.cpp
    ARGS        ""
    PROPERTIES  "LABELS;STRESS;TIMEOUT;800"
)

configure_file(
    mix/MixTestDevMgmtApiAllCmds.sh.in
    ${CMAKE_CURRENT_BINARY_DIR}/MixTestDevMgmtApiAllCmds.sh @ONLY
)

add_test(
    NAME              deviceManagement:MixTestDevMgmtApiAllCmds.randomizeTests
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND           MixTestDevMgmtApiAllCmds.sh
)

set_tests_properties(${TEST_NAME}
    PROPERTIES
    DEPENDS           FunctionalTestDevMgmtApiAssetTrackingCmds
    DEPENDS           FunctionalTestDevMgmtApiThermalAndPowerMonitoringCmds
    DEPENDS           FunctionalTestDevMgmtApiHistoricalExtremeValCmds
    DEPENDS           FunctionalTestDevMgmtApiErrorControlCmds
    DEPENDS           FunctionalTestDevMgmtApiLinkMgmtCmds
    DEPENDS           FunctionalTestDevMgmtApiPerfMgmtCmds
    DEPENDS           FunctionalTestDevMgmtApiFirmwareMgmtCmds
    DEPENDS           IntegrationTestDevMgmtApiCmds
    DEPENDS           IntegrationTestDevMgmtApiTraceCmds
    DEPENDS           RunControlApiTestcmds
    DEPENDS           StateInspectionApiTestcmds
    DEPENDS           SecurityTestDevMgmtApiGenericCmds
    DEPENDS           SecurityTestDevMgmtApiAssetTrackingCmds
    DEPENDS           SecurityTestDevMgmtApiThermalAndPowerMonitoringCmds
    DEPENDS           SecurityTestDevMgmtApiHistoricalExtremeValCmds
    DEPENDS           SecurityTestDevMgmtApiErrorControlCmds
    DEPENDS           SecurityTestDevMgmtApiLinkMgmtCmds
    DEPENDS           SecurityTestDevMgmtApiPerfMgmtCmds
)
