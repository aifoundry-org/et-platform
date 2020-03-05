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
    src/boot.S
    src/crt.S
    src/dummy_isr.c
    src/interrupt.c
    src/main.c
    src/stubs.c
    src/mailbox.c
    ${SHARED_SRC_DIR}/serial.c
    )

set(TARGET_NAME ServiceProcessor)
set(LINKER_SCRIPT src/sections.ld)
set(ZEBU_TARGET ROM) # change to SP_RAM eventually
set(ZEBU_FILENAME romImage)

include_directories(include)
include_directories(${SHARED_INC_DIRS})
include_directories(${FREERTOS_INC_DIRS})

add_riscv_executable(${TARGET_NAME} ${SOURCES})

target_link_libraries(${TARGET_NAME}.elf ${MINION_RUNTIME_PACKAGE_NAME}::bootrom_helpers)
set(EXPORT_NAME EsperantoBootLoaderTargets)

list(APPEND FLASH_IMAGE_DEPS ${TARGET_NAME}.elf)
set(FLASH_IMAGE_DEPS ${FLASH_IMAGE_DEPS} CACHE INTERNAL "FLASH_IMAGE_DEPS")

install(TARGETS ${TARGET_NAME}.elf
  EXPORT ${EXPORT_NAME}
  RUNTIME DESTINATION ${LIB_INSTALL_DIR}/esperanto-fw/ServiceProcessor
  COMPONENT service-processor)

install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.lst
    ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.bin
  DESTINATION
    ${LIB_INSTALL_DIR}/esperanto-fw/ServiceProcessor
  COMPONENT service-processor
  )


string(REPLACE " " ";" OUTPUT_LIST ${${TARGET_NAME}_OUTPUT})
foreach(FILE ${OUTPUT_LIST})
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${FILE}
    DESTINATION ${LIB_INSTALL_DIR}/esperanto-fw/ServiceProcessor
    COMPONENT zebu)
endforeach(FILE)
