#------------------------------------------------------------------------------
# Copyright (C) 2019, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

# Add the device_api submodule as an external project. The code-generator code needs to always
# compile for the host. CMake does not support having different targets use
# different toolchains in the same build. For simplicity submodules that are self
# contained build as external projects, otherwise a two-step process would be needded.

set(LINKER_SCRIPT src/sections.ld)
set(LINKER_SCRIPT_DEPENDENCY ${SHARED_INC_DIR}/layout.ld)

option(ENABLE_CMD_EXECUTION_TRACE "Enable Command Tracing support" OFF)

# MM build variants
foreach(MM_BUILD_VARIANT IN ITEMS MasterMinion MasterMinionTF)

    add_riscv_executable(${MM_BUILD_VARIANT}
        src/crt.S
        src/trap_handler.S
        src/exception.c
        src/main.c
        src/build_configuration.c
        src/config/dir_regs.c
        src/dispatcher/dispatcher.c
        src/workers/spw.c
        src/workers/sqw.c
        src/workers/dmaw.c
        src/workers/kw.c
        src/workers/cw.c
        src/drivers/console.c
        src/drivers/pcie_dma.c
        src/drivers/plic.c
        src/drivers/pu_timers.c
        src/services/log.c
        src/services/host_iface.c
        src/services/host_cmd_hdlr.c
        src/services/cm_iface.c
        src/services/sp_iface.c
        src/services/sw_timer.c
        src/services/trace.c
        ${SHARED_SRC_DIR}/etsoc_memory.c
        ${SHARED_SRC_DIR}/printf.c
        ${SHARED_SRC_DIR}/putchar.c
        ${SHARED_SRC_DIR}/print_exception.c
        ${SHARED_SRC_DIR}/pcie_int.c
        ${SHARED_SRC_DIR}/broadcast.c
        ${SHARED_SRC_DIR}/circbuff.c
        ${SHARED_SRC_DIR}/vq.c
        ${SHARED_SRC_DIR}/sp_mm_iface.c
    )

    if (MM_BUILD_VARIANT STREQUAL "MasterMinionTF")
        # Test Framework version
        set(TEST_FRAMEWORK 1)
    else()
        # Default
        set(TEST_FRAMEWORK 0)
    endif()

    target_include_directories(${MM_BUILD_VARIANT}.elf
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        PRIVATE
            $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/src/shared/include>
    )
    target_compile_definitions(${MM_BUILD_VARIANT}.elf
        PUBLIC
            -DMASTER_MINION=1
            -DMM_VARIANT="${MM_BUILD_VARIANT}"
            -DTEST_FRAMEWORK=${TEST_FRAMEWORK}
        PRIVATE
            $<$<BOOL:${ENABLE_CMD_EXECUTION_TRACE}>: MM_ENABLE_CMD_EXECUTION_TRACE>

    )
    target_link_libraries(${MM_BUILD_VARIANT}.elf
        PUBLIC
            deviceFirmware::device_trace
            deviceApi::deviceApi
            etsoc_hal::etsoc_hal
            deviceFirmware::device_common
            deviceFirmware::firmware_helpers_serial
        PRIVATE
            deviceFirmware::device_configuration
    )

    install(TARGETS ${MM_BUILD_VARIANT}.elf
        EXPORT EsperantoDeviceMinionRuntimeTargets
        RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}/esperanto-fw/${MM_BUILD_VARIANT}
        COMPONENT master-minion
    )

    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${MM_BUILD_VARIANT}.lst
                ${CMAKE_CURRENT_BINARY_DIR}/${MM_BUILD_VARIANT}.map
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/esperanto-fw/${MM_BUILD_VARIANT}
        COMPONENT master-minion
    )
endforeach()
