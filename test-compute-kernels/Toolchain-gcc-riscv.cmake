# See https://gitlab.kitware.com/cmake/community/wikis/doc/cmake/CrossCompiling

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR riscv64)

# Only search our cross-tooclhain's library paths
set(CMAKE_FIND_ROOT_PATH ${GCC_SYSROOT_PATH})
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

get_filename_component(GET_GIT_HASH_ABS_PATH "${ESPERANTO_DEVICE_MINION_RUNTIME_BIN_DIR}/esperanto-fw/get_git_hash.py" ABSOLUTE)
get_filename_component(GET_GIT_VERSION_ABS_PATH "${ESPERANTO_DEVICE_MINION_RUNTIME_BIN_DIR}/esperanto-fw/get_git_version.py" ABSOLUTE)

set(CMAKE_AR         ${GCC_SYSROOT_PATH}/bin/riscv64-unknown-elf-ar      CACHE PATH   "ar"       FORCE)
set(CMAKE_RANLIB     ${GCC_SYSROOT_PATH}/bin/riscv64-unknown-elf-ranlib  CACHE PATH   "ranlib"   FORCE)
set(CMAKE_C_COMPILER ${GCC_SYSROOT_PATH}/bin/riscv64-unknown-elf-gcc     CACHE PATH   "gcc"      FORCE)
set(CMAKE_OBJCOPY    ${GCC_SYSROOT_PATH}/bin/riscv64-unknown-elf-objcopy CACHE PATH   "objcopy"  FORCE)
set(CMAKE_OBJDUMP    ${GCC_SYSROOT_PATH}/bin/riscv64-unknown-elf-objdump CACHE PATH   "objdump"  FORCE)
set(CMAKE_ELFTOHEX   ${ELFTOHEX_ABS_PATH}                        CACHE PATH   "elftohex" FORCE)
set(CMAKE_GET_GIT_HASH ${GET_GIT_HASH_ABS_PATH}                  CACHE PATH   "get-git-hash" FORCE)
set(CMAKE_GET_GIT_VERSION ${GET_GIT_VERSION_ABS_PATH}            CACHE PATH   "get-git-version" FORCE)

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

