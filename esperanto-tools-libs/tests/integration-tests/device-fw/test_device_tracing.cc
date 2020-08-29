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

#include "Tracing/Tracing.h"
#include "device-fw-testing-helpers.h"
#include "esperanto/runtime/CodeManagement/CodeRegistry.h"
#include "esperanto/runtime/Core/TraceHelper.h"

using namespace et_runtime::device;
using namespace et_runtime::device_api;
using namespace et_runtime;
namespace fs = std::experimental::filesystem;

ABSL_FLAG(std::string, kernels_dir, "",
          "Directory where different kernel ELF files are located");

TEST_F(DeviceFWTest, Trace_test) {
  auto kernels_dir = absl::GetFlag(FLAGS_kernels_dir);
  fs::path trace_kernel = fs::path(kernels_dir)  / fs::path("trace.elf");
  auto &registry = dev_->codeRegistry();

  auto register_res = registry.registerKernel(
      "main", {Kernel::ArgType::T_layer_dynamic_info}, trace_kernel.string());
  ASSERT_TRUE((bool)register_res);
  auto &kernel = std::get<1>(register_res.get());

  auto load_res = registry.moduleLoad(kernel.moduleID(), dev_.get());
  ASSERT_EQ(load_res.getError(), etrtSuccess);
  auto module_id = load_res.get();

  Kernel::layer_dynamic_info_t layer_info = {0};
  Kernel::LaunchArg arg;
  arg.type = Kernel::ArgType::T_layer_dynamic_info;
  arg.value.layer_dynamic_info = layer_info;
  auto args = std::vector<Kernel::LaunchArg>({arg});
  auto launch = kernel.createKernelLaunch(args);
  auto launch_res = launch->launchBlocking(&dev_->defaultStream());

  ASSERT_EQ(launch_res, etrtSuccess);

  TraceHelper trace_helper(*dev_);
  ::device_api::non_privileged::discover_trace_buffer_rsp_t rsp = {0};
  rsp = trace_helper.discover_trace_buffer();
  ASSERT_TRUE(rsp.status);

  auto success = trace_helper.configure_trace_state_knob(0);
  ASSERT_TRUE(success);

  success = trace_helper.prepare_trace_buffers();
  ASSERT_TRUE(success);

  std::vector<uint8_t> data(rsp.trace_buffer_size * NUMBER_OF_TRACE_BUFFERS);
  auto res = dev_->memcpy(
      data.data(),
      (const void *)(rsp.trace_base +
                     ALIGN(
                         sizeof(::device_api::non_privileged::trace_control_t),
                         TRACE_BUFFER_REGION_ALIGNEMNT)),
      rsp.trace_buffer_size * NUMBER_OF_TRACE_BUFFERS, etrtMemcpyDeviceToHost);

  auto result = et_runtime::tracing::DeviceAPI_DeviceFW_process_device_traces(
      &data[0], rsp.trace_buffer_size, NUMBER_OF_TRACE_BUFFERS);

  ASSERT_EQ(result, ::device_api::non_privileged::TRACE_STATUS_SUCCESS);
}

