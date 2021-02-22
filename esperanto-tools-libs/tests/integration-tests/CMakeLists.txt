#------------------------------------------------------------------------------
# Copyright (C) 2019, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

set(SIM_DEVFW_TEST_LIST
  test_code_loading.cpp:"${DEV_FW_ARGS}"
  test_memcpy.cpp:"${DEV_FW_ARGS}"
  test_profiler.cpp:"${DUAL_MODE_OPTS} --kernels_dir ${KERNELS_DIR}"
  )
create_test_targets("${SIM_DEVFW_TEST_LIST}" "LABELS;Generic;LABELS;DeviceFW;TIMEOUT;120")
