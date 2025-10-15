# Copyright (c) 2025 Ainekko, Co.
# SPDX-License-Identifier: Apache-2.0

set(TEST_LIST
  stress_mem.cpp:""
  stress_kernel.cpp:""
  launchKernel1M.cpp:""
  )
  
set(PCIE_TEST_LIST
  stress_mem.cpp:"--mode=pcie"
  stress_kernel.cpp:"--mode=pcie"
  launchKernel1M.cpp:"--mode=pcie"
  )

set(MP_SYSEMU_TEST_LIST
  stress_mem.cpp:"--mp --mode=sysemu"
  stress_kernel.cpp:"--mp --mode=sysemu"
  launchKernel1M.cpp:"--mp --mode=sysemu"
  #latency_mp_vs_sp.cpp:"" this test was used only for debugging SW-16519, to be removed (or add support for easyprofiler in tests directly)
)

set(MP_PCIE_TEST_LIST
  stress_mem.cpp:"--mp --mode=pcie"
  stress_kernel.cpp:"--mp --mode=pcie"
  launchKernel1M.cpp:"--mp --mode=pcie"
)
create_test_targets("${TEST_LIST}" "LABELS;Stress;LABELS;Generic;LABELS;Sysemu;TIMEOUT;300" "st_")

create_test_targets("${MP_SYSEMU_TEST_LIST}" "LABELS;Sysemu;LABELS;Generic;LABELS;Stress;LABELS;MP;TIMEOUT;300" "st_mp_")

create_test_targets("${PCIE_TEST_LIST}" "LABELS;Pcie;LABELS;Stress;TIMEOUT;300" "pcie_st_")

create_test_targets("${MP_PCIE_TEST_LIST}" "LABELS;Pcie;LABELS;Stress;LABELS;MP;TIMEOUT;300" "pcie_st_mp_")