TEST_F(DeviceFWTest, Trace_TestTraceLogLevelKnob) {
  auto kernels_dir = absl::GetFlag(FLAGS_kernels_dir);
  fs::path trace_kernel = fs::path(kernels_dir) / fs::path("trace.elf");
  auto &registry = dev_->codeRegistry();

  auto register_res = registry.registerKernel(
      "main", {Kernel::ArgType::T_layer_dynamic_info}, trace_kernel.string());
  ASSERT_TRUE((bool)register_res);
  auto &kernel = std::get<1>(register_res.get());

  auto load_res = registry.moduleLoad(kernel.moduleID(), dev_.get());
  ASSERT_EQ(load_res.getError(), etrtSuccess);
  auto module_id = load_res.get();

  TraceHelper trace_helper(*dev_);
  auto success = trace_helper.set_level_critical();
  ASSERT_TRUE(success);

  // Consume the trace buffers before launching the kernel
  ::device_api::non_privileged::discover_trace_buffer_rsp_t rsp = {0};
  rsp = trace_helper.discover_trace_buffer();
  ASSERT_TRUE(rsp.status);

  success = trace_helper.configure_trace_state_knob(0);
  ASSERT_TRUE(success);

  success = trace_helper.prepare_trace_buffers();
  ASSERT_TRUE(success);

  success = trace_helper.reset_trace_buffers();
  ASSERT_TRUE(success);

  success = trace_helper.configure_trace_state_knob(1);
  ASSERT_TRUE(success);

  Kernel::layer_dynamic_info_t layer_info = {0};
  Kernel::LaunchArg arg;
  arg.type = Kernel::ArgType::T_layer_dynamic_info;
  arg.value.layer_dynamic_info = layer_info;
  auto args = std::vector<Kernel::LaunchArg>({arg});
  auto launch = kernel.createKernelLaunch(args);
  auto launch_res = launch->launchBlocking(&dev_->defaultStream());

  ASSERT_EQ(launch_res, etrtSuccess);

  rsp = trace_helper.discover_trace_buffer();
  ASSERT_TRUE(rsp.status);

  success = trace_helper.configure_trace_state_knob(0);
  ASSERT_TRUE(success);

  success = trace_helper.prepare_trace_buffers();
  ASSERT_TRUE(success);

  std::vector<uint8_t> data(rsp.trace_buffer_size * NUMBER_OF_TRACE_BUFFERS);
  auto res = dev_->memcpy(
      data.data(),
      (const void *)(rsp.trace_base +
                     ALIGN(
                         sizeof(::device_api::non_privileged::trace_control_t),
                         TRACE_BUFFER_REGION_ALIGNEMNT)),
      rsp.trace_buffer_size * NUMBER_OF_TRACE_BUFFERS, etrtMemcpyDeviceToHost);

  auto result = et_runtime::tracing::DeviceAPI_DeviceFW_process_device_traces(
      &data[0], rsp.trace_buffer_size, NUMBER_OF_TRACE_BUFFERS);

  ASSERT_EQ(result, ::device_api::non_privileged::TRACE_STATUS_SUCCESS);

  // Verify if any events are logged below the threshold set
  result = Test_Trace_verify_loglevel_disable(
      &data[0], rsp.trace_buffer_size, NUMBER_OF_TRACE_BUFFERS,
      ::device_api::non_privileged::LOG_LEVELS_CRITICAL);

  ASSERT_TRUE(result);
}

