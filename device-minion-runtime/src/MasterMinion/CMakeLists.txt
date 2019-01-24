cmake_minimum_required(VERSION 3.5)

set(TARGET_NAME "MasterMinion")

# Get the absolute path to the linker script
get_filename_component(LINKER_SCRIPT "src/sections.ld" ABSOLUTE)

# Must set CMAKE_TOOLCHAIN_FILE before project() is called
set(CMAKE_TOOLCHAIN_FILE Toolchain-gcc-riscv.cmake)

project(MasterMinion LANGUAGES C ASM)

set(CMAKE_VERBOSE_MAKEFILE ON)

set(SOURCES src/main.c src/boot.S)

add_custom_executable(${TARGET_NAME} ${SOURCES})

# Use custom linker script and generate a map file
set(CMAKE_EXE_LINKER_FLAGS "-T ${LINKER_SCRIPT} -Xlinker -Map=${MAP_FILE}")

# Add explicit dependency on linker script when linking target
set_target_properties(${ELF_FILE} PROPERTIES LINK_DEPENDS ${LINKER_SCRIPT})