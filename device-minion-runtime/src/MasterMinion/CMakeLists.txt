# TODO FIXME move shared code out of project specific dir
set(SOURCES src/main.c src/trap_handler.c src/boot.S src/trap_handler.S)

set(LINKER_SCRIPT src/sections.ld)
set(ZEBU_TARGET DRAM)

include_directories(include)

add_riscv_executable("MasterMinion" ${LINKER_SCRIPT} ${ZEBU_TARGET} ${SOURCES})
