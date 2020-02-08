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
    src/bl2_vaultip_controller.c
    src/bl2_vaultip_driver.c
    src/bl2_crypto.c
    src/bl2_certificates.c
    src/bl2_firmware_loader.c
    src/bl2_reset.c
    src/bl2_sp_pll.c
    src/bl2_sp_otp.c
    src/bl2_ddr_config.c
    src/bl2_minion_pll_and_dll.c
    src/bl2_timer.c
    src/crc32.c
    src/memory_copy_8x32.S
    # This file get compiled different based on the serial_device.h configuration of each target
    ${SHARED_SRC_DIR}/serial.c
    src/bl2_build_configuration.c)

if (BUILD_STAND_ALONE)

else()

  list(APPEND SOURCES
    ${SHARED_SRC_DIR}/constant_memory_compare.c
    ${SHARED_SRC_DIR}/backtrace.c
    ${SHARED_SRC_DIR}/pcie.c
    )

endif(BUILD_STAND_ALONE)

set(TARGET_NAME ServiceProcessorBL2)
set(LINKER_SCRIPT src/sections.ld)
set(ZEBU_TARGET SP_RAM)
set(ZEBU_FILENAME ServiceProcessorBL2)

include_directories(include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../shared/include)
include_directories(${SHARED_INC_DIR})
include_directories(${SHARED_HAL_DIR})
include_directories(${FREERTOS_INC_DIRS})
include_directories(${FLASH_TOOL_HEADERS})
include_directories(${CRYPTO_HEADERS}/esperanto/crypto-api)
include_directories(${PKCS11_HEADERS})

add_riscv_executable(${TARGET_NAME} ${CMAKE_CURRENT_LIST_DIR} ${SOURCES})

list(APPEND FLASH_IMAGE_DEPS ${TARGET_NAME}.elf)
set(FLASH_IMAGE_DEPS ${FLASH_IMAGE_DEPS} CACHE INTERNAL "FLASH_IMAGE_DEPS")

if (BUILD_STAND_ALONE)
  target_link_libraries(${TARGET_NAME}.elf esperanto-fw::bootrom_helpers)
  set(EXPORT_NAME EsperantoBootLoaderTargets)
else()
  set(EXPORT_NAME esperanto-dev-fw-targets)
endif()

install(TARGETS ${TARGET_NAME}.elf
  EXPORT ${EXPORT_NAME}
  RUNTIME DESTINATION ${LIB_INSTALL_DIR}/esperanto-fw/ServiceProcessor
  COMPONENT bootloader)

string(REPLACE " " ";" OUTPUT_LIST ${${TARGET_NAME}_OUTPUT})
foreach(FILE ${OUTPUT_LIST})
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${FILE}
    DESTINATION ${LIB_INSTALL_DIR}/esperanto-fw/ServiceProcessor
    COMPONENT zebu)
endforeach(FILE)
