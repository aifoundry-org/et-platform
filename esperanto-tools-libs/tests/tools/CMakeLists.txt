# Copyright (c) 2025 Ainekko, Co.
# SPDX-License-Identifier: Apache-2.0

set(TEST_LIST
  benchmarkDeviceLayerFake.cpp:""
  benchmarkDeviceLayerSysEmu.cpp:""
)

create_test_targets("${TEST_LIST}" "LABELS;Generic;LABELS;Unittest;TIMEOUT;120" "ut_")


foreach(TARGET ut_benchmarkDeviceLayerSysEmu;ut_benchmarkDeviceLayerFake)
  target_link_libraries(${TARGET}
  PRIVATE
    runtimeTools::benchmarker
  )
endforeach(TARGET)