TEST_F(DeviceFWTest, Trace_TestTraceGroupKnobs) {
  auto kernels_dir = absl::GetFlag(FLAGS_kernels_dir);
  fs::path trace_kernel = fs::path(kernels_dir) / fs::path("trace.elf");
  auto &registry = dev_->codeRegistry();

  auto register_res = registry.registerKernel(
      "main", {Kernel::ArgType::T_layer_dynamic_info}, trace_kernel.string());
  ASSERT_TRUE((bool)register_res);
  auto &kernel = std::get<1>(register_res.get());

  auto load_res = registry.moduleLoad(kernel.moduleID(), dev_.get());
  ASSERT_EQ(load_res.getError(), etrtSuccess);
  auto module_id = load_res.get();

  TraceHelper trace_helper(*dev_);

  // Disable Trace group
  auto success = trace_helper.configure_trace_group_knob(
      ::device_api::non_privileged::TRACE_GROUP_ID_PERFORMANCE, 0);
  ASSERT_TRUE(success);

  // Consume the trace buffers before launching the kernel
  ::device_api::non_privileged::discover_trace_buffer_rsp_t rsp = {0};
  rsp = trace_helper.discover_trace_buffer();
  ASSERT_TRUE(rsp.status);

  success = trace_helper.configure_trace_state_knob(0);
  ASSERT_TRUE(success);

  success = trace_helper.prepare_trace_buffers();
  ASSERT_TRUE(success);

  success = trace_helper.reset_trace_buffers();
  ASSERT_TRUE(success);

  success = trace_helper.configure_trace_state_knob(1);
  ASSERT_TRUE(success);

  Kernel::layer_dynamic_info_t layer_info = {0};
  Kernel::LaunchArg arg;
  arg.type = Kernel::ArgType::T_layer_dynamic_info;
  arg.value.layer_dynamic_info = layer_info;
  auto args = std::vector<Kernel::LaunchArg>({arg});
  auto launch = kernel.createKernelLaunch(args);
  auto launch_res = launch->launchBlocking(&dev_->defaultStream());

  ASSERT_EQ(launch_res, etrtSuccess);

  rsp = trace_helper.discover_trace_buffer();
  ASSERT_TRUE(rsp.status);

  success = trace_helper.configure_trace_state_knob(0);
  ASSERT_TRUE(success);

  success = trace_helper.prepare_trace_buffers();
  ASSERT_TRUE(success);

  std::vector<uint8_t> data(rsp.trace_buffer_size * NUMBER_OF_TRACE_BUFFERS);
  auto res = dev_->memcpy(
      data.data(),
      (const void *)(rsp.trace_base +
                     ALIGN(
                         sizeof(::device_api::non_privileged::trace_control_t),
                         TRACE_BUFFER_REGION_ALIGNEMNT)),
      rsp.trace_buffer_size * NUMBER_OF_TRACE_BUFFERS, etrtMemcpyDeviceToHost);

  auto result = et_runtime::tracing::DeviceAPI_DeviceFW_process_device_traces(
      &data[0], rsp.trace_buffer_size, NUMBER_OF_TRACE_BUFFERS);

  ASSERT_EQ(result, ::device_api::non_privileged::TRACE_STATUS_SUCCESS);

  // Verify specified group events are disabled
  result = Test_Trace_verify_group_disable(
      &data[0], rsp.trace_buffer_size, NUMBER_OF_TRACE_BUFFERS,
      ::device_api::non_privileged::TRACE_GROUP_ID_PERFORMANCE);

  ASSERT_TRUE(result);
}

