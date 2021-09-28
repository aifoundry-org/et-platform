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

add_riscv_executable(WorkerMinion
    src/crt.S
    src/trap_handler.S
    src/syscall.c
    src/swi.c
    src/exception.c
    src/main.c
    src/cm_to_mm_iface.c
    src/mm_to_cm_iface.c
    src/kernel.c
    src/kernel_return.c
    src/build_configuration.c
    src/log.c
    src/trace.c
)
target_include_directories(WorkerMinion.elf
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/src/shared/minion_rt_helpers/include>
)
target_compile_options(WorkerMinion.elf
    PRIVATE
        -Wall
        $<$<BOOL:${ENABLE_WARNINGS_AS_ERRORS}>:-Werror>
)
target_link_libraries(WorkerMinion.elf
    PUBLIC
        deviceApi::deviceApi
        et-common-libs::cm-rt-svcs
        esperantoTrace::et_trace
    PRIVATE
        deviceFirmware::minion_rt_helpers_interface
)

target_link_libraries(WorkerMinion.elf PUBLIC esperantoTrace::et_trace)

target_report_riscv_size(WorkerMinion.elf)
target_report_riscv_elf_file_header(WorkerMinion.elf)

install(TARGETS WorkerMinion.elf
    EXPORT EsperantoDeviceMinionRuntimeTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}/esperanto-fw/WorkerMinion
    COMPONENT worker-minion
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/WorkerMinion.lst
        ${CMAKE_CURRENT_BINARY_DIR}/WorkerMinion.map
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/esperanto-fw/WorkerMinion
    COMPONENT worker-minion
)
