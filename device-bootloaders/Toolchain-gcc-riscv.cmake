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

set(CMAKE_AR         ${GCC_PATH}/bin/riscv64-unknown-elf-ar      CACHE PATH   "ar"       FORCE)
set(CMAKE_RANLIB     ${GCC_PATH}/bin/riscv64-unknown-elf-ranlib  CACHE PATH   "ranlib"   FORCE)
set(CMAKE_C_COMPILER ${GCC_PATH}/bin/riscv64-unknown-elf-gcc     CACHE PATH   "gcc"      FORCE)
set(CMAKE_OBJCOPY    ${GCC_PATH}/bin/riscv64-unknown-elf-objcopy CACHE PATH   "objcopy"  FORCE)
set(CMAKE_OBJDUMP    ${GCC_PATH}/bin/riscv64-unknown-elf-objdump CACHE PATH   "objdump"  FORCE)

# Our gcc has -fdelete-null-pointer-checks enabled by default, needed for -Wnull-dereference
#
# Need -mcmodel=medany instead of default medlow to be able to place code at the bottom
# of DRAM space which begins at 516G (0x8100000000)
# See https://www.sifive.com/blog/all-aboard-part-4-risc-v-code-models
#
# Explicitly set -march and -mabi to disable compressed instructions in A0
#
# Worker minion will likely be freestanding without libc, -ffreestanding
# may need -funwind-tables for backtrace
# FreeRTOS is not compatible with -Wduplicated-cond and -Wduplicated-branches at -Og or higher
set(CMAKE_C_FLAGS "-Og -g3 -std=gnu11 --specs=nano.specs -mcmodel=medany -march=rv64imf -mabi=lp64f \
-flto -ffunction-sections -fdata-sections -fstack-usage -Wall -Wextra -Werror -Wdouble-promotion -Wformat \
-Wnull-dereference -Wswitch-enum -Wshadow -Wstack-usage=256 \
-Wpointer-arith -Wundef -Wbad-function-cast -Wcast-qual -Wcast-align -Wconversion -Wlogical-op \
-Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wno-main" CACHE STRING "c flags" FORCE)

add_definitions("-DRISCV_ET_MINION")

# macro to create an executable .elf plus .bin, .lst and .map files
# if LINKER_SCRIPT is defined, uses it instead of the default
macro(add_riscv_executable TARGET_NAME)
    set(ELF_FILE ${TARGET_NAME}.elf)
    if (DEFINED TARGET_RUNTIME_OUTPUT_DIRECTORY)
        set(ELF_FILE_PATH ${TARGET_RUNTIME_OUTPUT_DIRECTORY}/${ELF_FILE})
        set(BIN_FILE ${TARGET_RUNTIME_OUTPUT_DIRECTORY}/${TARGET_NAME}.bin)
        set(MAP_FILE ${TARGET_RUNTIME_OUTPUT_DIRECTORY}/${TARGET_NAME}.map)
        set(LST_FILE ${TARGET_RUNTIME_OUTPUT_DIRECTORY}/${TARGET_NAME}.lst)
    else()
        set(ELF_FILE_PATH ${ELF_FILE})
        set(BIN_FILE ${TARGET_NAME}.bin)
        set(MAP_FILE ${TARGET_NAME}.map)
        set(LST_FILE ${TARGET_NAME}.lst)
    endif()

    add_executable(${ELF_FILE} ${ARGN}) # ARGN is "the rest of the arguments", i.e. the source list
    if (DEFINED TARGET_RUNTIME_OUTPUT_DIRECTORY)
        set_target_properties(${ELF_FILE} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${TARGET_RUNTIME_OUTPUT_DIRECTORY})
    endif()
    target_include_directories(${ELF_FILE} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/include)

    if (DEFINED LINKER_SCRIPT)
        # Get the absolute path to the linker script
        get_filename_component(LINKER_SCRIPT_ABS_PATH ${LINKER_SCRIPT} ABSOLUTE)

        # Add explicit dependency on linker script when linking target
        set_target_properties(${ELF_FILE} PROPERTIES LINK_DEPENDS ${LINKER_SCRIPT_ABS_PATH})

        # Use custom linker script
        set(ELF_EXE_LINKER_FLAGS "-nostdlib -nostartfiles -Wl,--gc-sections -Xlinker -Map=${MAP_FILE} -T ${LINKER_SCRIPT_ABS_PATH}")
    else()
        set(ELF_EXE_LINKER_FLAGS "-nostdlib -nostartfiles -Wl,--gc-sections -Xlinker -Map=${MAP_FILE}")
    endif()

    set_target_properties(${ELF_FILE} PROPERTIES LINK_FLAGS ${ELF_EXE_LINKER_FLAGS})

    if (DEFINED LINKER_SCRIPT_DEPENDENCY)
        # Get the absolute path to the file the linker script depends on
        get_filename_component(LINKER_SCRIPT_DEPENDENCY_ABS_PATH ${LINKER_SCRIPT_DEPENDENCY} ABSOLUTE)

        # Add explicit dependency on file when linking target
        set_target_properties(${ELF_FILE} PROPERTIES LINK_DEPENDS ${LINKER_SCRIPT_DEPENDENCY_ABS_PATH})
    endif()

    # Must use target_link_libraries() to add libraries to get correct symbol resolution -
    # putting libraries in CMAKE_EXE_LINKER_FLAGS is too early
    target_link_libraries(${ELF_FILE} PRIVATE c m gcc)

    # Pass -L{$SHARED_INC_DIR} to linker so linker scripts can INCLUDE shared defines
    target_link_directories(${ELF_FILE} PRIVATE ${SHARED_INC_DIR})

    # custom command to generate a bin from the elf
    add_custom_command(
        OUTPUT ${BIN_FILE}
        COMMAND ${CMAKE_OBJCOPY} -O binary ${ELF_FILE_PATH} ${BIN_FILE}
        DEPENDS ${ELF_FILE}
    )

    # custom command to generate an assembly listing from the elf
    add_custom_command(
        OUTPUT ${LST_FILE}
        COMMAND ${CMAKE_OBJDUMP} -h -S ${ELF_FILE_PATH} > ${LST_FILE}
        DEPENDS ${ELF_FILE}
    )

    add_custom_target(${TARGET_NAME}.map ALL
        DEPENDS ${ELF_FILE}
                ${CMAKE_CURRENT_BINARY_DIR}/${MAP_FILE}
    )

    add_custom_target(${TARGET_NAME}.lst ALL
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${LST_FILE}
    )

    add_custom_target(${TARGET_NAME}.bin ALL
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${BIN_FILE}
    )

endmacro(add_riscv_executable)
