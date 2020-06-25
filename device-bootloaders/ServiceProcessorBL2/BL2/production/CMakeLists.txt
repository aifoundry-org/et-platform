#------------------------------------------------------------------------------
# Copyright (C) 2020, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

set(TARGET_NAME ServiceProcessorBL2_production)
set(ZEBU_FILENAME ServiceProcessorBL2_production)
set(INSTALL_DIR ${BL2_INSTALL_DIR}/BL2/production)

add_definitions(-DBL2_VARIANT="production")
add_definitions(-DFAST_BOOT=0)

add_riscv_executable(${TARGET_NAME} ${SOURCES})

list(APPEND FLASH_IMAGE_DEPS ${TARGET_NAME}.elf)
set(FLASH_IMAGE_DEPS ${FLASH_IMAGE_DEPS} CACHE INTERNAL "FLASH_IMAGE_DEPS")

target_link_libraries(${TARGET_NAME}.elf
  ${MINION_RUNTIME_PACKAGE_NAME}::bootrom_helpers
  EsperantoServiceProcessorROM::sp_rom_helpers
  )

install(TARGETS ${TARGET_NAME}.elf
  EXPORT ${BOOTLOADERS_EXPORT_NAME}
  RUNTIME DESTINATION ${INSTALL_DIR}
  COMPONENT bootloader)

install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.lst
    ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.map
    ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.bin
  DESTINATION
    ${INSTALL_DIR}
  COMPONENT service-processor
  )

string(REPLACE " " ";" OUTPUT_LIST ${${TARGET_NAME}_OUTPUT})
foreach(FILE ${OUTPUT_LIST})
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${FILE}
    DESTINATION ${INSTALL_DIR}
    COMPONENT zebu)
endforeach(FILE)
