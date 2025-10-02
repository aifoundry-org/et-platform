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
  test_memcpy.cpp:""
  test_device_errors.cpp:""
  test_dma_errors.cpp:""
  test_stack.cpp:""
  test_stack_death.cpp:""
  )
create_test_targets("${INTEGRATION_TEST_LIST}" "LABELS;Generic;LABELS;Sysemu;TIMEOUT;300" "it_")


set(PCIE_TEST_LIST
  test_code_loading.cpp:"--mode=pcie"
  test_memcpy.cpp:"--mode=pcie"
  test_device_errors.cpp:"--mode=pcie"
  test_dma_errors.cpp:"--mode=pcie"
  test_abort.cpp:"--mode=pcie"  
  )

set(MP_SYSEMU_TEST_LIST
test_memcpy.cpp:"--mp --mode=sysemu"
test_code_loading.cpp:"--mp --mode=sysemu"
test_device_errors.cpp:"--mp --mode=sysemu"
mp_memcpy.cpp:""
test_stack.cpp:"-mp --mode=sysemu"
#test_dma_errors.cpp:"--mp --mode=sysemu" cant be in MP since we can not access the underlying runtime.
)

set(MP_PCIE_TEST_LIST
test_memcpy.cpp:"--mp --mode=pcie"
test_code_loading.cpp:"--mp --mode=pcie"
test_device_errors.cpp:"--mp --mode=pcie"
#test_dma_errors.cpp:"--mp --mode=pcie" cant be in MP since we can not access the underlying runtime.
)

create_test_targets("${PCIE_TEST_LIST}" "LABELS;Pcie;TIMEOUT;300" "pcie_")

create_test_targets("${PCIE_TEST_LIST}" "LABELS;Pcie;LABELS;MP;TIMEOUT;300" "pcie_mp_")

create_test_targets("${MP_SYSEMU_TEST_LIST}" "LABELS;Generic;LABELS;Sysemu;LABELS;MP;TIMEOUT;300" "it_mp_")


configure_file(invoke-pcie-tests.sh.in ${CMAKE_CURRENT_BINARY_DIR}/tests/invoke-pcie-tests.sh)
