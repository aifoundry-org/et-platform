//******************************************************************************
// Copyright (C) 2018,2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "device-fw-fixture.h"

#include "esperanto/runtime/Core/DeviceHelpers.h"
#include "esperanto/runtime/Core/TraceHelper.h"
#include "esperanto/runtime/Core/VersionCheckers.h"

#include <thread>
#include <chrono>

ABSL_FLAG(std::string, kernels_dir, "",
          "Directory where different kernel ELF files are located");

using namespace et_runtime;

TEST_F(DeviceFWTest, DeviceAPI_GetDevFWVersion) {
  GitVersionChecker git_checker (*dev_);
  auto devfw_hash = git_checker.deviceFWHash();
  ASSERT_TRUE(devfw_hash != 0);
}

// Test the call that retries the DevceAPI version from the Device
TEST_F(DeviceFWTest, DeviceAPI_GetDevAPIVersion) {
  DeviceAPIChecker devapi_checker(*dev_);
  auto success = devapi_checker.getDeviceAPIVersion();
  ASSERT_TRUE(success);
  // Require that we always build the runtime with a target device-fw
  // that the runtime can support
  success = devapi_checker.isDeviceSupported();
  ASSERT_TRUE(success);
}

// Test setting the master minion logging levels
TEST_F(DeviceFWTest, DeviceAPI_SetLogLevelMaster) {
  SetMasterLogLevel master_level(*dev_);
  auto success = master_level.set_level_critical();
  ASSERT_TRUE(success);
  success = master_level.set_level_error();
  ASSERT_TRUE(success);
  success = master_level.set_level_warning();
  ASSERT_TRUE(success);
  success = master_level.set_level_info();
  ASSERT_TRUE(success);
  success = master_level.set_level_debug();
  ASSERT_TRUE(success);
  success = master_level.set_level_trace();
  ASSERT_TRUE(success);
}

// Test setting the logging levels
TEST_F(DeviceFWTest, DeviceAPI_ConfigureTraccLogLevelKnob) {
  TraceHelper trace_helper(*dev_);
  bool success;

  success = trace_helper.set_level_critical();
  ASSERT_TRUE(success);
  success = trace_helper.set_level_error();
  ASSERT_TRUE(success);
  success = trace_helper.set_level_warning();
  ASSERT_TRUE(success);
  success = trace_helper.set_level_info();
  ASSERT_TRUE(success);
  success = trace_helper.set_level_debug();
  ASSERT_TRUE(success);
  success = trace_helper.set_level_trace();
  ASSERT_TRUE(success);
}

// Test enable/disable feature of the trace groups
TEST_F(DeviceFWTest, DeviceAPI_ConfigureTraceGroupKnobs) {
  TraceHelper trace_helper(*dev_);
  bool success;

  for (::device_api::non_privileged::trace_groups_e i =
           ::device_api::non_privileged::TRACE_GROUP_ID_NONE + 1;
       i < ::device_api::non_privileged::TRACE_GROUP_ID_LAST; i++) {
    success = trace_helper.configure_trace_group_knob(i, 0);
    ASSERT_TRUE(success);
    success = trace_helper.configure_trace_group_knob(i, 1);
    ASSERT_TRUE(success);
  }
}

// Test enable/disable feature of trace event
TEST_F(DeviceFWTest, DeviceAPI_ConfigureTraceEventKnobs) {
  TraceHelper trace_helper(*dev_);
  bool success;

  for (::device_api::non_privileged::trace_events_e i =
           ::device_api::non_privileged::TRACE_EVENT_ID_NONE + 1;
       i < ::device_api::non_privileged::TRACE_EVENT_ID_LAST; i++) {
    success = trace_helper.configure_trace_event_knob(i, 0);
    ASSERT_TRUE(success);
    success = trace_helper.configure_trace_event_knob(i, 1);
    ASSERT_TRUE(success);
  }
}

// Test trace buffer size configuration feature
TEST_F(DeviceFWTest, DeviceAPI_ConfigureTraceBufferSizeKnob) {
  TraceHelper trace_helper(*dev_);

  auto success = trace_helper.configure_trace_buffer_size_knob(8192);
  ASSERT_TRUE(success);
}

// Test enable/disable feature of trace subsysem
TEST_F(DeviceFWTest, DeviceAPI_ConfigureTraceStateKnob) {
  TraceHelper trace_helper(*dev_);
  bool success;

  success = trace_helper.configure_trace_state_knob(0);
  ASSERT_TRUE(success);
  success = trace_helper.configure_trace_state_knob(1);
  ASSERT_TRUE(success);
}

// Test enable/disable feature of trace logging on uart interface
TEST_F(DeviceFWTest, DeviceAPI_ConfigureTraceUartLoggingKnob) {
  TraceHelper trace_helper(*dev_);
  bool success;

  success = trace_helper.configure_trace_uart_logging_knob(0);
  ASSERT_TRUE(success);
  success = trace_helper.configure_trace_uart_logging_knob(1);
  ASSERT_TRUE(success);
}

int main(int argc, char **argv) {
  google::InitGoogleLogging(argv[0]);
  google::InstallFailureSignalHandler();
  google::SetCommandLineOption("GLOG_minloglevel", "0");
  FLAGS_minloglevel = 0;
  FLAGS_logtostderr = 1;
  testing::InitGoogleTest(&argc, argv);
  et_runtime::ParseCommandLineOptions(argc, argv);
  return RUN_ALL_TESTS();
}
