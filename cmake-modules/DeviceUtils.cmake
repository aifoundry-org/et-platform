
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

    set_target_properties(${ELF_FILE} PROPERTIES COMPILE_FLAGS "-flto")
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

function(target_report_riscv_size TARGET_NAME)
    find_program(SIZE_PROGRAM riscv64-unknown-elf-size)
    if (NOT SIZE_PROGRAM)
        message(WARNING "riscv64-unknown-elf-size not found. Cannot analyze target: ${TARGET_NAME}")
        return()
    endif()

    add_custom_command(TARGET ${TARGET_NAME}
        POST_BUILD
        DEPENDS ${TARGET_NAME}
        COMMAND ${SIZE_PROGRAM} --format=SysV ${TARGET_NAME}
        COMMAND ${SIZE_PROGRAM} --format=berkeley ${TARGET_NAME}
        COMMENT "Generated ${TARGET_NAME} size (build_type: ${CMAKE_BUILD_TYPE})"
    )
endfunction(target_report_riscv_size)

function(target_report_riscv_elf_file_header TARGET_NAME)
    find_program(READELF_PROGRAM riscv64-unknown-elf-readelf)
    if (NOT READELF_PROGRAM)
        message(WARNING "riscv64-unknown-elf-readelf not found. Cannot analyze target: ${TARGET_NAME}")
        return()
    endif()

    add_custom_command(TARGET ${TARGET_NAME}
        POST_BUILD
        DEPENDS ${TARGET_NAME}
        COMMAND ${READELF_PROGRAM} -h ${TARGET_NAME}
        COMMENT "Generated ${TARGET_NAME} elf file header (build_type: ${CMAKE_BUILD_TYPE})"
    )
endfunction(target_report_riscv_elf_file_header)