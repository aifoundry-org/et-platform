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
    src/crt.S
    src/trap_handler.S
    src/syscall.c
    src/swi.c
    src/exception.c
    src/main.c
    src/kernel.c
    src/build_configuration.c
    src/log.c
    ${SHARED_SRC_DIR}/message.c
    ${SHARED_SRC_DIR}/printf.c
    ${SHARED_SRC_DIR}/kernel_return.c
    )

set(TARGET_NAME WorkerMinion)
set(LINKER_SCRIPT src/sections.ld)
set(LINKER_SCRIPT_DEPENDENCY ${SHARED_INC_DIR}/layout.ld)

include_directories(include)
include_directories(${SHARED_INC_DIRS})
include_directories(${ESPERANTO_DEVICE_API_INCLUDE_DIR})

if(EsperantoCryptoAPI_FOUND)
  include_directories(${CRYPTO_HEADERS})
else()
  include_directories(${CRYPTO_HEADERS}/esperanto/crypto-api)
endif(EsperantoCryptoAPI_FOUND)

add_riscv_executable(${TARGET_NAME} ${SOURCES})

install(TARGETS ${TARGET_NAME}.elf
  EXPORT EsperantoDeviceMinionRuntimeTargets
  RUNTIME DESTINATION ${LIB_INSTALL_DIR}/esperanto-fw/WorkerMinion
  COMPONENT worker-minion)

install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.lst
        ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.map
        ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.bin
  DESTINATION ${LIB_INSTALL_DIR}/esperanto-fw/WorkerMinion
  COMPONENT worker-minion)
