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

add_subdirectory(include)

set(LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/common/sections.ld)
set(LINKER_SCRIPT_DEPENDENCY ${SHARED_INC_DIR}/bl_layout.ld)
set(ZEBU_TARGET SP_RAM)

# BL2 build variants
foreach(BL2_BUILD_VARIANT IN ITEMS production fast-boot testframework mdi_enabled)
    set(TARGET_NAME ServiceProcessorBL2_${BL2_BUILD_VARIANT})

    if (BL2_BUILD_VARIANT STREQUAL "fast-boot")
        # Fast Boot version
        set(FAST_BOOT 1)
        set(TEST_FRAMEWORK 0)
        set(MINION_DEBUG_INTERFACE 0)
    elseif (BL2_BUILD_VARIANT STREQUAL "testframework")
        # Test Frame work version
        set(FAST_BOOT 0)
        set(TEST_FRAMEWORK 1)
        set(MINION_DEBUG_INTERFACE 0)
    elseif (BL2_BUILD_VARIANT STREQUAL "production")
        # Production version
        set(FAST_BOOT 0)
        set(TEST_FRAMEWORK 0)
        set(MINION_DEBUG_INTERFACE 0)
    else()
        # Debug enabled
        set(FAST_BOOT 1)
        set(TEST_FRAMEWORK 0)  
        set(MINION_DEBUG_INTERFACE 1)
    endif()

    add_riscv_executable(${TARGET_NAME}
        ${CMAKE_CURRENT_SOURCE_DIR}/common/entry.S
        ${CMAKE_CURRENT_SOURCE_DIR}/common/trap_handler.S
        ${CMAKE_CURRENT_SOURCE_DIR}/common/main.c
        ${CMAKE_CURRENT_SOURCE_DIR}/config/dir_regs.c
        ${CMAKE_CURRENT_SOURCE_DIR}/interface/sp_host_iface.c
        ${CMAKE_CURRENT_SOURCE_DIR}/interface/mm_iface.c
        ${CMAKE_CURRENT_SOURCE_DIR}/utils/stubs.c
        ${CMAKE_CURRENT_SOURCE_DIR}/utils/crc32.c
        ${CMAKE_CURRENT_SOURCE_DIR}/utils/memory_copy_8x32.S
        ${CMAKE_CURRENT_SOURCE_DIR}/utils/mem_controller_utils.c
        ${CMAKE_CURRENT_SOURCE_DIR}/utils/usdelay.c
        ${CMAKE_CURRENT_SOURCE_DIR}/driver/io_pll.c
        ${CMAKE_CURRENT_SOURCE_DIR}/driver/minion_configuration.c
        ${CMAKE_CURRENT_SOURCE_DIR}/driver/maxion_configuration.c
        ${CMAKE_CURRENT_SOURCE_DIR}/driver/noc_configuration.c
        ${CMAKE_CURRENT_SOURCE_DIR}/driver/mem_controller.c
        ${CMAKE_CURRENT_SOURCE_DIR}/driver/i2c_controller.c
        ${CMAKE_CURRENT_SOURCE_DIR}/driver/spi_controller.c
        ${CMAKE_CURRENT_SOURCE_DIR}/driver/spi_flash.c
        ${CMAKE_CURRENT_SOURCE_DIR}/driver/vaultip_controller.c
        ${CMAKE_CURRENT_SOURCE_DIR}/driver/gpio_controller.c
        ${CMAKE_CURRENT_SOURCE_DIR}/driver/pmic_controller.c
        ${CMAKE_CURRENT_SOURCE_DIR}/driver/pvt_controller.c
        ${CMAKE_CURRENT_SOURCE_DIR}/driver/reset.c
        ${CMAKE_CURRENT_SOURCE_DIR}/driver/timer.c
        ${CMAKE_CURRENT_SOURCE_DIR}/driver/flashfs.c
        ${CMAKE_CURRENT_SOURCE_DIR}/driver/secure_sign.c
        ${CMAKE_CURRENT_SOURCE_DIR}/driver/interrupt.c
        ${CMAKE_CURRENT_SOURCE_DIR}/driver/pcie_configuration.c
        ${CMAKE_CURRENT_SOURCE_DIR}/driver/bl2_cache_control.c
        ${CMAKE_CURRENT_SOURCE_DIR}/driver/bl2_watchdog.c
        ${CMAKE_CURRENT_SOURCE_DIR}/driver/bl2_otp.c
        ${CMAKE_CURRENT_SOURCE_DIR}/driver/spio_sram.c
        ${CMAKE_CURRENT_SOURCE_DIR}/driver/bl2_pu.c
        ${CMAKE_CURRENT_SOURCE_DIR}/rtos_task/command_dispatcher.c
        ${CMAKE_CURRENT_SOURCE_DIR}/rtos_task/flash_fs.c
        ${CMAKE_CURRENT_SOURCE_DIR}/rtos_task/vaultip.c
        ${CMAKE_CURRENT_SOURCE_DIR}/rtos_task/dm_task.c
        ${CMAKE_CURRENT_SOURCE_DIR}/rtos_task/event_handler.c
        ${CMAKE_CURRENT_SOURCE_DIR}/rtos_task/watchdog_task.c
        ${CMAKE_CURRENT_SOURCE_DIR}/rtos_task/mdi_debug_task.c
        ${CMAKE_CURRENT_SOURCE_DIR}/services/crypto_rot.c
        ${CMAKE_CURRENT_SOURCE_DIR}/services/test_events.c
        ${CMAKE_CURRENT_SOURCE_DIR}/services/firmware_update.c
        ${CMAKE_CURRENT_SOURCE_DIR}/services/firmware_loader.c
        ${CMAKE_CURRENT_SOURCE_DIR}/services/asset_track.c
        ${CMAKE_CURRENT_SOURCE_DIR}/services/asset_track_mgmt.c
        ${CMAKE_CURRENT_SOURCE_DIR}/services/thermal_power_monitor.c
        ${CMAKE_CURRENT_SOURCE_DIR}/services/performance.c
        ${CMAKE_CURRENT_SOURCE_DIR}/services/link_mgmt.c
        ${CMAKE_CURRENT_SOURCE_DIR}/services/error_control.c
        ${CMAKE_CURRENT_SOURCE_DIR}/services/historical_extreme.c
        ${CMAKE_CURRENT_SOURCE_DIR}/services/dm_events.c
        ${CMAKE_CURRENT_SOURCE_DIR}/services/perf_mgmt.c
        ${CMAKE_CURRENT_SOURCE_DIR}/services/thermal_pwr_mgmt.c
        ${CMAKE_CURRENT_SOURCE_DIR}/services/trace.c
        ${CMAKE_CURRENT_SOURCE_DIR}/services/log.c
        ${CMAKE_CURRENT_SOURCE_DIR}/services/bl2_exception.c
        ${CMAKE_CURRENT_SOURCE_DIR}/services/bl2_scratch_buffer.c
        ${CMAKE_CURRENT_SOURCE_DIR}/services/minion_debug.c
    )
    target_include_directories(${TARGET_NAME}.elf
        PRIVATE
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
            $<BUILD_INTERFACE:${SHARED_INC_DIRS}>
            $<BUILD_INTERFACE:${FLASH_TOOL_HEADERS}>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/esperanto-fw>
    )
    target_compile_definitions(${TARGET_NAME}.elf
        PUBLIC
            -DBL2_VARIANT="${BL2_BUILD_VARIANT}"
            -DFAST_BOOT=${FAST_BOOT} -DTEST_FRAMEWORK=${TEST_FRAMEWORK} -DMINION_DEBUG_INTERFACE=${MINION_DEBUG_INTERFACE}
    )
    target_link_libraries(${TARGET_NAME}.elf
        PRIVATE
            deviceApi::deviceApi
            signedImageFormat::signedImageFormat
            etsoc_hal::etsoc_hal
            et-common-libs::sp-bl2
            et-common-libs::sp-bl1
            deviceFirmware::device_common
            FreeRTOS::FreeRTOS
            $<${TEST_FRAMEWORK}:test_framework>
            esperantoTrace::et_trace
    )

    target_report_riscv_size(${TARGET_NAME}.elf)
    target_report_riscv_elf_file_header(${TARGET_NAME}.elf)

    install(TARGETS ${TARGET_NAME}.elf
        EXPORT EsperantoBootLoaderTargets
        RUNTIME DESTINATION ${LIB_INSTALL_DIR}/esperanto-fw/ServiceProcessorBL2/${BL2_BUILD_VARIANT}
        COMPONENT bootloader
    )
    install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.lst
        ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.map
        ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.bin
        DESTINATION ${LIB_INSTALL_DIR}/esperanto-fw/ServiceProcessorBL2/${BL2_BUILD_VARIANT}
        COMPONENT service-processor
    )
endforeach()

# Exposing BL2_driver for e.g. Test Framework
#   This interface is universal between variants for now
add_library(BL2_driver INTERFACE)
target_include_directories(BL2_driver
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(BL2_driver
    INTERFACE
        deviceApi::deviceApi
        signedImageFormat::signedImageFormat
        etsoc_hal::etsoc_hal
        et-common-libs::sp-bl2
        deviceFirmware::device_common
        FreeRTOS::FreeRTOS
        esperantoTrace::et_trace
)
