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
  test_code_loading.cpp:"--kernels_dir ${KERNELS_DIR}"
  test_memcpy.cpp:""
  test_profiler.cpp:"--kernels_dir ${KERNELS_DIR}"
  )
create_test_targets("${SIM_DEVFW_TEST_LIST}" "LABELS;Generic;LABELS;SYSEMU;TIMEOUT;120")
