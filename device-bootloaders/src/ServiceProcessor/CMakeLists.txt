# TODO FIXME move shared handlers out of project specific dir
set(SOURCES src/boot.S)

set(LINKER_SCRIPT src/sections.ld)
set(ZEBU_TARGET ROM) # change to SP_RAM eventually

include_directories(include)

add_riscv_executable("ServiceProcessor" ${LINKER_SCRIPT} ${ZEBU_TARGET} ${SOURCES})
