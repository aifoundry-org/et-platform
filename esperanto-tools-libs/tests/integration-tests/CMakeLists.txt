#------------------------------------------------------------------------------
# Copyright (C) 2019, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

set(INTEGRATION_TEST_LIST
  test_code_loading.cpp:""
  test_fwtraces.cpp:""
  test_memcpy.cpp:""
  test_profiler.cpp:""
  test_device_errors.cpp:""
  test_dma_errors.cpp:""
  )
create_test_targets("${INTEGRATION_TEST_LIST}" "LABELS;Generic;LABELS;Sysemu;TIMEOUT;300" "it_")


set(PCIE_TEST_LIST
  test_code_loading.cpp:"--mode=pcie"
  test_fwtraces.cpp:"--mode=pcie"
  test_memcpy.cpp:"--mode=pcie"
  test_device_errors.cpp:"--mode=pcie"
  test_dma_errors.cpp:"--mode=pcie"
  test_abort.cpp:"--mode=pcie"
  )

create_test_targets("${PCIE_TEST_LIST}" "LABELS;Pcie;TIMEOUT;300" "pcie_")


configure_file(invoke-pcie-loopback-tests.sh.in ${CMAKE_CURRENT_BINARY_DIR}/tests/invoke-pcie-loopback-tests.sh)