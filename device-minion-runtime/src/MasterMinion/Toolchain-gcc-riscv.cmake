# See https://gitlab.kitware.com/cmake/community/wikis/doc/cmake/CrossCompiling

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR riscv64)

set(GCC_PATH /esperanto/minion)

# Only search our cross-tooclhain's library paths
set(CMAKE_FIND_ROOT_PATH ${GCC_PATH})
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# TODO FIXME move this to a shared dir that isn't project specific
get_filename_component(ELFTOHEX_PATH "src/elftohex.py" ABSOLUTE)

set(CMAKE_AR         ${GCC_PATH}/bin/riscv64-unknown-elf-ar      CACHE PATH "ar"      FORCE)
set(CMAKE_RANLIB     ${GCC_PATH}/bin/riscv64-unknown-elf-ranlib  CACHE PATH "ranlib"  FORCE)
set(CMAKE_C_COMPILER ${GCC_PATH}/bin/riscv64-unknown-elf-gcc     CACHE PATH "gcc"     FORCE)
set(CMAKE_OBJCOPY    ${GCC_PATH}/bin/riscv64-unknown-elf-objcopy CACHE PATH "objcopy" FORCE)
set(CMAKE_OBJDUMP    ${GCC_PATH}/bin/riscv64-unknown-elf-objdump CACHE PATH "objdump" FORCE) 
set(CMAKE_HEXDUMP    hexdump CACHE STRING "hexdump" FORCE)
set(CMAKE_ELFTOHEX   ${ELFTOHEX_PATH} CACHE STRING "offset" FORCE)

set(ELF_FILE ${TARGET_NAME}.elf)
set(BIN_FILE ${TARGET_NAME}.bin)
set(HEX_FILE ${TARGET_NAME}.hex)
set(MAP_FILE ${TARGET_NAME}.map)
set(LST_FILE ${TARGET_NAME}.lst)

# Our gcc has -fdelete-null-pointer-checks enabled, needed for -Wnull-dereference
#
# Need mcmodel=medany instead of default medlow to be able to place code at the bottom
# of DRAM space which begins at 516G (0x8100000000)
# See https://www.sifive.com/blog/all-aboard-part-4-risc-v-code-models
#
set(CMAKE_C_FLAGS "-mcmodel=medany -Wall -Wextra -Werror -Wnull-dereference \
-Wduplicated-branches -Wduplicated-cond -Wshadow -Wpointer-arith -Wundef \
-Wbad-function-cast -Wcast-qual -Wcast-align -Wconversion -Wlogical-op \
-Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations")

# macro to create an executable .elf plus .bin, .hex, .lst and .map files
# if LINKER_SCRIPT is defined, uses it instead of the default
macro(add_riscv_executable TARGET_NAME)

    add_executable(${ELF_FILE} ${ARGN})

    # custom command to generate a raw binary image file from the elf
    add_custom_command(
        OUTPUT ${BIN_FILE}
        COMMAND ${CMAKE_OBJCOPY} -O binary ${ELF_FILE} ${BIN_FILE}
        DEPENDS ${ELF_FILE}
    )

    # custom command to generate a ZeBu hex file from the elf
    add_custom_command(
        OUTPUT ${HEX_FILE}
        COMMAND ${CMAKE_ELFTOHEX} ${ELF_FILE} ${HEX_FILE}
        DEPENDS ${ELF_FILE}
    )

    # custom command to generate an assembly listing from the elf
    add_custom_command(
        OUTPUT ${LST_FILE}
        COMMAND ${CMAKE_OBJDUMP} -h -S ${ELF_FILE} > ${LST_FILE}
        DEPENDS ${ELF_FILE}
    )

    # Always generate the bin file
    add_custom_target( 
        "bin"
        ALL
        DEPENDS ${BIN_FILE}
    )

    # Always generate the hex file
    add_custom_target( 
        "hex"
        ALL
        DEPENDS ${HEX_FILE}
    )

    # Always generate the assembly listing 
    add_custom_target( 
        "lst"
        ALL
        DEPENDS ${LST_FILE}
    )

endmacro(add_riscv_executable)
