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

# Add the device_api submodule as an external project. The code-generator code needs to always
# compile for the host. CMake does not support having different targets use
# different toolchains in the same build. For simplicity submodules that are self
# contained build as external projects, otherwise a two-step process would be needded.
include(ExternalProject)

#ExternalProject_Add(device-api
#    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/device_api
#    BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/device_api
#    CMAKE_ARGS parse-device-api)

set(SOURCES
    src/crt.S
    src/trap_handler.S
    src/syscall.c
    src/interrupt.c
    src/swi.c
    src/exception.c
    src/main.c
    src/putchar.c
    src/build_configuration.c
    src/mailbox.c
    src/pcie_isr.c
    ${SHARED_SRC_DIR}/serial.c
    ${SHARED_SRC_DIR}/printf.c
    ${SHARED_SRC_DIR}/message.c
    ${SHARED_SRC_DIR}/print_exception.c
    ${SHARED_SRC_DIR}/ringbuffer.c
    ${SHARED_SRC_DIR}/pcie_int.c
    )

set(TARGET_NAME MasterMinion)
set(LINKER_SCRIPT src/sections.ld)
set(ZEBU_TARGET DDR_NEW)
set(ZEBU_FILENAME memImage)

include_directories(include)
include_directories(${SHARED_INC_DIRS})
include_directories(${FIRMWARE_TOOLS_DIR}/code-signing-tools/crypto_library/include)

add_riscv_executable(${TARGET_NAME} ${SOURCES})

install(TARGETS ${TARGET_NAME}.elf
  EXPORT esperanto-fw-targets
  RUNTIME DESTINATION ${BIN_INSTALL_DIR}/esperanto-fw/MasterMinion
  COMPONENT master-minion)

message("Master minion hex to install ${TARGET_NAME_OUTPUT}")

string(REPLACE " " ";" OUTPUT_LIST ${${TARGET_NAME}_OUTPUT})
foreach(FILE ${OUTPUT_LIST})
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${FILE}
    DESTINATION ${LIB_INSTALL_DIR}/esperanto-fw/MasterMinion
    COMPONENT zebu)
endforeach(FILE)
