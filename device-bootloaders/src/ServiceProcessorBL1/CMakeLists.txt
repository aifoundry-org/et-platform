set(SOURCES 
    src/bl1_entry.S
    src/bl1_init_handler.S
    src/bl1_main.c
    src/bl1_printx.c
    ${SHARED_SRC_DIR}/serial.c
    src/bl1_trap_handler.c
    src/bl1_trap_handler.S)

set(TARGET_NAME ServiceProcessorBL1)
set(LINKER_SCRIPT src/bl1_sections.ld)
set(ZEBU_TARGET ROM) # change to SP_RAM eventually
set(ZEBU_FILENAME ServiceProcessorBL1)

include_directories(include)
include_directories(${SHARED_INC_DIR})
include_directories(${FIRMWARE_TOOLS_DIR}/esperanto-flash-tool/include)
include_directories(${FIRMWARE_TOOLS_DIR}/code-signing-tools/crypto_library/include)
include_directories(${FIRMWARE_TOOLS_DIR}/code-signing-tools/pkcs11/published/2-40-errata-1)

add_riscv_executable(${TARGET_NAME} ${LINKER_SCRIPT} ${ZEBU_TARGET} ${ZEBU_FILENAME} ${SOURCES})

