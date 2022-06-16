set(FILE_VERSION_MAJOR      ${PROJECT_VERSION_MAJOR})
set(FILE_VERSION_MINOR      ${PROJECT_VERSION_MINOR})
set(FILE_REVISION_NUMBER    ${PROJECT_VERSION_PATCH})

set(TARGET_NAME ServiceProcessorBL1)
set(LINKER_SCRIPT src/bl1_sections.ld)
set(LINKER_SCRIPT_DEPENDENCY ${SHARED_INC_DIR}/bl_layout.ld)
set(TEST_FRAMEWORK 0)

add_subdirectory(include)

add_riscv_executable(ServiceProcessorBL1
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
)
target_include_directories(ServiceProcessorBL1.elf
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/esperanto-fw>
)
target_link_libraries(ServiceProcessorBL1.elf
    PRIVATE
        signedImageFormat::signedImageFormat
        etsoc_hal::etsoc_hal
        esperantoBootLoader::common
        et-common-libs::sp-bl1
        $<${TEST_FRAMEWORK}:test_framework_core>
)
target_compile_definitions(ServiceProcessorBL1.elf
    PUBLIC
        -DTEST_FRAMEWORK=${TEST_FRAMEWORK}
)

target_report_riscv_size(ServiceProcessorBL1.elf)
target_report_riscv_elf_file_header(ServiceProcessorBL1.elf)

install(TARGETS ServiceProcessorBL1.elf
    EXPORT EsperantoBootLoaderTargets
    RUNTIME DESTINATION ${LIB_INSTALL_DIR}/esperanto-fw/ServiceProcessorBL1
    COMPONENT bootloader
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ServiceProcessorBL1.lst
              ${CMAKE_CURRENT_BINARY_DIR}/ServiceProcessorBL1.map
              ${CMAKE_CURRENT_BINARY_DIR}/ServiceProcessorBL1.bin
    DESTINATION ${LIB_INSTALL_DIR}/esperanto-fw/ServiceProcessorBL1
    COMPONENT service-processor
)
