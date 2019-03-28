set(SOURCES 
    src/reset_handler.S
    src/init_handler.S
    src/main.c
    src/printx.c
    ${SHARED_SRC_DIR}/serial.c
    src/trap_handler.c
    src/trap_handler.S
    src/spi_controller.c
    src/spi_flash.c
    src/flash_fs.c)

set(TARGET_NAME ServiceProcessorBL1)
set(LINKER_SCRIPT src/sections.ld)
set(ZEBU_TARGET ROM) # change to SP_RAM eventually
set(ZEBU_FILENAME ServiceProcessorBL1)

include_directories(include)
include_directories(${SHARED_INC_DIR})

add_riscv_executable(${TARGET_NAME} ${LINKER_SCRIPT} ${ZEBU_TARGET} ${ZEBU_FILENAME} ${SOURCES})

