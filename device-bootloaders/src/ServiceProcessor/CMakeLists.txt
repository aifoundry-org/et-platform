set(SOURCES ${SHARED_SRC_DIR}/boot.S src/crt.S src/main.c ${SHARED_SRC_DIR}/stubs.c ${SHARED_SRC_DIR}/serial.c ${SHARED_SRC_DIR}/backtrace.c ${SHARED_SRC_DIR}/trap_handler.c ${SHARED_SRC_DIR}/trap_handler.S)

set(TARGET_NAME ServiceProcessor)
set(LINKER_SCRIPT src/sections.ld)
set(ZEBU_TARGET ROM) # change to SP_RAM eventually

include_directories(include)
include_directories(${SHARED_INC_DIR})

add_riscv_executable(${TARGET_NAME} ${LINKER_SCRIPT} ${ZEBU_TARGET} ${SOURCES})
