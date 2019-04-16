#------------------------------------------------------------------------------
# Copyright (C) 2019, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

set(SOURCES
    ${SHARED_SRC_DIR}/boot.S
    src/trap_handler.S
    src/crt.S
    src/syscall.c
    src/swi.c
    src/exception.c
    src/main.c
    ${SHARED_SRC_DIR}/serial.c
    )

set(TARGET_NAME MachineMinion)
set(LINKER_SCRIPT src/sections.ld)
set(ZEBU_TARGET DDR)
set(ZEBU_FILENAME memImage)

include_directories(include)
include_directories(${SHARED_INC_DIR})

add_riscv_executable(${TARGET_NAME} ${LINKER_SCRIPT} ${ZEBU_TARGET} ${ZEBU_FILENAME} ${SOURCES})
