# Copyright (c) 2025 Ainekko, Co.
# SPDX-License-Identifier: Apache-2.0

set(LINKER_SCRIPT src/sections.ld)

add_riscv_executable(BootromTrampolineToBL2 src/boot.S)

install(TARGETS BootromTrampolineToBL2.elf
    EXPORT EsperantoBootLoaderTargets
    RUNTIME DESTINATION ${LIB_INSTALL_DIR}/esperanto-fw/BootromTrampolineToBL2
    COMPONENT service-processor
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/BootromTrampolineToBL2.lst
    ${CMAKE_CURRENT_BINARY_DIR}/BootromTrampolineToBL2.map
    ${CMAKE_CURRENT_BINARY_DIR}/BootromTrampolineToBL2.bin
    DESTINATION ${LIB_INSTALL_DIR}/esperanto-fw/BootromTrampolineToBL2
    COMPONENT service-processor
)
