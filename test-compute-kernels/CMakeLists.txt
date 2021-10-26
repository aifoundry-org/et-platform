#------------------------------------------------------------------------------
# Copyright (C) 2019, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.5)

include(ExternalProject)
include(GNUInstallDirs)
cmake_policy(SET CMP0076 NEW)


set(GCC_SYSROOT_PATH /esperanto/minion CACHE STRING "Path to the gcc riscv toolchain sysroot folder")

# First step as the artifacts of this package are consumed by the toolchain script
find_package(EsperantoDeviceMinionRuntime REQUIRED)
find_package(esperantoTrace REQUIRED)

# Must set CMAKE_TOOLCHAIN_FILE before project() is called
set(CMAKE_TOOLCHAIN_FILE Toolchain-gcc-riscv.cmake)

project(EsperantoTestKernelsLoader VERSION 0.1.0 LANGUAGES C ASM)

find_package(et-common-libs REQUIRED COMPONENTS cm-umode)

### Add package information
include(CMakePackageConfigHelpers)

set(BIN_INSTALL_DIR bin CACHE STRING "Bin install dir")
set(INCLUDE_INSTALL_DIR include/ CACHE STRING "Include install dir")
set(LIB_INSTALL_DIR  lib/  CACHE STRING "Library install dir")
set(SYSCONFIG_INSTALL_DIR etc/et-fw/ CACHE STRING "Config install dir")

get_property(DEVICE_COMMON_INCLUDE_PATH
  TARGET et-common-libs::cm-umode
  PROPERTY INTERFACE_INCLUDE_DIRECTORIES)

set(SHARED_INC_DIR ${PROJECT_SOURCE_DIR}/shared/include)

get_filename_component(SHARED_KERNEL_SRC_DIR "shared" ABSOLUTE)
get_filename_component(SHARED_KERNEL_INC_DIR "shared/include" ABSOLUTE)
get_filename_component(SCRIPTS_KERNEL_DIR "scripts" ABSOLUTE)

list(APPEND COMMON_KERNEL_SRC_FILES
  ${SHARED_KERNEL_SRC_DIR}/crt.S
  )

# Helper function for creating different instances of randomized tests
# In this case we expect that a different instance of an elf file will be compiled
# based on an auto-generated header
# Arguments:
# TEST_NAME: Name of the test
# SOURCES : Input compile sources
# INCLUDES : Test include directories
macro(test_kernel)
  cmake_parse_arguments(TEST_KERNEL
    ""
    "NAME;"
    "SOURCES;INCLUDES"
    ${ARGN})

  set(SOURCES
    ${COMMON_KERNEL_SRC_FILES}
    ${TEST_KERNEL_SOURCES}
    )

  set(TARGET_NAME ${TEST_KERNEL_NAME})
  set(LINKER_SCRIPT ${SHARED_KERNEL_SRC_DIR}/sections.ld)
  set(ZEBU_TARGET DDR_NEW)
  set(ZEBU_FILENAME memImage)

  include_directories(${TEST_KERNEL_INCLUDES})
  include_directories(${SHARED_INC_DIRS})
  include_directories(${SHARED_KERNEL_INC_DIR})

  add_riscv_executable(${TARGET_NAME} ${SOURCES})

  set(EXPORT_NAME EsperantoTestKernelsTargets)

  target_link_libraries(${TARGET_NAME}.elf
    et-common-libs::cm-umode
    esperantoTrace::et_trace
    )

  install(TARGETS ${TARGET_NAME}.elf
    EXPORT ${EXPORT_NAME}
    RUNTIME DESTINATION ${LIB_INSTALL_DIR}/esperanto-fw/kernels
    COMPONENT kernels)
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
  cmake_parse_arguments(RANDOMIZED_TEST
    ""
    "NAME;NUMBER_OF_TESTS"
    "OUTPUT_GEN_FILES;SOURCES;GEN_COMMAND"
    ${ARGN})

  include_directories(include)
  include_directories(${SHARED_INC_DIRS})
  include_directories(${SHARED_KERNEL_INC_DIR})
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
      DEPENDS ${TEST_BUILD_FOLDER}/gen.done)

    set(SOURCES
      ${COMMON_KERNEL_SRC_FILES}
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

    target_link_libraries(${TARGET_NAME}.elf et-common-libs::cm-umode)

    set(EXPORT_NAME esperanto-fw-targets)

    install(TARGETS ${TARGET_NAME}.elf
      EXPORT ${EXPORT_NAME}
      RUNTIME DESTINATION ${LIB_INSTALL_DIR}/esperanto-fw/kernels/${TARGET_NAME}
      COMPONENT kernels)

  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/mem_desc.txt ${TEST_BUILD_FOLDER}/mem_desc.txt)

  endforeach()

endmacro(randomized_test)

add_subdirectory("abort")
add_subdirectory("add_vector")
add_subdirectory("cm_umode_test")
add_subdirectory("memset")
add_subdirectory("bandwidth")
add_subdirectory("beef")
add_subdirectory("trace")
# Old Trace tests disabled for now until we update them to use new Tracing lib
#add_subdirectory("trace_ring_buffer")
add_subdirectory("crc32")
add_subdirectory("echo")
add_subdirectory("empty")
add_subdirectory("error")
add_subdirectory("exception")
add_subdirectory("hang")
add_subdirectory("log")
add_subdirectory("power")
add_subdirectory("random_data")
add_subdirectory("random_read")
add_subdirectory("tl_tfma_2s")
add_subdirectory("tl_tfma_fc")
add_subdirectory("coop_tl_tfma_2s")
add_subdirectory("coop_tl_tfma_fc")
add_subdirectory("tl_tfma_reduce_2s")
add_subdirectory("tl_tfma_reduce_fc")
add_subdirectory("producer_consumer_2s")
add_subdirectory("producer_consumer_fc")
add_subdirectory("tl_tfma_tstore_2s")
add_subdirectory("tl_tfma_tstore_fc")
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
add_subdirectory("load_bw")
add_subdirectory("load_lat")
add_subdirectory("write_bw")
add_subdirectory("prefetch_bw")
add_subdirectory("jump_loop")
add_subdirectory("mlp")
add_subdirectory("multierror")

# Trace tests disabled for now until we have the new Tracing lib
#add_subdirectory("pmc_test_2s")
#add_subdirectory("pmc_test_fc")

# Export the package for use from the build-tree
# (this registers the build-tree with a global CMake-registry)
export(PACKAGE EsperantoTestKernelsTargets)

# Create the *-config.cmake and *-config-version.cmake files
configure_package_config_file(EsperantoTestKernelsConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/EsperantoTestKernelsConfig.cmake
  INSTALL_DESTINATION ${LIB_INSTALL_DIR}/EsperantoTestKernels/cmake
  PATH_VARS INCLUDE_INSTALL_DIR SYSCONFIG_INSTALL_DIR LIB_INSTALL_DIR BIN_INSTALL_DIR
  )
write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/EsperantoTestKernelsConfigVersion.cmake
  VERSION ${VERSION}
  COMPATIBILITY SameMajorVersion )

# Install the *Config.cmake and *ConfigVersion.cmake
install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/EsperantoTestKernelsConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/EsperantoTestKernelsConfigVersion.cmake"
  DESTINATION ${LIB_INSTALL_DIR}/cmake/EsperantoTestKernels
  COMPONENT machine-minion)

# Install the export set for use with the install-tree
install(EXPORT EsperantoTestKernelsTargets
  NAMESPACE esperanto-test-kernels::
  DESTINATION "${LIB_INSTALL_DIR}/cmake/EsperantoTestKernels")
