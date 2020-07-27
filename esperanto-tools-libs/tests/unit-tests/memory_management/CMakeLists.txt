#------------------------------------------------------------------------------
# Copyright (C) 2020, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

set(TEST_LIST
  test_device_ptr.cc:""
  test_tensor_info.cc:""
  test_linear_allocator.cc:""
  test_bidirectional_allocator.cc:""
  test_memory_manager.cc:""
  test_memory_manager_internals.cc:""
  )

create_test_targets("${TEST_LIST}" "LABELS;Generic;TIMEOUT;600")
