#------------------------------------------------------------------------------
# Copyright (C) 2019, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

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