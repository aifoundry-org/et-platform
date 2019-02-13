# TODO FIXME move shared code out of project specific dir
set(SOURCES src/boot.S src/crt.S src/main.c src/fw_compute_code.c src/fw_master_code.c src/trap_handler.c src/trap_handler.S)

set(TARGET_NAME MasterMinion)
set(LINKER_SCRIPT src/sections.ld)
set(ZEBU_TARGET DRAM)

include_directories(include)

add_riscv_executable(${TARGET_NAME} ${LINKER_SCRIPT} ${ZEBU_TARGET} ${SOURCES})
