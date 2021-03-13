#------------------------------------------------------------------------------
# Copyright (C) 2019, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

set(FILE_VERSION_MAJOR 0)
set(FILE_VERSION_MINOR 0)
set(FILE_REVISION_NUMBER 1)

set(BL2_INSTALL_DIR ${LIB_INSTALL_DIR}/esperanto-fw/ServiceProcessorBL2)

set(BL2_SOURCES
    common/entry.S
    common/trap_handler.S
    common/main.c
    config/dir_regs.c
    interface/sp_host_iface.c
    interface/sp_mm_iface.c
    utils/stubs.c
    utils/crc32.c
    utils/memory_copy_8x32.S
    driver/io_pll.c
    driver/minion_pll_and_dll.c
    driver/ddr_controller.c
    driver/i2c_controller.c
    driver/spi_controller.c
    driver/spi_flash.c
    driver/vaultip_controller.c
    driver/pmic_controller.c
    driver/crypto_rot.c
    driver/reset.c
    driver/timer.c
    driver/flashfs.c
    driver/secure_sign.c
    driver/firmware_loader.c
    driver/interrupt.c
    driver/bl2_pcie.c
    driver/bl2_sram.c
    driver/bl2_otp.c
    rtos_task/command_dispatcher.c
    rtos_task/flash_fs.c
    rtos_task/vaultip.c
    rtos_task/dm_task.c
    rtos_task/event_handler.c
    services/firmware_update.c
    services/asset_track.c
    services/asset_track_mgmt.c
    services/thermal_power_monitor.c
    services/performance.c
    services/link_mgmt.c
    services/error_control.c
    services/historical_extreme.c
    services/minion_state.c
    services/dm_events.c
    services/perf_mgmt.c
    services/thermal_pwr_mgmt.c 
    # This file get compiled different based on the configuration of each target
    common/build_configuration.c)

list(TRANSFORM BL2_SOURCES PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/)

set(SOURCES
    ${BL2_SOURCES}
    ${FREERTOS_SRC_FILES}
    ${SHARED_SRC_DIR}/constant_memory_compare.c
    ${SHARED_SRC_DIR}/pcie_init.c
    ${SHARED_SRC_DIR}/serial.c
    ${SHARED_SRC_DIR}/sp_otp.c
    ${SHARED_SRC_DIR}/usb2_0.c
)

set(LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/common/sections.ld)
set(ZEBU_TARGET SP_RAM)

# FreeRTOS definitions
add_definitions("-DportasmHANDLE_INTERRUPT=external_interrupt_handler")

include_directories(include)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/include)
include_directories(${SHARED_INC_DIR})
include_directories(${FREERTOS_INC_DIRS})
include_directories(${FLASH_TOOL_HEADERS})
include_directories(${PKCS11_HEADERS})
include_directories(${CRYPTO_HEADERS})
include_directories(${ESPERANTO_DEVICE_API_INCLUDE_DIR})

add_subdirectory(include)

# BL2 build variants
add_subdirectory(build-variants)
