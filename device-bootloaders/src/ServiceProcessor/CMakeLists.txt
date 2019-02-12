# TODO FIXME move shared handlers out of project specific dir
set(SOURCES src/boot.S src/crt.S src/main.c src/trap_handler.c src/trap_handler.S)

set(TARGET_NAME ServiceProcessor)
set(LINKER_SCRIPT src/sections.ld)
set(ZEBU_TARGET ROM) # change to SP_RAM eventually

include_directories(include)

# Disable floating point and compressed instructions
# -msave-restore might save some space in ROM
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=rv64imaf -mabi=lp64f")

message("CMAKE_C_FLAGS = ${CMAKE_C_FLAGS}")

add_riscv_executable(${TARGET_NAME} ${LINKER_SCRIPT} ${ZEBU_TARGET} ${SOURCES})
