cmake_minimum_required(VERSION 3.5)

set(TARGET_NAME "MasterMinion")

#Get the absolute path to the linker script
get_filename_component(LINKER_SCRIPT "src/sections.ld" ABSOLUTE)

# Must set CMAKE_TOOLCHAIN_FILE before project() is called
set(CMAKE_TOOLCHAIN_FILE Toolchain-gcc-riscv.cmake)

project(MasterMinion LANGUAGES C)

set(CMAKE_VERBOSE_MAKEFILE ON)

# gcc has -fdelete-null-pointer-checks enabled by default, needed for -Wnull-dereference
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror -Wnull-dereference \
 -Wduplicated-branches -Wduplicated-cond -Wshadow -Wpointer-arith -Wundef \
 -Wbad-function-cast -Wcast-qual -Wcast-align -Wconversion -Wlogical-op \
 -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations")

add_executable(${TARGET_NAME} src/main.c)

# Use custom linker script instead of default
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -T ${LINKER_SCRIPT}")

# Add explicit dependency on linker script when linking target
set_target_properties(${TARGET_NAME} PROPERTIES LINK_DEPENDS ${LINKER_SCRIPT})


