#------------------------------------------------------------------------------
# Copyright (C) 2020, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

# DM only tests
set(DM_TEST_LIST
  test_device_management.cc:""
  )

set(COMMON_TEST_LIBRARIES

   -Wl,--whole-archive etrt -Wl,--no-whole-archive
  libgtest_main.a
  libgtest.a
  libgmock.a
  libbreakpad_client.a
  ${CMAKE_DL_LIBS}
  )

create_test_targets("${DM_TEST_LIST}" "LABELS;QEMU-Driver-SNPS;TIMEOUT;600")