TEST_F(DeviceFWTest, Trace_TestTraceEventKnobs) {
  auto kernels_dir = absl::GetFlag(FLAGS_kernels_dir);
  fs::path trace_kernel = fs::path(kernels_dir) / fs::path("trace.elf");
  auto &registry = dev_->codeRegistry();

  auto register_res = registry.registerKernel(
      "main", {Kernel::ArgType::T_layer_dynamic_info}, trace_kernel.string());
  ASSERT_TRUE((bool)register_res);
  auto &kernel = std::get<1>(register_res.get());

  auto load_res = registry.moduleLoad(kernel.moduleID(), dev_.get());
  ASSERT_EQ(load_res.getError(), etrtSuccess);
  auto module_id = load_res.get();

  TraceHelper trace_helper(*dev_);

  // Disable the event
  auto success = trace_helper.configure_trace_event_knob(
      ::device_api::non_privileged::TRACE_EVENT_ID_PERFORMANCE_PERFCTR, 0);
  ASSERT_TRUE(success);

  // Consume the trace buffers before launching the kernel
  ::device_api::non_privileged::discover_trace_buffer_rsp_t rsp = {0};
  rsp = trace_helper.discover_trace_buffer();
  ASSERT_TRUE(rsp.status);

  success = trace_helper.configure_trace_state_knob(0);
  ASSERT_TRUE(success);

  success = trace_helper.prepare_trace_buffers();
  ASSERT_TRUE(success);

  success = trace_helper.reset_trace_buffers();
  ASSERT_TRUE(success);

  success = trace_helper.configure_trace_state_knob(1);
  ASSERT_TRUE(success);

  Kernel::layer_dynamic_info_t layer_info = {0};
  Kernel::LaunchArg arg;
  arg.type = Kernel::ArgType::T_layer_dynamic_info;
  arg.value.layer_dynamic_info = layer_info;
  auto args = std::vector<Kernel::LaunchArg>({arg});
  auto launch = kernel.createKernelLaunch(args);
  auto launch_res = launch->launchBlocking(&dev_->defaultStream());

  ASSERT_EQ(launch_res, etrtSuccess);

  rsp = trace_helper.discover_trace_buffer();
  ASSERT_TRUE(rsp.status);

  success = trace_helper.configure_trace_state_knob(0);
  ASSERT_TRUE(success);

  success = trace_helper.prepare_trace_buffers();
  ASSERT_TRUE(success);

  std::vector<uint8_t> data(rsp.trace_buffer_size * NUMBER_OF_TRACE_BUFFERS);
  auto res = dev_->memcpy(
      data.data(),
      (const void *)(rsp.trace_base +
                     ALIGN(
                         sizeof(::device_api::non_privileged::trace_control_t),
                         TRACE_BUFFER_REGION_ALIGNEMNT)),
      rsp.trace_buffer_size * NUMBER_OF_TRACE_BUFFERS, etrtMemcpyDeviceToHost);

  auto result = et_runtime::tracing::DeviceAPI_DeviceFW_process_device_traces(
      &data[0], rsp.trace_buffer_size, NUMBER_OF_TRACE_BUFFERS);

  ASSERT_EQ(result, ::device_api::non_privileged::TRACE_STATUS_SUCCESS);

  // Verify specified event is disabled
  result = Test_Trace_verify_event_disable(
      &data[0], rsp.trace_buffer_size, NUMBER_OF_TRACE_BUFFERS,
      ::device_api::non_privileged::TRACE_EVENT_ID_PERFORMANCE_PERFCTR);

  ASSERT_TRUE(result);
}

TEST_F(DeviceFWTest, Trace_TestTraceStateKnob) {
  auto kernels_dir = absl::GetFlag(FLAGS_kernels_dir);
  fs::path trace_kernel = fs::path(kernels_dir) / fs::path("trace.elf");
  auto &registry = dev_->codeRegistry();

  auto register_res = registry.registerKernel(
      "main", {Kernel::ArgType::T_layer_dynamic_info}, trace_kernel.string());
  ASSERT_TRUE((bool)register_res);
  auto &kernel = std::get<1>(register_res.get());

  auto load_res = registry.moduleLoad(kernel.moduleID(), dev_.get());
  ASSERT_EQ(load_res.getError(), etrtSuccess);
  auto module_id = load_res.get();

  TraceHelper trace_helper(*dev_);

  // Consume the trace buffers before launching the kernel
  ::device_api::non_privileged::discover_trace_buffer_rsp_t rsp = {0};
  rsp = trace_helper.discover_trace_buffer();
  ASSERT_TRUE(rsp.status);

  // Disable trace logging
  auto success = trace_helper.configure_trace_state_knob(0);
  ASSERT_TRUE(success);

  success = trace_helper.prepare_trace_buffers();
  ASSERT_TRUE(success);

  success = trace_helper.reset_trace_buffers();
  ASSERT_TRUE(success);

  Kernel::layer_dynamic_info_t layer_info = {0};
  Kernel::LaunchArg arg;
  arg.type = Kernel::ArgType::T_layer_dynamic_info;
  arg.value.layer_dynamic_info = layer_info;
  auto args = std::vector<Kernel::LaunchArg>({arg});
  auto launch = kernel.createKernelLaunch(args);
  auto launch_res = launch->launchBlocking(&dev_->defaultStream());

  ASSERT_EQ(launch_res, etrtSuccess);

  rsp = trace_helper.discover_trace_buffer();
  ASSERT_TRUE(rsp.status);

  success = trace_helper.prepare_trace_buffers();
  ASSERT_TRUE(success);

  std::vector<uint8_t> data(rsp.trace_buffer_size * NUMBER_OF_TRACE_BUFFERS);
  auto res = dev_->memcpy(
      data.data(),
      (const void *)(rsp.trace_base +
                     ALIGN(
                         sizeof(::device_api::non_privileged::trace_control_t),
                         TRACE_BUFFER_REGION_ALIGNEMNT)),
      rsp.trace_buffer_size * NUMBER_OF_TRACE_BUFFERS, etrtMemcpyDeviceToHost);

  auto result = et_runtime::tracing::DeviceAPI_DeviceFW_process_device_traces(
      &data[0], rsp.trace_buffer_size, NUMBER_OF_TRACE_BUFFERS);

  ASSERT_EQ(result, ::device_api::non_privileged::TRACE_STATUS_SUCCESS);

  // Verify if trace logging is disabled
  result = Test_Trace_verify_state_disable(&data[0], rsp.trace_buffer_size,
                                           NUMBER_OF_TRACE_BUFFERS);

  ASSERT_TRUE(result);
}

