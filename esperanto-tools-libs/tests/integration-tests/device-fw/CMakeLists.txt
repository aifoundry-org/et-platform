#------------------------------------------------------------------------------
# Copyright (C) 2020, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

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
  test_device_api.cc:"${DUAL_MODE_OPTS} --kernels_dir ${KERNELS_DIR}"
  test_launch_uberkernel.cc:"${DUAL_MODE_OPTS} --kernels_dir ${KERNELS_DIR}"
  test_hang.cc:"${DEV_FW_ARGS} --kernels_dir ${KERNELS_DIR}"
  )
create_test_targets("${DUAL_TEST_LIST}" "${DUAL_TEST_LABELS}")
