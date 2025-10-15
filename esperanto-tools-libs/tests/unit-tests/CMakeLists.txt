# Copyright (c) 2025 Ainekko, Co.
# SPDX-License-Identifier: Apache-2.0

set(TEST_LIST
  test_memory_manager.cpp:""
  test_streams_lifecycle.cpp:""
  test_event_manager.cpp:""
  test_devices.cpp:"" 
  benchmark.cpp:""
  other.cpp:""
  KernelLaunchFake.cpp:""
  TestCommandSender.cpp:""
  initRuntime.cpp:""
  test_spinlock.cpp:""
  test_KernelLaunchOptionsAPI.cpp:""  
)

set(TEST_LIST_MP
  mp_handshake.cpp:""
  mp_malloc.cpp:""
  mp_streams.cpp:""
  mp_sync_events.cpp:""
  mp_monitor.cpp:""
)

create_test_targets("${TEST_LIST}" "LABELS;Generic;LABELS;Sysemu;LABELS;UT;TIMEOUT;300" "ut_")

create_test_targets("${TEST_LIST_MP}" "LABELS;Generic;LABELS;Sysemu;LABELS;UT;LABELS;MP;TIMEOUT;300" "ut_")
