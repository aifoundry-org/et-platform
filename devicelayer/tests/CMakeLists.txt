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

find_package(EsperantoDeviceMinionRuntime REQUIRED)
find_package(EsperantoBootLoader REQUIRED)

get_property(BOOTROM_TRAMPOLINE_TO_BL2_ELF TARGET EsperantoBootLoader::BootromTrampolineToBL2.elf PROPERTY LOCATION)
get_property(BL2_ELF TARGET EsperantoBootLoader::ServiceProcessorBL2_fast-boot.elf PROPERTY LOCATION)
get_property(MASTER_MINION_ELF TARGET EsperantoDeviceMinionRuntime::MasterMinion.elf PROPERTY LOCATION)
get_property(MACHINE_MINION_ELF TARGET EsperantoDeviceMinionRuntime::MachineMinion.elf PROPERTY LOCATION)
get_property(WORKER_MINION_ELF TARGET EsperantoDeviceMinionRuntime::WorkerMinion.elf PROPERTY LOCATION)

configure_file(Autogen.h.in Autogen.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(integration)
add_subdirectory(device-bootloaders)
add_subdirectory(device-minion-runtime)
add_subdirectory(bringup-tests)


install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/Autogen.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/device-layer
)
