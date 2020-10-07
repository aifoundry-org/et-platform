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
  test_command_response.cc:""
  test_command_queue.cc:""
  test_data_transfers.cc:""
  test_device_manager.cc:""
  test_error_or.cc:""
  test_elf_parsing.cc:""
  utest_memory_manager.cpp:""
  )

create_test_targets("${TEST_LIST}" "LABELS;Generic;TIMEOUT;600")

add_subdirectory(memory_management)
