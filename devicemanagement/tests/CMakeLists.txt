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

# This will be used by test case to verify f/w update support as this is CMakeLists for DM tests.
# End user can use f/w image of choice by passing it as argument to dev_mngt_service.
set(FLASH_IMG_PATH "${FLASH_INSTALL_DIR}/dev-image/production-signed-with-test-keys/flash_32Mbit.bin")

include_directories(${CMAKE_CURRENT_BINARY_DIR})
configure_file(Autogen.h.in Autogen.h)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/Autogen.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/device-management
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
    target_include_directories(${TEST_TARGET}
        PRIVATE
            $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
            ${CMAKE_CURRENT_SOURCE_DIR}
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

# All tests should be created using the helper

####################
# FUNCTIONAL TESTS #
####################
create_test(
    TARGET	FunctionalTestDevMgmtApiAssetTrackingCmds
    SOURCES	functional/FunctionalTestDevMgmtApiAssetTrackingCmds.cpp TestDevMgmtApiSyncCmds.cpp
    ARGS	""
    PROPERTIES	"LABELS;FUNCTIONAL;TIMEOUT;60"
)

create_test(
    TARGET      FunctionalTestDevMgmtApiThermalAndPowerMonitoringCmds
    SOURCES     functional/FunctionalTestDevMgmtApiThermalAndPowerMonitoringCmds.cpp TestDevMgmtApiSyncCmds.cpp
    ARGS        ""
    PROPERTIES  "LABELS;FUNCTIONAL;TIMEOUT;60"
)

create_test(
    TARGET      FunctionalTestDevMgmtApiHistoricalExtremeValCmds
    SOURCES     functional/FunctionalTestDevMgmtApiHistoricalExtremeValCmds.cpp TestDevMgmtApiSyncCmds.cpp
    ARGS        ""
    PROPERTIES  "LABELS;FUNCTIONAL;TIMEOUT;60"
)

create_test(
    TARGET      FunctionalTestDevMgmtApiErrorControlCmds
    SOURCES     functional/FunctionalTestDevMgmtApiErrorControlCmds.cpp TestDevMgmtApiSyncCmds.cpp
    ARGS        ""
    PROPERTIES  "LABELS;FUNCTIONAL;TIMEOUT;60"
)

create_test(
    TARGET      FunctionalTestDevMgmtApiLinkMgmtCmds
    SOURCES     functional/FunctionalTestDevMgmtApiLinkMgmtCmds.cpp TestDevMgmtApiSyncCmds.cpp
    ARGS        ""
    PROPERTIES  "LABELS;FUNCTIONAL;TIMEOUT;60"
)

create_test(
    TARGET      FunctionalTestDevMgmtApiPerfMgmtCmds
    SOURCES     functional/FunctionalTestDevMgmtApiPerfMgmtCmds.cpp TestDevMgmtApiSyncCmds.cpp
    ARGS        ""
    PROPERTIES  "LABELS;FUNCTIONAL;TIMEOUT;60"
)

create_test(
    TARGET      FunctionalTestDevMgmtApiFirmwareMgmtCmds
    SOURCES     functional/FunctionalTestDevMgmtApiFirmwareMgmtCmds.cpp TestDevMgmtApiSyncCmds.cpp
    ARGS        ""
    PROPERTIES  "LABELS;FUNCTIONAL;TIMEOUT;60"
)

#####################
# INTEGRATION TESTS #
#####################
create_test(
    TARGET      IntegrationTestDevMgmtApiCmds
    SOURCES     integration/IntegrationTestDevMgmtApiCmds.cpp TestDevMgmtApiSyncCmds.cpp
    ARGS        ""
    PROPERTIES  "LABELS;INTEGRATION;TIMEOUT;300"
)

create_test(
    TARGET      IntegrationTestDevMgmtApiTraceCmds
    SOURCES     integration/IntegrationTestDevMgmtApiTraceCmds.cpp TestDevMgmtApiSyncCmds.cpp
    ARGS        ""
    PROPERTIES  "LABELS;INTEGRATION;TIMEOUT;300"
)

#####################
# MDI DEBUG TESTS #
#####################
create_test(
    TARGET      RunControlApiTestcmds
    SOURCES     mdi_debug/RunControlApiTestcmds.cpp TestDevMgmtApiSyncCmds.cpp
    ARGS        ""
    PROPERTIES  "LABELS;MDI;TIMEOUT;300"
)

create_test(
    TARGET      StateInspectionApiTestcmds
    SOURCES     mdi_debug/StateInspectionApiTestcmds.cpp TestDevMgmtApiSyncCmds.cpp
    ARGS        ""
    PROPERTIES  "LABELS;MDI;TIMEOUT;300"
)

##################
# SECURITY TESTS #
##################
create_test(
    TARGET      SecurityTestDevMgmtApiGenericCmds
    SOURCES     security/SecurityTestDevMgmtApiGenericCmds.cpp TestDevMgmtApiSyncCmds.cpp
    ARGS        ""
    PROPERTIES  "LABELS;SECURITY;TIMEOUT;60"
)

create_test(
    TARGET      SecurityTestDevMgmtApiAssetTrackingCmds
    SOURCES     security/SecurityTestDevMgmtApiAssetTrackingCmds.cpp TestDevMgmtApiSyncCmds.cpp
    ARGS        ""
    PROPERTIES  "LABELS;SECURITY;TIMEOUT;60"
)

create_test(
    TARGET      SecurityTestDevMgmtApiThermalAndPowerMonitoringCmds
    SOURCES     security/SecurityTestDevMgmtApiThermalAndPowerMonitoringCmds.cpp TestDevMgmtApiSyncCmds.cpp
    ARGS        ""
    PROPERTIES  "LABELS;SECURITY;TIMEOUT;60"
)

create_test(
    TARGET      SecurityTestDevMgmtApiHistoricalExtremeValCmds
    SOURCES     security/SecurityTestDevMgmtApiHistoricalExtremeValCmds.cpp TestDevMgmtApiSyncCmds.cpp
    ARGS        ""
    PROPERTIES  "LABELS;SECURITY;TIMEOUT;60"
)

create_test(
    TARGET      SecurityTestDevMgmtApiErrorControlCmds
    SOURCES     security/SecurityTestDevMgmtApiErrorControlCmds.cpp TestDevMgmtApiSyncCmds.cpp
    ARGS        ""
    PROPERTIES  "LABELS;SECURITY;TIMEOUT;60"
)

create_test(
    TARGET      SecurityTestDevMgmtApiLinkMgmtCmds
    SOURCES     security/SecurityTestDevMgmtApiLinkMgmtCmds.cpp TestDevMgmtApiSyncCmds.cpp
    ARGS        ""
    PROPERTIES  "LABELS;SECURITY;TIMEOUT;60"
)

create_test(
    TARGET      SecurityTestDevMgmtApiPerfMgmtCmds
    SOURCES     security/SecurityTestDevMgmtApiPerfMgmtCmds.cpp TestDevMgmtApiSyncCmds.cpp
    ARGS        ""
    PROPERTIES  "LABELS;SECURITY;TIMEOUT;60"
)

create_test(
    TARGET      OpsNodeDependentTestDevMgmtApiMMResetCmds
    SOURCES     opsNodeDependent/OpsNodeDependentTestDevMgmtApiMMResetCmds.cpp TestDevMgmtApiSyncCmds.cpp
    ARGS        ""
    PROPERTIES  "LABELS;OPS_NODE_DEPENDENT;TIMEOUT;60"
)

create_test(
    TARGET      OpsNodeDependentTestDevMgmtApiTraceCmds
    SOURCES     opsNodeDependent/OpsNodeDependentTestDevMgmtApiTraceCmds.cpp TestDevMgmtApiSyncCmds.cpp
    ARGS        ""
    PROPERTIES  "LABELS;OPS_NODE_DEPENDENT;TIMEOUT;60"
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
    DEPENDS           OpsNodeDependentTestDevMgmtApiMMResetCmds
)