TEST_F(DeviceFWTest, Trace_TestTraceUartLoggingKnob) {
  auto kernels_dir = absl::GetFlag(FLAGS_kernels_dir);
  fs::path trace_kernel = fs::path(kernels_dir) / fs::path("trace.elf");
  auto &registry = dev_->codeRegistry();

  auto register_res = registry.registerKernel(
      "main", {Kernel::ArgType::T_layer_dynamic_info}, trace_kernel.string());
  ASSERT_TRUE((bool)register_res);
  auto &kernel = std::get<1>(register_res.get());

  auto load_res = registry.moduleLoad(kernel.moduleID(), dev_.get());
  ASSERT_EQ(load_res.getError(), etrtSuccess);
  auto module_id = load_res.get();

  TraceHelper trace_helper(*dev_);
  auto success = trace_helper.configure_trace_uart_logging_knob(1);
  ASSERT_TRUE(success);

  // Consume the trace buffers before launching the kernel
  ::device_api::non_privileged::discover_trace_buffer_rsp_t rsp = {0};
  rsp = trace_helper.discover_trace_buffer();
  ASSERT_TRUE(rsp.status);

  success = trace_helper.configure_trace_state_knob(0);
  ASSERT_TRUE(success);

  success = trace_helper.prepare_trace_buffers();
  ASSERT_TRUE(success);

  success = trace_helper.reset_trace_buffers();
  ASSERT_TRUE(success);

  success = trace_helper.configure_trace_state_knob(1);
  ASSERT_TRUE(success);

  Kernel::layer_dynamic_info_t layer_info = {0};
  Kernel::LaunchArg arg;
  arg.type = Kernel::ArgType::T_layer_dynamic_info;
  arg.value.layer_dynamic_info = layer_info;
  auto args = std::vector<Kernel::LaunchArg>({arg});
  auto launch = kernel.createKernelLaunch(args);
  auto launch_res = launch->launchBlocking(&dev_->defaultStream());

  ASSERT_EQ(launch_res, etrtSuccess);

  rsp = trace_helper.discover_trace_buffer();
  ASSERT_TRUE(rsp.status);

  success = trace_helper.configure_trace_state_knob(0);
  ASSERT_TRUE(success);

  success = trace_helper.prepare_trace_buffers();
  ASSERT_TRUE(success);

  std::vector<uint8_t> data(rsp.trace_buffer_size * NUMBER_OF_TRACE_BUFFERS);
  auto res = dev_->memcpy(
      data.data(),
      (const void *)(rsp.trace_base +
                     ALIGN(
                         sizeof(::device_api::non_privileged::trace_control_t),
                         TRACE_BUFFER_REGION_ALIGNEMNT)),
      rsp.trace_buffer_size * NUMBER_OF_TRACE_BUFFERS, etrtMemcpyDeviceToHost);

  auto result = et_runtime::tracing::DeviceAPI_DeviceFW_process_device_traces(
      &data[0], rsp.trace_buffer_size, NUMBER_OF_TRACE_BUFFERS);

  ASSERT_EQ(result, ::device_api::non_privileged::TRACE_STATUS_SUCCESS);
}

