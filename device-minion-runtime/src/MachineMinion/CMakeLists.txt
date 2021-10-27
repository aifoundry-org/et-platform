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
    src/minion_cfg.c
    src/build_configuration.c
)

#External libs header paths - etsoc_hal.lib, minion-bl.lib
target_include_directories(MachineMinion.elf
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/src/shared/minion_rt_helpers/include>
        ${CMAKE_INSTALL_PREFIX}/include/esperanto-fw/etsoc_hal # FIXME: THIS BREAKS ENCAPSULATION
        ${CMAKE_INSTALL_PREFIX}/minion-bl/include              # FIXME: THIS BREAKS ENCAPSULATION
)

target_compile_options(MachineMinion.elf
    PRIVATE
        -Wall
        $<$<BOOL:${ENABLE_WARNINGS_AS_ERRORS}>:-Werror>
)

target_link_libraries(MachineMinion.elf
    PRIVATE
        et-common-libs::minion-bl
        etsoc_hal::etsoc_hal
        deviceFirmware::minion_rt_helpers_interface
)

target_report_riscv_size(MachineMinion.elf)
target_report_riscv_elf_file_header(MachineMinion.elf)

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
