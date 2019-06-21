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
    ${FREERTOS_SRC_FILES}
    src/bl2_entry.S
    src/crt.S
    src/dummy_isr.c
    src/interrupt.c
    src/main.c
    src/stubs.c
    src/bl2_spi_controller.c
    src/bl2_spi_flash.c
    src/bl2_flash_fs.c
    src/bl2_flashfs_driver.c
    ${SHARED_SRC_DIR}/serial.c
    ${SHARED_SRC_DIR}/backtrace.c
    src/bl2_build_configuration.c)

set(TARGET_NAME ServiceProcessorBL2)
set(LINKER_SCRIPT src/sections.ld)
set(ZEBU_TARGET SP_RAM)
set(ZEBU_FILENAME ServiceProcessorBL2)

include_directories(include)
include_directories(${SHARED_INC_DIRS})
include_directories(${FREERTOS_INC_DIRS})
include_directories(${FIRMWARE_TOOLS_DIR}/esperanto-flash-tool/include)
include_directories(${FIRMWARE_TOOLS_DIR}/code-signing-tools/crypto_library/include)
include_directories(${FIRMWARE_TOOLS_DIR}/code-signing-tools/pkcs11/published/2-40-errata-1)

add_riscv_executable(${TARGET_NAME} ${LINKER_SCRIPT} ${ZEBU_TARGET} ${ZEBU_FILENAME} ${CMAKE_CURRENT_LIST_DIR} ${SOURCES})

install(TARGETS ${TARGET_NAME}.elf
  DESTINATION ${CMAKE_INSTALL_PREFIX}/et-firmware/ServiceProcessor
  COMPONENT service-processor)

string(REPLACE " " ";" OUTPUT_LIST ${${TARGET_NAME}_OUTPUT})
foreach(FILE ${OUTPUT_LIST})
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${FILE}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/et-firmware/ServiceProcessor
    COMPONENT zebu)
endforeach(FILE)
