# TODO FIXME move shared handlers out of project specific dir
set(SOURCES src/boot.S src/crt.S src/main.c src/trap_handler.c src/trap_handler.S)

set(TARGET_NAME ServiceProcessor)
set(LINKER_SCRIPT src/sections.ld)
set(ZEBU_TARGET ROM) # change to SP_RAM eventually

include_directories(include)

add_riscv_executable(${TARGET_NAME} ${LINKER_SCRIPT} ${ZEBU_TARGET} ${SOURCES})