# macro to create an executable .elf plus .bin, .hex, .lst and .map files
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

    set(HEX_FILE ${TARGET_NAME}.hex)

    if (NOT DEFINED GIT_HASH_STRING)
        execute_process(COMMAND ${CMAKE_GET_GIT_HASH} ${CMAKE_CURRENT_SOURCE_DIR} OUTPUT_VARIABLE GIT_HASH_STRING RESULT_VARIABLE RES)
        if (RES)
            message(FATAL_ERROR "Get git hash string failed with: ${OUTPUT_VARIABLE}")
        endif()
    endif()
    if (NOT DEFINED GIT_HASH_ARRAY)
        execute_process(COMMAND ${CMAKE_GET_GIT_HASH} -a ${CMAKE_CURRENT_SOURCE_DIR} OUTPUT_VARIABLE GIT_HASH_ARRAY RESULT_VARIABLE RES)
        if (RES)
            message(FATAL_ERROR "Get git hash array string failed with: ${OUTPUT_VARIABLE}")
        endif()
    endif()
    if (NOT DEFINED GIT_VERSION_STRING)
        execute_process(COMMAND ${CMAKE_GET_GIT_VERSION} ${CMAKE_CURRENT_SOURCE_DIR} OUTPUT_VARIABLE GIT_VERSION_STRING RESULT_VARIABLE RES)
        if (RES)
            message(FATAL_ERROR "Get git version failed with: ${OUTPUT_VARIABLE}")
        endif()
    endif()
    if (NOT DEFINED GIT_VERSION_ARRAY)
        execute_process(COMMAND ${CMAKE_GET_GIT_VERSION} -a ${CMAKE_CURRENT_SOURCE_DIR} OUTPUT_VARIABLE GIT_VERSION_ARRAY RESULT_VARIABLE RES)
        if (RES)
            message(FATAL_ERROR "Get git version array string failed with: ${OUTPUT_VARIABLE}")
        endif()
    endif()

    configure_file (
        "${CMAKE_CURRENT_SOURCE_DIR}/include/build_configuration.h.in"
        "${CMAKE_CURRENT_BINARY_DIR}/include/build_configuration.h"
    )

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
    target_link_libraries(${ELF_FILE} c m gcc)

    # Pass -L{$SHARED_INC_DIR} to linker so linker scripts can INCLUDE shared defines
    target_link_directories(${ELF_FILE} PRIVATE ${SHARED_INC_DIR})

    # custom command to generate a bin from the elf
    add_custom_command(
        OUTPUT ${BIN_FILE}
        COMMAND ${CMAKE_OBJCOPY} -O binary ${ELF_FILE_PATH} ${BIN_FILE}
        DEPENDS ${ELF_FILE}
    )

    if (DEFINED ZEBU_TARGET)
        # custom command to generate a ZeBu hex file from the elf
        # This file creates multiple output files that are not captured correctly as outputs.
        # Create a token file file to mark success of converting the ELF to hex, and prevent
        # regeneration of the hex files if the ELF has not changed
        add_custom_command(
            OUTPUT ${HEX_FILE}.done
            COMMAND ${CMAKE_ELFTOHEX} ${ZEBU_TARGET} ${ELF_FILE_PATH} --output-file ${ZEBU_FILENAME}
            COMMAND date > ${HEX_FILE}.done
            DEPENDS ${ELF_FILE}
        )

        # call elftohex and get the list of files it will generate per target
        execute_process(
            COMMAND ${CMAKE_ELFTOHEX} ${ZEBU_TARGET} ${ELF_FILE_PATH} --output-file ${ZEBU_FILENAME} --print-output-files
            OUTPUT_VARIABLE "${TARGET_NAME}_OUTPUT"
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
    endif()

    # custom command to generate an assembly listing from the elf
    add_custom_command(
        OUTPUT ${LST_FILE}
        COMMAND ${CMAKE_OBJDUMP} -h -S ${ELF_FILE_PATH} > ${LST_FILE}
        DEPENDS ${ELF_FILE}
    )

    # These custom targets are unintuitive:
    # Must use a unique name: can't be the same between components (e.g. MasterMinion and ServiceProcessor), so include ${TARGET_NAME}
    # Must not match an existing target (e.g. can't be "${BIN_FILE"}), so append ".always". Don't understand this.

    # Always generate the bin file
    add_custom_target(
        "${TARGET_NAME}.bin.always"
        ALL
        DEPENDS ${BIN_FILE}
    )

    if (DEFINED ZEBU_TARGET)
        # Generate the ZeBu hex file
        add_custom_target(
            "${TARGET_NAME}.hex.always"
            DEPENDS ${ELFTOHEX_ABS_PATH} ${HEX_FILE}.done
        )
    endif()

    # Always generate the assembly listing
    add_custom_target(
        "${TARGET_NAME}.lst.always"
        ALL
        DEPENDS ${LST_FILE}
    )

    find_package(EsperantoDeviceMinionRuntime REQUIRED)
    set(MINION_RUNTIME_PACKAGE_NAME EsperantoDeviceMinionRuntime)

    get_property(MASTER_MINION_ELF TARGET ${MINION_RUNTIME_PACKAGE_NAME}::MasterMinion.elf PROPERTY LOCATION)
    get_property(MACHINE_MINION_ELF TARGET ${MINION_RUNTIME_PACKAGE_NAME}::MachineMinion.elf PROPERTY LOCATION)
    get_property(WORKER_MINION_ELF TARGET ${MINION_RUNTIME_PACKAGE_NAME}::WorkerMinion.elf PROPERTY LOCATION)


endmacro(add_riscv_executable)
