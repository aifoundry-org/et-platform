#------------------------------------------------------------------------------
# Copyright (C) 2019, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

include(ExternalProject)
include(GoogleTest)

include_directories(${GTEST_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/../include)
include_directories(${EXTERNAL_INCLUDE_DIRECTORIES})

link_directories(${GTEST_DIR}/lib64
  ${EXTERNAL_LINK_DIRECTORIES}
  )

### External dependencies

set(CI_DEPENDENCIES convolution.elf
                    bootrom.mem
                    gpu_0_softmax_110.elf
  )
set(CI_DN_TARGETS "")

## Download the external depednencies from Artifactory
foreach(CI_DEPENDENCY ${CI_DEPENDENCIES})
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${CI_DEPENDENCY}
    COMMAND mkdir -p `dirname ${CI_DEPENDENCY}`
    COMMAND wget http://sc-artifactory1.esperanto.ai/artifactory/sw-ci-inputs/runtime/${CI_DEPENDENCY} -O ${CI_DEPENDENCY}
    )
  list(APPEND CI_DN_TARGETS ${CMAKE_CURRENT_BINARY_DIR}/${CI_DEPENDENCY})
endforeach(CI_DEPENDENCY)

add_custom_target(ci-dependencies
  DEPENDS ${CI_DN_TARGETS}
  )

add_compile_options(-g3 -O0 -std=c++17)

set(COMMON_TEST_LIBRARIES
  -Wl,--whole-archive etrt_static -Wl,--no-whole-archive
  absl::flags
  absl::flags_parse
  etsim_api
  protobuf
  grpc++
  grpc
  gpr
  z
  cares
  address_sorting
  libgtest_main.a
  libgtest.a
  ${GLOG_LIBRARIES}
  ${FMTLIB_LIBRARIES}
  pthread
  stdc++fs
  )


## Helper function for creating the different test binary targets
## Args:
##  TEST_INFO: takes a list of string where the string a column (":") separated arguments
##     to emulate tuples where the first element of the tuple is the test source file
##     and the send element is any additional test arguments
##  TEST_LABEL String with the list of labels to apply to the test
function(create_test_targets TEST_LIST TEST_LABEL)
  foreach(TEST_INFO ${TEST_LIST})
    string(REPLACE ":" ";" TMP_LIST ${TEST_INFO})
    list(GET TMP_LIST 0 TEST_FILE)
    message("Test source: " ${TEST_FILE})
    list(GET TMP_LIST 1 TEST_ARGS)
    # Remove the quotes from the string that we have passed otherwise they
    # survice in the invocation of gtest and result in incorrect argument parsing
    string(REPLACE "\"" "" TEST_ARGS ${TEST_ARGS})
    # Convert the string into a cmake list (";" separated) this will result in the2
    # argument list looking like --arg1=val1;--arg2=val2 etc
    # When passed to gtest_discover_tests in EXTRA_ARGS this will result in generating
    # "--arg1=val1"  "--arg2=val2"  that is the correct way to escape the arguments.
    string(REPLACE " " ";" TEST_ARGS "${TEST_ARGS}")
    message("Test args: " ${TEST_ARGS})
    get_filename_component(TEST_TARGET ${TEST_FILE} NAME_WE)
    add_executable(${TEST_TARGET} ${TEST_FILE})
    add_dependencies(${TEST_TARGET} googletest)
    add_dependencies(${TEST_TARGET} ${external_project_dependencies})
    add_dependencies(${TEST_TARGET} ci-dependencies)
    target_link_libraries(${TEST_TARGET} ${COMMON_TEST_LIBRARIES})
    target_include_directories(${TEST_TARGET} PUBLIC ${GFLAGS_INCLUDE_DIRS})
    target_include_directories(${TEST_TARGET} PUBLIC ${GLOG_INCLUDE_DIRS})
    gtest_discover_tests(${TEST_TARGET}
      EXTRA_ARGS ${TEST_ARGS}
      TEST_PREFIX runtime:
      PROPERTIES LABELS ${TEST_LABEL}
      )
  endforeach(TEST_INFO)
endfunction()


set(TEST_LIST
  test_error_or.cc:""
  test_command_response.cc:""
  test_command_queue.cc:""
  test_device_manager.cc:""
  test_device_target_sysemu_grpc.cc:""
  test_elf_parsing.cc:""
  test_module_load.cc:""
  test_module_manager.cc:""
  )

if(esperanto-fw_FOUND)
  get_property(MASTER_MINION_ELF TARGET esperanto-fw::MasterMinion.elf PROPERTY LOCATION)
  get_property(MACHINE_MINION_ELF TARGET esperanto-fw::MachineMinion.elf PROPERTY LOCATION)
  get_property(WORKER_MINION_ELF TARGET esperanto-fw::WorkerMinion.elf PROPERTY LOCATION)

  # Example when passsing arguments then make sure that you use the "=" operation between
  # the option and its value and you space separate the different options.

  set(DEV_FW_ARGS "--fw_type=device-fw --master_minion_elf=${MASTER_MINION_ELF} --machine_minion_elf=${MACHINE_MINION_ELF} --worker_minion_elf=${WORKER_MINION_ELF}")

  list(APPEND TEST_LIST
    test_load_device_fw.cc:${DEV_FW_ARGS}
    test_elf_parsing_dev_fw.cc:${DEV_FW_ARGS}
    )
endif(esperanto-fw_FOUND)

create_test_targets("${TEST_LIST}" "Generic")

set(QEMU_TEST_LIST
  system/test_pcie_device.cc:""
  )

create_test_targets("${QEMU_TEST_LIST}" "QEMU-Driver-SNPS")
