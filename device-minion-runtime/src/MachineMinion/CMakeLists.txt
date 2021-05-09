#------------------------------------------------------------------------------
# Copyright (C) 2019, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

set(LINKER_SCRIPT src/sections.ld)
set(LINKER_SCRIPT_DEPENDENCY ${SHARED_INC_DIR}/layout.ld)

add_riscv_executable(MachineMinion
    src/boot.S
    src/trap_handler.S
    src/syscall.c
    src/trap_routine.c
    src/main.c
    src/pmu.c
    src/minion_cfg.c
    src/build_configuration.c
    ${SHARED_SRC_DIR}/print_exception.c
    ${SHARED_SRC_DIR}/broadcast.c
)
target_include_directories(MachineMinion.elf
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE 
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/src/shared/include>
)
target_link_libraries(MachineMinion.elf
    PRIVATE
        deviceFirmware::firmware_helpers_interface
        deviceFirmware::firmware_helpers_serial
    PRIVATE
        deviceFirmware::device_configuration
)

install(TARGETS MachineMinion.elf
    EXPORT EsperantoDeviceMinionRuntimeTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}/esperanto-fw/MachineMinion
    COMPONENT machine-minion
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/MachineMinion.lst
              ${CMAKE_CURRENT_BINARY_DIR}/MachineMinion.map
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/esperanto-fw/MachineMinion
    COMPONENT machine-minion
)
