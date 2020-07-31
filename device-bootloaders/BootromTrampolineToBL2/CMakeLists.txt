#------------------------------------------------------------------------------
# Copyright (C) 2020, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

set(SOURCES src/boot.S)

set(TARGET_NAME BootromTrampolineToBL2)
set(LINKER_SCRIPT src/sections.ld)
set(ZEBU_TARGET ROM)
set(ZEBU_FILENAME BootromTrampolineToBL2)

add_riscv_executable(${TARGET_NAME} ${SOURCES})

set(EXPORT_NAME EsperantoBootLoaderTargets)

list(APPEND FLASH_IMAGE_DEPS ${TARGET_NAME}.elf)
set(FLASH_IMAGE_DEPS ${FLASH_IMAGE_DEPS} CACHE INTERNAL "FLASH_IMAGE_DEPS")

install(TARGETS ${TARGET_NAME}.elf
  EXPORT ${EXPORT_NAME}
  RUNTIME DESTINATION ${LIB_INSTALL_DIR}/esperanto-fw/BootromTrampolineToBL2
  COMPONENT service-processor)

install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.lst
    ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.map
    ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.bin
  DESTINATION
    ${LIB_INSTALL_DIR}/esperanto-fw/BootromTrampolineToBL2
  COMPONENT service-processor
  )


string(REPLACE " " ";" OUTPUT_LIST ${${TARGET_NAME}_OUTPUT})
foreach(FILE ${OUTPUT_LIST})
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${FILE}
    DESTINATION ${LIB_INSTALL_DIR}/esperanto-fw/BootromTrampolineToBL2
    COMPONENT zebu)
endforeach(FILE)
