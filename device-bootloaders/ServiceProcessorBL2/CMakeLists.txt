#------------------------------------------------------------------------------
# Copyright (C) 2019, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

set(FILE_VERSION_MAJOR 0)
set(FILE_VERSION_MINOR 0)
set(FILE_REVISION_NUMBER 1)

set(BL2_INSTALL_DIR ${LIB_INSTALL_DIR}/esperanto-fw/ServiceProcessorBL2)

set(BL2_SOURCES
    src/bl2_entry.S
    src/crt.S
    src/dummy_isr.c
    src/interrupt.c
    src/main.c
    src/mailbox.c
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
    src/bl2_ddr_init.c
    src/bl2_minion_pll_and_dll.c
    src/bl2_timer.c
    src/crc32.c
    src/memory_copy_8x32.S
    # This file get compiled different based on the serial_device.h configuration of each target
    src/bl2_build_configuration.c)

list(TRANSFORM BL2_SOURCES PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/)

set(SOURCES
    ${BL2_SOURCES}
    ${FREERTOS_SRC_FILES}
    ${SHARED_SRC_DIR}/pcie_init.c
    ${SHARED_SRC_DIR}/serial.c
    ${SHARED_SRC_DIR}/sp_otp.c
)

set(LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/src/sections.ld)
set(ZEBU_TARGET SP_RAM)

include_directories(include)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/include)
include_directories(${SHARED_INC_DIR})
include_directories(${FREERTOS_INC_DIRS})
include_directories(${FLASH_TOOL_HEADERS})
include_directories(${PKCS11_HEADERS})
include_directories(${CRYPTO_HEADERS})
include_directories(${ESPERANTO_DEVICE_API_INCLUDE_DIR})

add_subdirectory(include)

# BL2 build variants
add_subdirectory(BL2)
