#------------------------------------------------------------------------------
# Copyright (C) 2021, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

add_library(firmware_helpers_interface INTERFACE)
add_library(deviceFirmware::firmware_helpers_interface ALIAS firmware_helpers_interface)
target_include_directories(firmware_helpers_interface INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(firmware_helpers_interface INTERFACE deviceFirmware::device_common)

install(TARGETS firmware_helpers_interface
    EXPORT EsperantoDeviceMinionRuntimeTargets
    LIBRARY       DESTINATION ${CMAKE_INSTALL_LIBDIR}                                   COMPONENT firmware_helpers_interface
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/esperanto-fw/firmware_helpers COMPONENT firmware_helpers_interface
)

add_library(firmware_helpers_serial STATIC src/serial.c)
add_library(deviceFirmware::firmware_helpers_serial ALIAS firmware_helpers_serial)
target_include_directories(firmware_helpers_serial PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(firmware_helpers_serial PUBLIC etsoc_hal::etsoc_hal)

set(FIRMWARE_HELPERS_SOURCES
    src/pcie_int.c
    src/etsoc_memory.c
    src/circbuff.c
    src/vq.c
    src/sp_mm_iface.c
)

# create a static library with MasterMinion compile time definition
add_library(mm_firmware_helpers STATIC ${FIRMWARE_HELPERS_SOURCES})
target_compile_definitions(mm_firmware_helpers PUBLIC -DMASTER_MINION=1)
target_include_directories(mm_firmware_helpers
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/esperanto-fw/firmware_helpers>
    PRIVATE
        ${SHARED_INC_DIRS}
)
target_link_libraries(mm_firmware_helpers
    PUBLIC
        deviceFirmware::device_common
        etsoc_hal::etsoc_hal
)

install(TARGETS mm_firmware_helpers
    EXPORT EsperantoDeviceMinionRuntimeTargets
    ARCHIVE       DESTINATION ${CMAKE_INSTALL_LIBDIR}/esperanto-fw                      COMPONENT firmware_helpers
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/esperanto-fw/firmware_helpers COMPONENT firmware_helpers
)

# create a static library with ServiceProcessorBL2 compile time definition
add_library(sp_firmware_helpers STATIC ${FIRMWARE_HELPERS_SOURCES})
target_compile_definitions(sp_firmware_helpers PUBLIC -DSERVICE_PROCESSOR_BL2=1)

target_include_directories(sp_firmware_helpers
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/esperanto-fw/firmware_helpers>
    PRIVATE
        ${SHARED_INC_DIRS}
)

target_link_libraries(sp_firmware_helpers
    PUBLIC
        deviceFirmware::device_common
        etsoc_hal::etsoc_hal
)

install(TARGETS sp_firmware_helpers
    EXPORT EsperantoDeviceMinionRuntimeTargets
    ARCHIVE       DESTINATION ${CMAKE_INSTALL_LIBDIR}/esperanto-fw                      COMPONENT firmware_helpers
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/esperanto-fw/firmware_helpers COMPONENT firmware_helpers
)

install(FILES
    include/layout.h
    include/minion_esr_defines.h
    include/minion_fw_boot_config.h
    include/pcie_device.h
    include/pcie_int.h
    include/common_defs.h
    include/circbuff.h
    include/etsoc_memory.h
    include/vq.h
    include/sp_mm_shared_config.h
    include/sp_mm_iface.h
    include/sp_mm_comms_spec.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/esperanto-fw/firmware_helpers COMPONENT firmware_helpers
)
