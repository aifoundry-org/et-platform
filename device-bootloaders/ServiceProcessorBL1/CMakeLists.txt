set (FILE_VERSION_MAJOR 0)
set (FILE_VERSION_MINOR 0)
set (FILE_REVISION_NUMBER 1)

set(SOURCES
    src/bl1_entry.S
    src/bl1_init_handler.S
    src/bl1_main.c
    src/printx.c
    src/bl1_trap_handler.c
    src/bl1_trap_handler.S
    src/bl1_spi_controller.c
    src/bl1_spi_flash.c
    src/bl1_flash_fs.c
    src/crc32.c
    src/memory_copy_8x32.S
    src/bl1_timer.c
    src/bl1_crypto.c
    src/bl1_sp_certificates.c
    src/bl1_sp_firmware_loader.c
    src/bl1_sp_otp.c
    src/bl1_vaultip_controller.c
    # This file get compiled different based on the serial_device.h configuration of each target
    ${SHARED_SRC_DIR}/serial.c
    src/bl1_build_configuration.c)

if (BUILD_STAND_ALONE)

else()

  list(APPEND SOURCES
    ${SHARED_SRC_DIR}/constant_memory_compare.c
    )

endif(BUILD_STAND_ALONE)

set(TARGET_NAME ServiceProcessorBL1)
set(LINKER_SCRIPT src/bl1_sections.ld)
set(ZEBU_TARGET SP_RAM)
set(ZEBU_FILENAME ServiceProcessorBL1)

include_directories(include)
include_directories(${SHARED_INC_DIRS})
include_directories(${FLASH_TOOL_HEADERS})

if(EsperantoCryptoAPI_FOUND)
  include_directories(${CRYPTO_HEADERS})
else()
  include_directories(${CRYPTO_HEADERS}/esperanto/crypto-api)
endif(EsperantoCryptoAPI_FOUND)

add_riscv_executable(${TARGET_NAME} ${SOURCES})

list(APPEND FLASH_IMAGE_DEPS ${TARGET_NAME}.elf)
set(FLASH_IMAGE_DEPS ${FLASH_IMAGE_DEPS} CACHE INTERNAL "FLASH_IMAGE_DEPS")

if (BUILD_STAND_ALONE)
  target_link_libraries(${TARGET_NAME}.elf ${MINION_RUNTIME_PACKAGE_NAME}::bootrom_helpers)
  set(EXPORT_NAME EsperantoBootLoaderTargets)
else()
  set(EXPORT_NAME esperanto-dev-fw-targets)
endif()

install(TARGETS ${TARGET_NAME}.elf
  EXPORT ${EXPORT_NAME}
  RUNTIME DESTINATION ${LIB_INSTALL_DIR}/esperanto-fw/ServiceProcessor
  COMPONENT bootloader)

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
