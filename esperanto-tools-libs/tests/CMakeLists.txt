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

set(CI_DEPENDENCIES convolution.elf
                    bootrom.mem
                    gpu_0_softmax_110.elf
                    MachineMinion/MachineMinion.elf
                    MasterMinion/MasterMinion.elf
                    WorkerMinion/WorkerMinion.elf
  )
set(CI_DN_TARGETS "")

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
  etrt_static
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

set(TEST_LIST test_error_or.cc
  test_command_response.cc
  test_device_manager.cc
  test_device_target_card_proxy.cc
  test_device_target_sysemu_grpc.cc
  test_elf_parsing.cc
  test_load_device_fw.cc
  test_module_load.cc
  test_module_manager.cc
  )


foreach(TEST_FILE ${TEST_LIST})
  get_filename_component(TEST_TARGET ${TEST_FILE} NAME_WE)
  add_executable(${TEST_TARGET} ${TEST_FILE})
  add_dependencies(${TEST_TARGET} googletest)
  add_dependencies(${TEST_TARGET} ${external_project_dependencies})
  add_dependencies(${TEST_TARGET} ci-dependencies)
  target_link_libraries(${TEST_TARGET} ${COMMON_TEST_LIBRARIES})
  target_include_directories(${TEST_TARGET} PUBLIC ${GFLAGS_INCLUDE_DIRS})
  target_include_directories(${TEST_TARGET} PUBLIC ${GLOG_INCLUDE_DIRS})
  gtest_discover_tests(${TEST_TARGET} TEST_PREFIX runtime:)

endforeach(TEST_FILE)
