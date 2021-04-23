#------------------------------------------------------------------------------
# Copyright (C) 2020, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

set(TARGET_NAME ServiceProcessorBL2_testframework)

add_riscv_executable(${TARGET_NAME} ${SOURCES})

target_compile_definitions(${TARGET_NAME}.elf PUBLIC -DBL2_VARIANT="testframework")
target_compile_definitions(${TARGET_NAME}.elf PUBLIC -DFAST_BOOT=1 -DBRINGUP_TEST=1)

target_link_libraries(${TARGET_NAME}.elf PRIVATE EsperantoDeviceMinionRuntime::sp_firmware_helpers)

install(TARGETS ${TARGET_NAME}.elf
  EXPORT EsperantoBootLoaderTargets
  RUNTIME DESTINATION ${BL2_INSTALL_DIR}/testframework
  COMPONENT bootloader)

install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.lst
    ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.map
    ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.bin
  DESTINATION
    ${BL2_INSTALL_DIR}/testframework
  COMPONENT service-processor
  )
