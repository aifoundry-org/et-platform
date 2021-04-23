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
    src/bl1_vaultip_controller.c
    ${SHARED_SRC_DIR}/constant_memory_compare.c
    ${SHARED_SRC_DIR}/serial.c
    ${SHARED_SRC_DIR}/sp_otp.c
    # This file get compiled different based on the serial_device.h configuration of each target
    src/bl1_build_configuration.c)

set(TARGET_NAME ServiceProcessorBL1)
set(LINKER_SCRIPT src/bl1_sections.ld)

include_directories(include)
include_directories(${SHARED_INC_DIRS})
include_directories(${FLASH_TOOL_HEADERS})

add_subdirectory(include)

add_riscv_executable(${TARGET_NAME} ${SOURCES})
target_link_libraries(${TARGET_NAME}.elf PRIVATE signedImageFormat::signedImageFormat)

install(TARGETS ${TARGET_NAME}.elf
  EXPORT EsperantoBootLoaderTargets
  RUNTIME DESTINATION ${LIB_INSTALL_DIR}/esperanto-fw/ServiceProcessorBL1
  COMPONENT bootloader)

install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.lst
    ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.map
    ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.bin
  DESTINATION
    ${LIB_INSTALL_DIR}/esperanto-fw/ServiceProcessorBL1
  COMPONENT service-processor
  )
