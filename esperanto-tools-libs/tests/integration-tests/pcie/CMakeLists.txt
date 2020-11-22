#------------------------------------------------------------------------------
# Copyright (C) 2020, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

# PCIE only tests
set(QEMU_TEST_LIST
  test_pcie_device.cc:""
  test_pcie_device_mgmt.cc:""
  test_pcie_launch_kernels.cc:"${DEV_FW_ARGS} --kernels_dir ${KERNELS_DIR}"
  )

create_test_targets("${QEMU_TEST_LIST}" "LABELS;QEMU-Driver-SNPS;TIMEOUT;600")
