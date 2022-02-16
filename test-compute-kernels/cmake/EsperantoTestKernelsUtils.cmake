# Helper function for creating different instances of randomized tests
# In this case we expect that a different instance of an elf file will be compiled
# based on an auto-generated header
# Arguments:
# TEST_NAME: Name of the test
# SOURCES : Input compile sources
# INCLUDES : Test include directories
macro(test_kernel)
    set(options)
    set(oneValueArgs NAME)
    set(multiValueArgs SOURCES INCLUDES)
    cmake_parse_arguments(TEST_KERNEL  "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if (NOT TEST_KERNEL_NAME)
        message(SEND_ERROR "Error: test_kernel() called without NAME argument!")
    endif()

    if (NOT TEST_KERNEL_SOURCES)
        message(SEND_ERROR "Error: test_kernel() called without SOURCES argument!")
    endif()
    # INCLUDES is optional

    set(TARGET_NAME ${TEST_KERNEL_NAME})
    if (NOT LINKER_SCRIPT)
        set(LINKER_SCRIPT ${PROJECT_SOURCE_DIR}/src/shared/sections.ld)
    endif()
    set(ZEBU_TARGET DDR_NEW)
    set(ZEBU_FILENAME memImage)

    add_riscv_executable(${TARGET_NAME})
    target_sources(${TARGET_NAME}.elf PRIVATE ${TEST_KERNEL_SOURCES})
    target_include_directories(${TARGET_NAME}.elf PRIVATE ${TEST_KERNEL_INCLUDES})
    target_link_libraries(${TARGET_NAME}.elf
        PRIVATE
            test-compute-kernels::shared_kernel
            et-common-libs::cm-umode
            esperantoTrace::et_trace
    )
    set_target_properties(${TARGET_NAME}.elf 
        PROPERTIES
            INTERPROCEDURAL_OPTIMIZATION TRUE  # fPIC
    )

    install(TARGETS ${TARGET_NAME}.elf
        EXPORT EsperantoTestKernelsTargets
        RUNTIME DESTINATION ${LIB_INSTALL_DIR}/esperanto-fw/kernels
        COMPONENT kernels
    )
endmacro()


# Helper function for creating different instances of randomized tests
# In this case we expect that a different instance of an elf file will be compiled
# based on an auto-generated header
# Arguments:
# TEST_NAME: Name of the test
# NUMBER_OF_TESTS : number of individual tests to create
# GEN_COMMAND : Command to generate to produce the
# OUTPUT_GEN_FILE : Name of the output generated files
# SOURCES : Input compile sources
macro(randomized_test)
    set(options)
    set(oneValueArgs NAME NUMBER_OF_TESTS)
    set(multiValueArgs SOURCES OUTPUT_GEN_FILES GEN_COMMAND)
    cmake_parse_arguments(RANDOMIZED_TEST  "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if (NOT RANDOMIZED_TEST_NAME)
        message(SEND_ERROR "Error: randomized_test() called without NAME argument!")
    endif()

    if (NOT DEFINED RANDOMIZED_TEST_NUMBER_OF_TESTS)
        message(SEND_ERROR "Error: randomized_test() called without NUMBER_OF_TESTS argument!")
    endif()

    if (NOT RANDOMIZED_TEST_SOURCES)
        message(SEND_ERROR "Error: randomized_test() called without SOURCES argument!")
    endif()

    if (NOT RANDOMIZED_TEST_OUTPUT_GEN_FILES)
        message(SEND_ERROR "Error: randomized_test() called without OUTPUT_GEN_FILES argument!")
    endif()

    if (NOT RANDOMIZED_TEST_GEN_COMMAND)
        message(SEND_ERROR "Error: randomized_test() called without GEN_COMMAND argument!")
    endif()

    foreach(TEST_ID RANGE ${RANDOMIZED_TEST_NUMBER_OF_TESTS})

      set(TEST_NAME "${RANDOMIZED_TEST_NAME}_${TEST_ID}")
      set(TEST_BUILD_FOLDER ${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME})
      # Trace tests disabled for now until we have the new Tracing lib
      set(TRACE_ENABLED FALSE)

      execute_process(COMMAND mkdir -p ${TEST_BUILD_FOLDER})

      list(TRANSFORM RANDOMIZED_TEST_OUTPUT_GEN_FILES PREPEND ${TEST_BUILD_FOLDER}/ OUTPUT_VARIABLE OUTPUT_GEN_FILES)

      add_custom_command(
        OUTPUT ${TEST_BUILD_FOLDER}/gen.done
        COMMAND mkdir -p ${TEST_BUILD_FOLDER} && mkdir -p ${TEST_BUILD_FOLDER}/include
        COMMAND cd ${TEST_BUILD_FOLDER} && ${RANDOMIZED_TEST_GEN_COMMAND}
        COMMAND date > ${TEST_BUILD_FOLDER}/gen.done
      )

      add_custom_target(${TEST_NAME}-gen-files
        DEPENDS ${TEST_BUILD_FOLDER}/gen.done
      )

      set(TARGET_NAME ${TEST_NAME})
      set(LINKER_SCRIPT ${PROJECT_SOURCE_DIR}/src/shared/sections.ld)
      set(ZEBU_TARGET DDR_NEW)
      set(ZEBU_FILENAME ${TEST_BUILD_FOLDER}/memImage)
      #set(TARGET_RUNTIME_OUTPUT_DIRECTORY ${TEST_BUILD_FOLDER})

      add_riscv_executable(${TARGET_NAME})
      target_sources(${TARGET_NAME}.elf PRIVATE ${RANDOMIZED_TEST_SOURCES})
      target_include_directories(${TARGET_NAME}.elf
          PRIVATE
              ${CMAKE_CURRENT_SOURCE_DIR}/include
              ${TEST_BUILD_FOLDER}/include
      )
      target_link_libraries(${TARGET_NAME}.elf
          PRIVATE 
              test-compute-kernels::shared_kernel
              et-common-libs::cm-umode
      )

      add_dependencies(${TARGET_NAME}.elf ${TEST_NAME}-gen-files)

      install(TARGETS ${TARGET_NAME}.elf
          EXPORT esperanto-fw-targets
          RUNTIME DESTINATION ${LIB_INSTALL_DIR}/esperanto-fw/kernels/${TARGET_NAME}
          COMPONENT kernels
      )

      configure_file(${CMAKE_CURRENT_SOURCE_DIR}/mem_desc.txt ${TEST_BUILD_FOLDER}/mem_desc.txt)

    endforeach()

endmacro(randomized_test)
