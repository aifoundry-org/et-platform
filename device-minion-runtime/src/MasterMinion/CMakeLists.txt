# TODO FIXME move shared code out of project specific dir
set(SOURCES src/boot.S src/crt.S src/main.c src/fw_compute_code.c src/fw_master_code.c src/trap_handler.c src/trap_handler.S)

set(TARGET_NAME MasterMinion)
set(LINKER_SCRIPT src/sections.ld)
set(ZEBU_TARGET DRAM)

include_directories(include)

# Disable compressed instructions
# TODO FIXME disable floating point if SP doesn't support the F extension, but
# need better multilib support in toolchain to build soft-float ABI
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=rv64imaf -mabi=lp64f")

add_riscv_executable(${TARGET_NAME} ${LINKER_SCRIPT} ${ZEBU_TARGET} ${SOURCES})
