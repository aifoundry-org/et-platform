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

add_definitions(-DBL2_VARIANT="production")
add_definitions(-DFAST_BOOT=0 -DBRINGUP_TEST=0)

add_riscv_executable(${TARGET_NAME} ${SOURCES})

target_link_libraries(${TARGET_NAME}.elf PRIVATE
    EsperantoDeviceMinionRuntime::sp_firmware_helpers
    signedImageFormat::signedImageFormat
)

install(TARGETS ${TARGET_NAME}.elf
  EXPORT EsperantoBootLoaderTargets
  RUNTIME DESTINATION ${BL2_INSTALL_DIR}/production
  COMPONENT bootloader)

install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.lst
    ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.map
    ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.bin
  DESTINATION
    ${BL2_INSTALL_DIR}/production
  COMPONENT service-processor
  )
