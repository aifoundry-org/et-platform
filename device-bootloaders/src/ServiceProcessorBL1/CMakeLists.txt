set (FILE_VERSION_MAJOR 0)
set (FILE_VERSION_MINOR 0)
set (FILE_REVISION_NUMBER 1)

set(SOURCES
    src/bl1_entry.S
    src/bl1_init_handler.S
    src/bl1_main.c
    src/printx.c
    ${SHARED_SRC_DIR}/serial.c
    src/bl1_trap_handler.c
    src/bl1_trap_handler.S
    src/bl1_spi_controller.c
    src/bl1_spi_flash.c
    src/bl1_flash_fs.c
    src/crc32.c
    ${SHARED_SRC_DIR}/constant_memory_compare.c
    src/memory_copy_8x32.S
    src/bl1_timer.c
    src/bl1_crypto.c
    src/bl1_sp_certificates.c
    src/bl1_sp_firmware_loader.c
    src/bl1_sp_otp.c
    src/bl1_vaultip_controller.c
    src/bl1_build_configuration.c)

set(TARGET_NAME ServiceProcessorBL1)
set(LINKER_SCRIPT src/bl1_sections.ld)
set(ZEBU_TARGET SP_RAM)
set(ZEBU_FILENAME ServiceProcessorBL1)

include_directories(include)
include_directories(${SHARED_INC_DIRS})
include_directories(${FIRMWARE_TOOLS_DIR}/esperanto-flash-tool/include)
include_directories(${FIRMWARE_TOOLS_DIR}/code-signing-tools/crypto_library/include)
include_directories(${FIRMWARE_TOOLS_DIR}/code-signing-tools/pkcs11/published/2-40-errata-1)

add_riscv_executable(${TARGET_NAME} ${SOURCES})

list(APPEND FLASH_IMAGE_DEPS ${TARGET_NAME}.elf)
set(FLASH_IMAGE_DEPS ${FLASH_IMAGE_DEPS} CACHE INTERNAL "FLASH_IMAGE_DEPS")

install(TARGETS ${TARGET_NAME}.elf
  EXPORT esperanto-dev-fw-targets
  RUNTIME DESTINATION ${LIB_INSTALL_DIR}/esperanto-fw/ServiceProcessor
  COMPONENT service-processor)

string(REPLACE " " ";" OUTPUT_LIST ${${TARGET_NAME}_OUTPUT})
foreach(FILE ${OUTPUT_LIST})
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${FILE}
    DESTINATION ${LIB_INSTALL_DIR}/esperanto-fw/ServiceProcessor
    COMPONENT zebu)
endforeach(FILE)
