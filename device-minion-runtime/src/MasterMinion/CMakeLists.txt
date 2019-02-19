set(SOURCES ${SHARED_SRC_DIR}/boot.S src/crt.S src/main.c ${SHARED_SRC_DIR}/stubs.c ${SHARED_SRC_DIR}/serial.c src/fw_compute_code.c src/fw_master_code.c ${SHARED_SRC_DIR}/trap_handler.c ${SHARED_SRC_DIR}/trap_handler.S)

set(TARGET_NAME MasterMinion)
set(LINKER_SCRIPT src/sections.ld)
set(ZEBU_TARGET DRAM)

include_directories(include)
include_directories(${SHARED_INC_DIR})

add_riscv_executable(${TARGET_NAME} ${LINKER_SCRIPT} ${ZEBU_TARGET} ${SOURCES})
