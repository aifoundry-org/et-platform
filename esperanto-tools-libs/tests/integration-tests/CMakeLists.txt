#------------------------------------------------------------------------------
# Copyright (C) 2019, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

set(DEV_FW_ARGS "--master_minion_elf=${MASTER_MINION_ELF} --machine_minion_elf=${MACHINE_MINION_ELF} --worker_minion_elf=${WORKER_MINION_ELF}")

set(TEST_LIST
  test_device_target_sysemu_grpc.cc:""
  test_tracing.cc:""
  )

create_test_targets("${TEST_LIST}" "LABELS;Generic;TIMEOUT;600")

# List of tests to be compiled if we have support for device-fw
# List of tests that are currently running only in the simulator
set(SIM_DEVFW_TEST_LIST
  test_code_manager.cc:"${DEV_FW_ARGS}"
  test_code_loading.cpp:"${DEV_FW_ARGS}"
  test_module_load.cc:"${DEV_FW_ARGS} --kernels_dir ${KERNELS_DIR}"
  test_module_manager.cc:"${DEV_FW_ARGS}"
  )
create_test_targets("${SIM_DEVFW_TEST_LIST}" "LABELS;Generic;LABELS;DeviceFW;TIMEOUT;120")


add_subdirectory(device-fw)
add_subdirectory(DeviceManagement)
add_subdirectory(pcie)
