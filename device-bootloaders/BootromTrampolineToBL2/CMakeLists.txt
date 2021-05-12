#------------------------------------------------------------------------------
# Copyright (C) 2020, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

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