TEST_F(DeviceFWTest, Trace_TestTraceBufferSizeKnob) {
  auto kernels_dir = absl::GetFlag(FLAGS_kernels_dir);
  fs::path trace_kernel = fs::path(kernels_dir) / fs::path("trace.elf");
  auto &registry = dev_->codeRegistry();

  auto register_res = registry.registerKernel(
      "main", {Kernel::ArgType::T_layer_dynamic_info}, trace_kernel.string());
  ASSERT_TRUE((bool)register_res);
  auto &kernel = std::get<1>(register_res.get());

  auto load_res = registry.moduleLoad(kernel.moduleID(), dev_.get());
  ASSERT_EQ(load_res.getError(), etrtSuccess);
  auto module_id = load_res.get();

  TraceHelper trace_helper(*dev_);

  // Consume the trace buffers before launching the kernel
  ::device_api::non_privileged::discover_trace_buffer_rsp_t rsp = {0};
  rsp = trace_helper.discover_trace_buffer();
  ASSERT_TRUE(rsp.status);

  auto success = trace_helper.configure_trace_state_knob(0);
  ASSERT_TRUE(success);

  success = trace_helper.prepare_trace_buffers();
  ASSERT_TRUE(success);

  // Set trace buffer size
  success = trace_helper.configure_trace_buffer_size_knob(8192);
  ASSERT_TRUE(success);

  success = trace_helper.reset_trace_buffers();
  ASSERT_TRUE(success);

  success = trace_helper.configure_trace_state_knob(1);
  ASSERT_TRUE(success);

  Kernel::layer_dynamic_info_t layer_info = {0};
  Kernel::LaunchArg arg;
  arg.type = Kernel::ArgType::T_layer_dynamic_info;
  arg.value.layer_dynamic_info = layer_info;
  auto args = std::vector<Kernel::LaunchArg>({arg});
  auto launch = kernel.createKernelLaunch(args);
  auto launch_res = launch->launchBlocking(&dev_->defaultStream());

  ASSERT_EQ(launch_res, etrtSuccess);

  rsp = trace_helper.discover_trace_buffer();
  ASSERT_TRUE(rsp.status);

  success = trace_helper.configure_trace_state_knob(0);
  ASSERT_TRUE(success);

  success = trace_helper.prepare_trace_buffers();
  ASSERT_TRUE(success);

  std::vector<uint8_t> data(rsp.trace_buffer_size * NUMBER_OF_TRACE_BUFFERS);
  auto res = dev_->memcpy(
      data.data(),
      (const void *)(rsp.trace_base +
                     ALIGN(
                         sizeof(::device_api::non_privileged::trace_control_t),
                         TRACE_BUFFER_REGION_ALIGNEMNT)),
      rsp.trace_buffer_size * NUMBER_OF_TRACE_BUFFERS, etrtMemcpyDeviceToHost);

  // Verify if trace buffers are in good health
  success = Test_Trace_verify_buffer_size(&data[0], rsp.trace_buffer_size,
                                          NUMBER_OF_TRACE_BUFFERS);

  ASSERT_TRUE(success);
}

int main(int argc, char **argv) {
  google::InitGoogleLogging(argv[0]);
  google::InstallFailureSignalHandler();
  // Force logging in stderr and set min logging level
  FLAGS_minloglevel = 0;
  FLAGS_logtostderr = 1;
  testing::InitGoogleTest(&argc, argv);
  et_runtime::ParseCommandLineOptions(argc, argv, {"test_pcie_launch_kernels.cc"});
  return RUN_ALL_TESTS();
}
