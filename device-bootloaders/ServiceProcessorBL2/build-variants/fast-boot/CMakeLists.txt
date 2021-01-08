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

add_riscv_executable(${TARGET_NAME} ${SOURCES})

target_compile_definitions(${TARGET_NAME}.elf PUBLIC -DBL2_VARIANT="fast-boot")
target_compile_definitions(${TARGET_NAME}.elf PUBLIC -DFAST_BOOT=1)
target_compile_definitions(${TARGET_NAME}.elf PUBLIC -DSERVICE_PROCESSOR_BL2=1)

target_link_libraries(${TARGET_NAME}.elf
  EsperantoDeviceMinionRuntime::firmware_helpers
  )

install(TARGETS ${TARGET_NAME}.elf
  EXPORT EsperantoBootLoaderTargets
  RUNTIME DESTINATION ${BL2_INSTALL_DIR}/fast-boot
  COMPONENT bootloader)

install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.lst
    ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.map
    ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.bin
  DESTINATION
    ${BL2_INSTALL_DIR}/fast-boot
  COMPONENT service-processor
  )

# Refactored code: IMPLEMENTATION_BYPASS switch

add_riscv_executable(${TARGET_NAME}New ${SOURCES})

target_compile_definitions(${TARGET_NAME}New.elf PUBLIC -DBL2_VARIANT="fast-bootNew")
target_compile_definitions(${TARGET_NAME}New.elf PUBLIC -DFAST_BOOT=1)
target_compile_definitions(${TARGET_NAME}New.elf PUBLIC -DIMPLEMENTATION_BYPASS=1)
target_compile_definitions(${TARGET_NAME}New.elf PUBLIC -DSERVICE_PROCESSOR_BL2=1)

target_link_libraries(${TARGET_NAME}New.elf
  EsperantoDeviceMinionRuntime::firmware_helpers
  )

install(TARGETS ${TARGET_NAME}New.elf
  EXPORT EsperantoBootLoaderTargets
  RUNTIME DESTINATION ${BL2_INSTALL_DIR}/fast-boot
  COMPONENT bootloader)

install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}New.lst
    ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}New.map
    ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}New.bin
  DESTINATION
    ${BL2_INSTALL_DIR}/fast-boot
  COMPONENT service-processor
  )
