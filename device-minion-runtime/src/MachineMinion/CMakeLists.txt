#------------------------------------------------------------------------------
# Copyright (C) 2019, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

set (FILE_VERSION_MAJOR 0)
set (FILE_VERSION_MINOR 0)
set (FILE_REVISION_NUMBER 1)

set(SOURCES
    ${SHARED_SRC_DIR}/boot.S
    src/trap_handler.S
    src/crt.S
    src/syscall.c
    src/swi.c
    src/exception.c
    src/main.c
    src/putchar.c
    ${SHARED_SRC_DIR}/message.c
    ${SHARED_SRC_DIR}/serial.c
    ${SHARED_SRC_DIR}/printf.c
    ${SHARED_SRC_DIR}/print_exception.c
    )

set(TARGET_NAME MachineMinion)
set(LINKER_SCRIPT src/sections.ld)
set(ZEBU_TARGET DDR_NEW)
set(ZEBU_FILENAME memImage)

include_directories(include)
include_directories(${SHARED_INC_DIR})

add_riscv_executable(${TARGET_NAME} ${LINKER_SCRIPT} ${ZEBU_TARGET} ${ZEBU_FILENAME} ${SOURCES})

install(TARGETS ${TARGET_NAME}.elf
  DESTINATION ${CMAKE_INSTALL_PREFIX}/et-firmware/MachineMinion
  COMPONENT machine-minion)

string(REPLACE " " ";" OUTPUT_LIST ${${TARGET_NAME}_OUTPUT})
foreach(FILE ${OUTPUT_LIST})
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${FILE}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/et-firmware/MachineMinion
    COMPONENT zebu)
endforeach(FILE)
