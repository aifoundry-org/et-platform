#------------------------------------------------------------------------------
# Copyright (C) 2019, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

get_filename_component(SHARED_KERNEL_SRC_DIR "shared" ABSOLUTE)
get_filename_component(SHARED_KERNEL_INC_DIR "shared/include" ABSOLUTE)

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
  cmake_parse_arguments(RANDOMIZED_TEST
    ""
    "NAME;NUMBER_OF_TESTS"
    "OUTPUT_GEN_FILES;SOURCES;GEN_COMMAND"
    ${ARGN})

  include_directories(include)
  include_directories(${SHARED_INC_DIRS})
  include_directories(${SHARED_KERNEL_INC_DIR})
  include_directories(${ESPERANTO_DEVICE_API_INCLUDE_DIR})
  foreach(TEST_ID RANGE ${RANDOMIZED_TEST_NUMBER_OF_TESTS})

    set(TEST_NAME "${RANDOMIZED_TEST_NAME}_${TEST_ID}")
    set(TEST_BUILD_FOLDER ${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME})

    execute_process(COMMAND mkdir -p ${TEST_BUILD_FOLDER})

    list(TRANSFORM RANDOMIZED_TEST_OUTPUT_GEN_FILES PREPEND ${TEST_BUILD_FOLDER}/ OUTPUT_VARIABLE OUTPUT_GEN_FILES)

    add_custom_command(
      OUTPUT ${TEST_BUILD_FOLDER}/gen.done
      COMMAND mkdir -p ${TEST_BUILD_FOLDER} && mkdir -p ${TEST_BUILD_FOLDER}/include
      COMMAND cd ${TEST_BUILD_FOLDER} && ${RANDOMIZED_TEST_GEN_COMMAND}
      COMMAND date > ${TEST_BUILD_FOLDER}/gen.done
      )

    add_custom_target(${TEST_NAME}-gen-files
      DEPENDS ${TEST_BUILD_FOLDER}/gen.done)

    set(SOURCES
      ${SHARED_KERNEL_SRC_DIR}/crt.S
      ${SHARED_SRC_DIR}/log.c
      ${SHARED_SRC_DIR}/printf.c # log needs printf.c to build string
      ${SHARED_SRC_DIR}/message.c # log needs message.c to send log message to master minion
      ${RANDOMIZED_TEST_SOURCES}
      )

    set(TARGET_NAME ${TEST_NAME})
    set(LINKER_SCRIPT ${SHARED_KERNEL_SRC_DIR}/sections.ld)
    set(ZEBU_TARGET DDR_NEW)
    set(ZEBU_FILENAME ${TEST_BUILD_FOLDER}/memImage)
    set(TARGET_RUNTIME_OUTPUT_DIRECTORY ${TEST_BUILD_FOLDER})
    add_riscv_executable(${TARGET_NAME} ${SOURCES})
    target_include_directories(${TARGET_NAME}.elf PRIVATE ${TEST_BUILD_FOLDER}/include)
    add_dependencies(${TARGET_NAME}.elf ${TEST_NAME}-gen-files)

    install(TARGETS ${TARGET_NAME}.elf
      EXPORT esperanto-fw-targets
      RUNTIME DESTINATION ${LIB_INSTALL_DIR}/esperanto-fw/kernels/${TARGET_NAME}
      COMPONENT kernels)

    string(REPLACE " " ";" OUTPUT_LIST ${${TARGET_NAME}_OUTPUT})
    foreach(FILE ${OUTPUT_LIST})
      install(FILES ${FILE}
        DESTINATION ${LIB_INSTALL_DIR}/esperanto-fw/kernels/${TEST_NAME}
        COMPONENT zebu)
    endforeach(FILE)

  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/mem_desc.txt ${TEST_BUILD_FOLDER}/mem_desc.txt)

  endforeach()

endmacro(randomized_test)


add_subdirectory("beef")
add_subdirectory("crc32")
add_subdirectory("empty")
add_subdirectory("hang")
add_subdirectory("log")
add_subdirectory("power")
add_subdirectory("random_data")
add_subdirectory("random_read")
add_subdirectory("tl_tfma")
add_subdirectory("coop_tl_tfma")
add_subdirectory("tl_tfma_reduce")
add_subdirectory("producer_consumer")
add_subdirectory("tl_tfma_tstore")
add_subdirectory("gatomic_test_allt_same_addr")
add_subdirectory("gatomic_test_1mps_loop_shires")
add_subdirectory("gatomic_test_allt_rand_addr")
add_subdirectory("cacheop_prefetch_s64")
add_subdirectory("cacheop_prefetch_dst_stride")
add_subdirectory("cacheop_prefetch_l2l3_loop_shires")
add_subdirectory("cacheop_prefetch_l2l3_rstride")
add_subdirectory("cacheop_prefetch_l2scp_1mps_raddr")
add_subdirectory("cacheop_prefetch_l2scp_1mps_loop_shires")
add_subdirectory("cacheop_flev_1mps_loop_shires")
add_subdirectory("cacheop_flev_stride_dst")
add_subdirectory("cacheop_flev_s64")
add_subdirectory("fcc_with_cacheops")
#add_subdirectory("fcc_overflow")
add_subdirectory("uberkernel")
