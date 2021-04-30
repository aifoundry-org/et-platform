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

include_directories(${SHARED_INC_DIRS})

if(EsperantoCryptoAPI_FOUND)
  include_directories(${CRYPTO_HEADERS})
else()
  include_directories(${CRYPTO_HEADERS}/esperanto/crypto-api)
endif(EsperantoCryptoAPI_FOUND)

add_riscv_executable(MachineMinion
    src/boot.S
    src/trap_handler.S
    src/syscall.c
    src/trap_routine.c
    src/main.c
    src/pmu.c
    src/minion_cfg.c
    src/build_configuration.c
    ${SHARED_SRC_DIR}/serial.c
    ${SHARED_SRC_DIR}/print_exception.c
    ${SHARED_SRC_DIR}/broadcast.c
)
target_include_directories(MachineMinion.elf
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

install(TARGETS MachineMinion.elf
    EXPORT EsperantoDeviceMinionRuntimeTargets
    RUNTIME DESTINATION ${LIB_INSTALL_DIR}/esperanto-fw/MachineMinion
    COMPONENT machine-minion
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/MachineMinion.lst
              ${CMAKE_CURRENT_BINARY_DIR}/MachineMinion.map
    DESTINATION ${LIB_INSTALL_DIR}/esperanto-fw/MachineMinion
    COMPONENT machine-minion
)
