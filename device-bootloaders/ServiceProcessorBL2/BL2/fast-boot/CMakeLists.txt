#------------------------------------------------------------------------------
# Copyright (C) 2020, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

set(TARGET_NAME ServiceProcessorBL2_fast-boot)
set(ZEBU_FILENAME ServiceProcessorBL2_fast-boot)
set(INSTALL_DIR ${BL2_INSTALL_DIR}/BL2/fast-boot)

add_definitions(-DBL2_VARIANT="fast-boot")
add_definitions(-DFAST_BOOT=1)

configure_file(gen_sp_sram_image.sh.in gen_sp_sram_image.sh @ONLY)

# TODO: Remove this and use elftohex.py when it properly generates Zebu SP-SRAM images
set(SP_SRAM_HEX_FILES
  ${CMAKE_CURRENT_BINARY_DIR}/sp-sram_0.hex
  ${CMAKE_CURRENT_BINARY_DIR}/sp-sram_1.hex
  ${CMAKE_CURRENT_BINARY_DIR}/sp-sram_2.hex
  ${CMAKE_CURRENT_BINARY_DIR}/sp-sram_3.hex
  ${CMAKE_CURRENT_BINARY_DIR}/sp-sram_4.hex
  ${CMAKE_CURRENT_BINARY_DIR}/sp-sram_5.hex
  ${CMAKE_CURRENT_BINARY_DIR}/sp-sram_6.hex
  ${CMAKE_CURRENT_BINARY_DIR}/sp-sram_7.hex
  ${CMAKE_CURRENT_BINARY_DIR}/sp-sram_8.hex
  ${CMAKE_CURRENT_BINARY_DIR}/sp-sram_9.hex
  ${CMAKE_CURRENT_BINARY_DIR}/sp-sram_A.hex
  ${CMAKE_CURRENT_BINARY_DIR}/sp-sram_B.hex
  ${CMAKE_CURRENT_BINARY_DIR}/sp-sram_C.hex
  ${CMAKE_CURRENT_BINARY_DIR}/sp-sram_D.hex
  ${CMAKE_CURRENT_BINARY_DIR}/sp-sram_E.hex
  ${CMAKE_CURRENT_BINARY_DIR}/sp-sram_F.hex
  )

add_riscv_executable(${TARGET_NAME} ${SOURCES})

add_custom_target(${TARGET_NAME}-sp-sram-image ALL
  DEPENDS ${SP_SRAM_HEX_FILES}
  )

add_custom_command(
  OUTPUT ${SP_SRAM_HEX_FILES}
  COMMAND ${CMAKE_CURRENT_BINARY_DIR}/gen_sp_sram_image.sh
  DEPENDS ${TARGET_NAME}.bin
          ${CMAKE_CURRENT_BINARY_DIR}/gen_sp_sram_image.sh
  )

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

install(
  FILES ${SP_SRAM_HEX_FILES}
  DESTINATION ${INSTALL_DIR}
  COMPONENT zebu
  )
