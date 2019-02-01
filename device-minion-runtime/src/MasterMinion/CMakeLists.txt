cmake_minimum_required(VERSION 3.5)

set(TARGET_NAME "MasterMinion")

# Must set CMAKE_TOOLCHAIN_FILE before project() is called
set(CMAKE_TOOLCHAIN_FILE Toolchain-gcc-riscv.cmake)

project(MasterMinion LANGUAGES C ASM)

set(CMAKE_VERBOSE_MAKEFILE ON)

# Locate hex output @ beginning of DRAM region 0x8000200000
set(CMAKE_OFFSET_ADDRESS 0x8000200000)

# TODO FIXME move shared code out of project specific dir
set(SOURCES src/main.c src/trap_handler.c src/boot.S src/trap_handler.S)

include_directories(include)

add_custom_executable(${TARGET_NAME} ${SOURCES})

# Get the absolute path to the linker script
get_filename_component(LINKER_SCRIPT "src/sections.ld" ABSOLUTE)

# Use custom linker script and generate a map file
set(CMAKE_EXE_LINKER_FLAGS "-nostdlib -lc -lm -lgcc -T ${LINKER_SCRIPT} -Xlinker -Map=${MAP_FILE}")

# Add explicit dependency on linker script when linking target
set_target_properties(${ELF_FILE} PROPERTIES LINK_DEPENDS ${LINKER_SCRIPT})