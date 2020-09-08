#------------------------------------------------------------------------------
# Copyright (C) 2020, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

set(DEVAPI_LIB ${ESPERANTO_DEVICE_API_LIB_DIR}/esperanto/device-api)
set(DEVAPI_CODE_GEN ${DEVAPI_LIB}/device_api_codegen.py)

# List of Jinja files to generate files for
SET(JINJA_FILES
  "device-fw-testing-helpers.h.jinja:device-fw-testing-helpers.h"
  )

include_directories(${CMAKE_CURRENT_BINARY_DIR})

foreach(TMPL_TUPLE ${JINJA_FILES})
  string(REPLACE ":" ";" TMPL_LIST ${TMPL_TUPLE})
  list(GET TMPL_LIST 0 TMPL)
  list(GET TMPL_LIST 1 GEN_OUTPUT)
  add_custom_command(
     OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${GEN_OUTPUT}
     COMMAND ${DEVAPI_CODE_GEN}
     ARGS  --spec ${DEVAPI_LIB}/non-privileged-api/device-api-non-privileged.yaml
           --schema ${DEVAPI_LIB}/schema/device-api.schema.json
           --template ${CMAKE_CURRENT_SOURCE_DIR}/${TMPL}
           --output ${CMAKE_CURRENT_BINARY_DIR}/${GEN_OUTPUT}
     DEPENDS ${DEVAPI_CODE_GEN}
             ${CMAKE_CURRENT_SOURCE_DIR}/${TMPL}
             ${DEVAPI_LIB}/non-privileged-api/tracing/device-mrt-trace.yaml
             ${DEVAPI_LIB}/schema/device-api.schema.json
     )
   add_custom_target(${GEN_OUTPUT} ALL
             DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${GEN_OUTPUT})
  list(APPEND GEN_FILES ${CMAKE_CURRENT_BINARY_DIR}/${GEN_OUTPUT})
endforeach()

add_custom_target(gen-devicefw-helpers
  DEPENDS ${GEN_FILES}
  )

# List of tests that are currently running only in the simulator
set(SIM_DEVFW_TEST_LIST
  test_mb_sim_api.cc:""
  test_load_device_fw.cc:${DEV_FW_ARGS}
  test_elf_parsing_dev_fw.cc:"${DEV_FW_ARGS} --empty_elf ${EMPTY_KERNEL_ELF}"
  test_mailbox.cc:"${DEV_FW_ARGS}"
  test_sysemu_launch_kernels.cc:"${DEV_FW_ARGS} --kernels_dir ${KERNELS_DIR}"
  test_device_api_sysemu.cc:"${DEV_FW_ARGS} --kernels_dir ${KERNELS_DIR}"
  test_streams.cc:"${DEV_FW_ARGS} --kernels_dir ${KERNELS_DIR}"
  )
create_test_targets("${SIM_DEVFW_TEST_LIST}" "LABELS;Generic;LABELS;DeviceFW;TIMEOUT;120")

# List of tests that run either in the simulator or in PCIE
set(DUAL_MODE_OPTS "${DEV_FW_ARGS}")
if(ENABLE_PCIE)
  set(DUAL_MODE_OPTS "${DUAL_MODE_OPTS} --dev_target=pcie")
  set(DUAL_TEST_LABELS "LABELS;QEMU-Driver-SNPS;TIMEOUT;600")
else()
  set(DUAL_TEST_LABELS "LABELS;Generic;LABELS;DeviceFW;TIMEOUT;120")
endif(ENABLE_PCIE)

set(DUAL_TEST_LIST
  # FIXME SW-4087
  #test_device_tracing.cc:"${DUAL_MODE_OPTS} --kernels_dir ${KERNELS_DIR}"
  test_device_api.cc:"${DUAL_MODE_OPTS} --kernels_dir ${KERNELS_DIR}"
  test_launch_uberkernel.cc:"${DUAL_MODE_OPTS} --kernels_dir ${KERNELS_DIR}"
  test_reflect_cmd.cc:"${DUAL_MODE_OPTS}"
  # FIXME SW-3254
  # test_hang.cc:"${DEV_FW_ARGS} --kernels_dir ${KERNELS_DIR}"
  )
create_test_targets("${DUAL_TEST_LIST}" "${DUAL_TEST_LABELS}")
