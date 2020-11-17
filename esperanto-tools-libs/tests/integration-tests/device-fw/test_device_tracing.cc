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

#include "Core/TraceHelper.h"
#include "Tracing/Tracing.h"
#include "device-fw-testing-helpers.h"

#include "esperanto/runtime/CodeManagement/CodeRegistry.h"

#include <chrono>
#include <ctime>
#include <string>

using namespace et_runtime::device;
using namespace et_runtime::device_api;
using namespace et_runtime;
namespace fs = std::experimental::filesystem;
using namespace std::chrono;

ABSL_FLAG(std::string, kernels_dir, "", "Directory where different kernel ELF files are located");

ABSL_FLAG(std::string, trace_elf, "", "Path to elf to execute");

TEST_F(DeviceFWTest, Trace_RingBufferTest_DataRead) {
  fs::path trace_kernel = fs::path(absl::GetFlag(FLAGS_kernels_dir)) / fs::path("trace_ring_buffer.elf");
  auto& registry = dev_->codeRegistry();

  auto register_res = registry.registerKernel("main", {Kernel::ArgType::T_layer_dynamic_info}, trace_kernel.string());
  ASSERT_TRUE((bool)register_res);
  auto& kernel = std::get<1>(register_res.get());

  auto load_res = registry.moduleLoad(kernel.moduleID(), dev_.get());
  ASSERT_EQ(load_res.getError(), etrtSuccess);
  auto module_id = load_res.get();

  TraceHelper trace_helper(*dev_);
  auto success = trace_helper.set_level_critical();
  ASSERT_TRUE(success);

  ::device_api::non_privileged::discover_trace_buffer_rsp_t rsp = {0};

  // Discover the trace buffer properties
  rsp = trace_helper.discover_trace_buffer();
  ASSERT_TRUE(rsp.status);

  // This test case expect ring buffer to be of size 4096 bytes.
  ASSERT_EQ(rsp.trace_buffer_size, 4096);

  // Disable the trace logging
  success = trace_helper.configure_trace_state_knob(0);
  ASSERT_TRUE(success);

  // Flush the data
  success = trace_helper.prepare_trace_buffers();
  ASSERT_TRUE(success);

  // Clear the ring buffers and state
  success = trace_helper.reset_trace_buffers();
  ASSERT_TRUE(success);

  // Disable the trace logging
  success = trace_helper.configure_trace_state_knob(1);
  ASSERT_TRUE(success);

  // Prepare kernel launch
  Kernel::layer_dynamic_info_t layer_info = {0};
  Kernel::LaunchArg arg;
  arg.type = Kernel::ArgType::T_layer_dynamic_info;
  arg.value.layer_dynamic_info = layer_info;
  auto args = std::vector<Kernel::LaunchArg>({arg});
  auto launch = kernel.createKernelLaunch(args);

  // Lauch the kernel
  auto launch_res = launch->launchBlocking(&dev_->defaultStream());
  ASSERT_EQ(launch_res, etrtSuccess);

  // Get trace buffer properties
  rsp = trace_helper.discover_trace_buffer();
  ASSERT_TRUE(rsp.status);

  // Disable trace logging before reading the data
  success = trace_helper.configure_trace_state_knob(0);
  ASSERT_TRUE(success);

  // Flush any remaining data
  success = trace_helper.prepare_trace_buffers();
  ASSERT_TRUE(success);

  std::vector<uint8_t> data(rsp.trace_buffer_size * NUMBER_OF_TRACE_BUFFERS);

  // Read data from device memory into host buffer
  auto res = dev_->memcpy(data.data(),
                          (const void*)(rsp.trace_base + ALIGN(sizeof(::device_api::non_privileged::trace_control_t),
                                                               TRACE_BUFFER_REGION_ALIGNEMNT)),
                          rsp.trace_buffer_size * NUMBER_OF_TRACE_BUFFERS, etrtMemcpyDeviceToHost);

  auto result2 = Test_Trace_verify_ring_buffer_data(&data[0], rsp.trace_buffer_size, NUMBER_OF_TRACE_BUFFERS, 1);

  ASSERT_TRUE(result2);
}

TEST_F(DeviceFWTest, Trace_RingBufferTest_BufferFull) {
  fs::path trace_kernel = fs::path(absl::GetFlag(FLAGS_kernels_dir)) / fs::path("trace_ring_buffer.elf");
  auto& registry = dev_->codeRegistry();

  auto register_res = registry.registerKernel("main", {Kernel::ArgType::T_layer_dynamic_info}, trace_kernel.string());
  ASSERT_TRUE((bool)register_res);
  auto& kernel = std::get<1>(register_res.get());

  auto load_res = registry.moduleLoad(kernel.moduleID(), dev_.get());
  ASSERT_EQ(load_res.getError(), etrtSuccess);
  auto module_id = load_res.get();

  TraceHelper trace_helper(*dev_);
  auto success = trace_helper.set_level_error();
  ASSERT_TRUE(success);

  ::device_api::non_privileged::discover_trace_buffer_rsp_t rsp = {0};

  // Discover the trace buffer properties
  rsp = trace_helper.discover_trace_buffer();
  ASSERT_TRUE(rsp.status);

  // This test case expect ring buffer to be of size 4096.
  ASSERT_EQ(rsp.trace_buffer_size, 4096);

  // Disable the trace logging
  success = trace_helper.configure_trace_state_knob(0);
  ASSERT_TRUE(success);

  // Flush the data
  success = trace_helper.prepare_trace_buffers();
  ASSERT_TRUE(success);

  // Clear the ring buffer and state
  success = trace_helper.reset_trace_buffers();
  ASSERT_TRUE(success);

  // Disable the trace logging
  success = trace_helper.configure_trace_state_knob(1);
  ASSERT_TRUE(success);

  // Prepare kernel launch
  Kernel::layer_dynamic_info_t layer_info = {0};
  Kernel::LaunchArg arg;
  arg.type = Kernel::ArgType::T_layer_dynamic_info;
  arg.value.layer_dynamic_info = layer_info;
  auto args = std::vector<Kernel::LaunchArg>({arg});
  auto launch = kernel.createKernelLaunch(args);

  // Lauch the kernel
  auto launch_res = launch->launchBlocking(&dev_->defaultStream());
  ASSERT_EQ(launch_res, etrtSuccess);

  // Get trace buffer properties
  rsp = trace_helper.discover_trace_buffer();
  ASSERT_TRUE(rsp.status);

  // Disable trace logging before reading the data
  success = trace_helper.configure_trace_state_knob(0);
  ASSERT_TRUE(success);

  // Flush any remaining data
  success = trace_helper.prepare_trace_buffers();
  ASSERT_TRUE(success);

  std::vector<uint8_t> data(rsp.trace_buffer_size * NUMBER_OF_TRACE_BUFFERS);

  // Read data from device memory into host buffer
  auto res = dev_->memcpy(data.data(),
                          (const void*)(rsp.trace_base + ALIGN(sizeof(::device_api::non_privileged::trace_control_t),
                                                               TRACE_BUFFER_REGION_ALIGNEMNT)),
                          rsp.trace_buffer_size * NUMBER_OF_TRACE_BUFFERS, etrtMemcpyDeviceToHost);

  auto result2 = Test_Trace_verify_ring_buffer_data(&data[0], rsp.trace_buffer_size, NUMBER_OF_TRACE_BUFFERS, 2);

  ASSERT_TRUE(result2);
}

TEST_F(DeviceFWTest, DISABLED_Trace_RingBufferTest_BufferWrapped) {
  fs::path trace_kernel = fs::path(absl::GetFlag(FLAGS_kernels_dir)) / fs::path("trace_ring_buffer.elf");
  auto& registry = dev_->codeRegistry();

  auto register_res = registry.registerKernel("main", {Kernel::ArgType::T_layer_dynamic_info}, trace_kernel.string());
  ASSERT_TRUE((bool)register_res);
  auto& kernel = std::get<1>(register_res.get());

  auto load_res = registry.moduleLoad(kernel.moduleID(), dev_.get());
  ASSERT_EQ(load_res.getError(), etrtSuccess);
  auto module_id = load_res.get();

  TraceHelper trace_helper(*dev_);
  auto success = trace_helper.set_level_warning();
  ASSERT_TRUE(success);

  ::device_api::non_privileged::discover_trace_buffer_rsp_t rsp = {0};

  // Discover the trace buffer properties
  rsp = trace_helper.discover_trace_buffer();
  ASSERT_TRUE(rsp.status);

  // This test case expect ring buffer to be of size 4096.
  ASSERT_EQ(rsp.trace_buffer_size, 4096);

  // Disable the trace logging
  success = trace_helper.configure_trace_state_knob(0);
  ASSERT_TRUE(success);

  // Flush the data
  success = trace_helper.prepare_trace_buffers();
  ASSERT_TRUE(success);

  // Clear the ring buffer and state
  success = trace_helper.reset_trace_buffers();
  ASSERT_TRUE(success);

  // Disable the trace logging
  success = trace_helper.configure_trace_state_knob(1);
  ASSERT_TRUE(success);

  // Prepare kernel launch
  Kernel::layer_dynamic_info_t layer_info = {0};
  Kernel::LaunchArg arg;
  arg.type = Kernel::ArgType::T_layer_dynamic_info;
  arg.value.layer_dynamic_info = layer_info;
  auto args = std::vector<Kernel::LaunchArg>({arg});
  auto launch = kernel.createKernelLaunch(args);

  // Lauch the kernel
  auto launch_res = launch->launchBlocking(&dev_->defaultStream());
  ASSERT_EQ(launch_res, etrtSuccess);

  // Get trace buffer properties
  rsp = trace_helper.discover_trace_buffer();
  ASSERT_TRUE(rsp.status);

  // Disable trace logging before reading the data
  success = trace_helper.configure_trace_state_knob(0);
  ASSERT_TRUE(success);

  // Flush any remaining data
  success = trace_helper.prepare_trace_buffers();
  ASSERT_TRUE(success);

/* JIRA -SW 5053 - disabling check since its failing Zebu BEMU, needs to be re-enabled once SW-5053 has been resolved
  std::vector<uint8_t> data(rsp.trace_buffer_size * NUMBER_OF_TRACE_BUFFERS);

  // Read data from device memory into host buffer
  auto res = dev_->memcpy(data.data(),
                          (const void*)(rsp.trace_base + ALIGN(sizeof(::device_api::non_privileged::trace_control_t),
                                                               TRACE_BUFFER_REGION_ALIGNEMNT)),
                          rsp.trace_buffer_size * NUMBER_OF_TRACE_BUFFERS, etrtMemcpyDeviceToHost);

  auto result2 = Test_Trace_verify_ring_buffer_data(&data[0], rsp.trace_buffer_size, NUMBER_OF_TRACE_BUFFERS, 3);

  ASSERT_TRUE(result2);
*/
}

TEST_F(DeviceFWTest, Trace_test) {
  fs::path trace_kernel = fs::path(absl::GetFlag(FLAGS_trace_elf));
  auto& registry = dev_->codeRegistry();

  auto register_res = registry.registerKernel("main", {Kernel::ArgType::T_layer_dynamic_info}, trace_kernel.string());
  ASSERT_TRUE((bool)register_res);
  auto& kernel = std::get<1>(register_res.get());

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
  auto res = dev_->memcpy(data.data(),
                          (const void*)(rsp.trace_base + ALIGN(sizeof(::device_api::non_privileged::trace_control_t),
                                                               TRACE_BUFFER_REGION_ALIGNEMNT)),
                          rsp.trace_buffer_size * NUMBER_OF_TRACE_BUFFERS, etrtMemcpyDeviceToHost);

  auto result = et_runtime::tracing::DeviceAPI_DeviceFW_process_device_traces(&data[0], rsp.trace_buffer_size,
                                                                              NUMBER_OF_TRACE_BUFFERS);

  ASSERT_EQ(result, ::device_api::non_privileged::TRACE_STATUS_SUCCESS);
}

TEST_F(DeviceFWTest, Trace_FindLoggingOverhead) {
  fs::path trace_kernel = fs::path(absl::GetFlag(FLAGS_kernels_dir)) / fs::path("trace_ring_buffer.elf");
  auto& registry = dev_->codeRegistry();

  auto register_res = registry.registerKernel("main", {Kernel::ArgType::T_layer_dynamic_info}, trace_kernel.string());
  ASSERT_TRUE((bool)register_res);
  auto& kernel = std::get<1>(register_res.get());

  auto load_res = registry.moduleLoad(kernel.moduleID(), dev_.get());
  ASSERT_EQ(load_res.getError(), etrtSuccess);
  auto module_id = load_res.get();

  TraceHelper trace_helper(*dev_);

  // For maximum utilization, following log level will fully fill
  // the buffer by selected kernel.
  auto success = trace_helper.set_level_error();
  ASSERT_TRUE(success);

  ::device_api::non_privileged::discover_trace_buffer_rsp_t rsp = {0};

  // Discover the trace buffer properties
  rsp = trace_helper.discover_trace_buffer();
  ASSERT_TRUE(rsp.status);

  // Disable the trace logging
  success = trace_helper.configure_trace_state_knob(0);
  ASSERT_TRUE(success);

  // Flush the data
  success = trace_helper.prepare_trace_buffers();
  ASSERT_TRUE(success);

  // Clear the ring buffer and state
  success = trace_helper.reset_trace_buffers();
  ASSERT_TRUE(success);

  // Disable the trace logging
  success = trace_helper.configure_trace_state_knob(0);
  ASSERT_TRUE(success);

  Kernel::layer_dynamic_info_t layer_info = {0};
  Kernel::LaunchArg arg;
  arg.type = Kernel::ArgType::T_layer_dynamic_info;
  arg.value.layer_dynamic_info = layer_info;
  auto args = std::vector<Kernel::LaunchArg>({arg});
  auto launch = kernel.createKernelLaunch(args);

  // =================== Executing workload without tracing... ==============

  // Save current time before launching the kernel
  auto start1 = chrono::high_resolution_clock::now();

  // Launch the kernel
  auto launch_res = launch->launchBlocking(&dev_->defaultStream());

  // Ensure previous operation was successfull
  ASSERT_EQ(launch_res, etrtSuccess);

  // Save current time once kernel is executed and ended
  auto end1 = chrono::high_resolution_clock::now();

  // Calculate the differnce to find kernel execution time
  auto delta1 = chrono::duration_cast<chrono::microseconds>(end1 - start1).count();

  // =================== Executing workload with tracing... ==============

  // Test case specific. For maximum utilization.
  auto new_harts_mask = 0xFFFFFFFFFFFFFFFFUL;
  auto enabled_harts = 64;

  // Configure harts mask
  success = trace_helper.configure_trace_harts_mask_knob(new_harts_mask);
  ASSERT_TRUE(success);

  // Discover the trace buffer properties
  rsp = trace_helper.discover_trace_buffer();
  ASSERT_TRUE(rsp.status);

  //  Enable trace logging
  success = trace_helper.configure_trace_state_knob(1);
  ASSERT_TRUE(success);

  // Save current time before launching the kernel
  auto start2 = chrono::high_resolution_clock::now();

  // Launch the kernel
  launch_res = launch->launchBlocking(&dev_->defaultStream());
  ASSERT_EQ(launch_res, etrtSuccess);

  // Save current time once kernel is executed and ended
  auto end2 = chrono::high_resolution_clock::now();

  // Calculate the differnce to find kernel execution time
  auto delta2 = chrono::duration_cast<chrono::microseconds>(end2 - start2).count();

  // Calculate the overhead by finding the difference in execution time with
  // and without trace
  auto execution_overhead = delta2 - delta1;

  // Calculate percent overhead
  auto percent_overhead = (execution_overhead * 100) / delta1;

  // Get trace buffer properties
  rsp = trace_helper.discover_trace_buffer();
  ASSERT_TRUE(rsp.status);

  // Disable trace logging before reading the data
  success = trace_helper.configure_trace_state_knob(0);
  ASSERT_TRUE(success);

  // Flush any remaining data
  success = trace_helper.prepare_trace_buffers();
  ASSERT_TRUE(success);

  auto enabled_shires = 0; // This is to be calculated yet.
  auto shire_size = rsp.trace_buffer_size * enabled_harts;
  auto shire_mask = rsp.shire_mask;
  auto shire_mask_size = 64;

  for (int shire_id = 0; shire_id < shire_mask_size; shire_id++) {
    enabled_shires += (1UL & (shire_mask >> shire_id));
  }

  std::vector<uint8_t> shire_data(shire_size * enabled_shires);
  auto shire_counter = 0;

  // Save current time before reading the data
  auto start3 = chrono::high_resolution_clock::now();

  // Loop through all shire ids to read data one by one. Note that we have enabled
  // all the harts, therefore we will be reading the data one shire by one.
  for (int shire_index = 0; shire_index < shire_mask_size; shire_index++) {
    // Check if we need to read data for this shire?
    if ((1UL & (shire_mask >> shire_index)) == 1UL) {
      // Read data from device memory into host buffer for this time.
      auto res = dev_->memcpy(
        (shire_data.data() + shire_size * shire_counter),
        (const void*)(rsp.trace_base + (shire_size * shire_index) +
                      ALIGN(sizeof(::device_api::non_privileged::trace_control_t), TRACE_BUFFER_REGION_ALIGNEMNT)),
        shire_size, etrtMemcpyDeviceToHost);

      // Data read for following number of shires.
      shire_counter++;
    }
  }

  // Save current time after the data is read
  auto end3 = chrono::high_resolution_clock::now();

  // Calculate data copy time by calculating the difference
  auto delta3 = chrono::duration_cast<chrono::microseconds>(end3 - start3).count();

  // Save current time before parsing the data into protobuf
  auto start4 = chrono::high_resolution_clock::now();

  // Parse the trace data into protobuff
  auto result = et_runtime::tracing::DeviceAPI_DeviceFW_process_device_traces(&shire_data[0], rsp.trace_buffer_size,
                                                                              enabled_shires * enabled_harts);

  ASSERT_EQ(result, ::device_api::non_privileged::TRACE_STATUS_SUCCESS);

  // Save the current time as data is parsed
  auto end4 = chrono::high_resolution_clock::now();

  // Find the data parsing time
  auto delta4 = chrono::duration_cast<chrono::microseconds>(end4 - start4).count();

  // Calculate timestamps
  microseconds ms1 = duration_cast<microseconds>(start1.time_since_epoch());
  seconds s1 = duration_cast<seconds>(ms1);
  std::time_t t1 = s1.count();
  std::size_t fractional_seconds1 = ms1.count() % 1000000;

  microseconds ms2 = duration_cast<microseconds>(end1.time_since_epoch());
  seconds s2 = duration_cast<seconds>(ms2);
  std::time_t t2 = s2.count();
  std::size_t fractional_seconds2 = ms2.count() % 1000000;

  microseconds ms3 = duration_cast<microseconds>(start2.time_since_epoch());
  seconds s3 = duration_cast<seconds>(ms3);
  std::time_t t3 = s3.count();
  std::size_t fractional_seconds3 = ms3.count() % 1000000;

  microseconds ms4 = duration_cast<microseconds>(end2.time_since_epoch());
  seconds s4 = duration_cast<seconds>(ms4);
  std::time_t t4 = s4.count();
  std::size_t fractional_seconds4 = ms4.count() % 1000000;

  RTINFO << "Test shire_mask: " << rsp.shire_mask << " Test harts_mask: " << rsp.harts_mask;
  RTINFO << "Enabled shires: " << enabled_shires;
  RTINFO << "Parsed shires: " << shire_counter;
  RTINFO << "Kernel Launch without Trace\r\n";
  RTINFO << "Host Launch Timestamp: " << fractional_seconds1 << " microseconds " << std::ctime(&t1);
  RTINFO << "\t\t\tCM Kernel Start\r\n";
  RTINFO << "\t\t\t" << delta1 << " microseconds \r\n";
  RTINFO << "\t\t\tCM Kernel Complete\r\n";
  RTINFO << "Host Resp TimeStamp: " << fractional_seconds2 << " microseconds " << std::ctime(&t2);

  RTINFO << "Kernel Launch with Trace\r\n";
  RTINFO << "Host Launch Timestamp: " << fractional_seconds3 << " microseconds " << std::ctime(&t3);
  RTINFO << "\t\t\tCM Kernel Start\r\n";
  RTINFO << "\t\t\t" << delta2 << " microseconds \r\n";
  RTINFO << "\t\t\tCM Kernel Complete\r\n";
  RTINFO << "Host Resp TimeStamp: " << fractional_seconds4 << " microseconds " << std::ctime(&t4);

  RTINFO << "Overhead Calculation\r\n";
  RTINFO << "CM overhead = " << execution_overhead << "(" << percent_overhead << "%)\r\n";
  RTINFO << "Host Overhead = \r\n";
  RTINFO << "\t\t- Trace Data fetch from Device: " << delta3 << " (" << (delta3 * 100) / (delta1) << "%)\r\n";
  RTINFO << "\t\t- Post Processing (Adding overhead to convert Trace binary to "
            "Protobuf): "
         << delta4 << " (" << (delta4 * 100) / (delta1) << "%)\r\n";
}

TEST_F(DeviceFWTest, Trace_TestTraceLogLevelKnob) {
  fs::path trace_kernel = fs::path(absl::GetFlag(FLAGS_trace_elf));
  auto& registry = dev_->codeRegistry();

  auto register_res = registry.registerKernel("main", {Kernel::ArgType::T_layer_dynamic_info}, trace_kernel.string());
  ASSERT_TRUE((bool)register_res);
  auto& kernel = std::get<1>(register_res.get());

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
  auto res = dev_->memcpy(data.data(),
                          (const void*)(rsp.trace_base + ALIGN(sizeof(::device_api::non_privileged::trace_control_t),
                                                               TRACE_BUFFER_REGION_ALIGNEMNT)),
                          rsp.trace_buffer_size * NUMBER_OF_TRACE_BUFFERS, etrtMemcpyDeviceToHost);

  auto result = et_runtime::tracing::DeviceAPI_DeviceFW_process_device_traces(&data[0], rsp.trace_buffer_size,
                                                                              NUMBER_OF_TRACE_BUFFERS);

  ASSERT_EQ(result, ::device_api::non_privileged::TRACE_STATUS_SUCCESS);

  // Verify if any events are logged below the threshold set
  result = Test_Trace_verify_loglevel_disable(&data[0], rsp.trace_buffer_size, NUMBER_OF_TRACE_BUFFERS,
                                              ::device_api::non_privileged::LOG_LEVELS_CRITICAL);

  ASSERT_TRUE(result);
}

TEST_F(DeviceFWTest, Trace_TestTraceGroupKnobs) {
  fs::path trace_kernel = fs::path(absl::GetFlag(FLAGS_trace_elf));
  auto& registry = dev_->codeRegistry();

  auto register_res = registry.registerKernel("main", {Kernel::ArgType::T_layer_dynamic_info}, trace_kernel.string());
  ASSERT_TRUE((bool)register_res);
  auto& kernel = std::get<1>(register_res.get());

  auto load_res = registry.moduleLoad(kernel.moduleID(), dev_.get());
  ASSERT_EQ(load_res.getError(), etrtSuccess);
  auto module_id = load_res.get();

  TraceHelper trace_helper(*dev_);

  // Disable Trace group
  auto success = trace_helper.configure_trace_group_knob(::device_api::non_privileged::TRACE_GROUP_ID_PERFORMANCE, 0);
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
  auto res = dev_->memcpy(data.data(),
                          (const void*)(rsp.trace_base + ALIGN(sizeof(::device_api::non_privileged::trace_control_t),
                                                               TRACE_BUFFER_REGION_ALIGNEMNT)),
                          rsp.trace_buffer_size * NUMBER_OF_TRACE_BUFFERS, etrtMemcpyDeviceToHost);

  auto result = et_runtime::tracing::DeviceAPI_DeviceFW_process_device_traces(&data[0], rsp.trace_buffer_size,
                                                                              NUMBER_OF_TRACE_BUFFERS);

  ASSERT_EQ(result, ::device_api::non_privileged::TRACE_STATUS_SUCCESS);

  // Verify specified group events are disabled
  result = Test_Trace_verify_group_disable(&data[0], rsp.trace_buffer_size, NUMBER_OF_TRACE_BUFFERS,
                                           ::device_api::non_privileged::TRACE_GROUP_ID_PERFORMANCE);

  ASSERT_TRUE(result);
}

TEST_F(DeviceFWTest, Trace_TestTraceEventKnobs) {
  fs::path trace_kernel = fs::path(absl::GetFlag(FLAGS_trace_elf));
  auto& registry = dev_->codeRegistry();

  auto register_res = registry.registerKernel("main", {Kernel::ArgType::T_layer_dynamic_info}, trace_kernel.string());
  ASSERT_TRUE((bool)register_res);
  auto& kernel = std::get<1>(register_res.get());

  auto load_res = registry.moduleLoad(kernel.moduleID(), dev_.get());
  ASSERT_EQ(load_res.getError(), etrtSuccess);
  auto module_id = load_res.get();

  TraceHelper trace_helper(*dev_);

  // Disable the event
  auto success =
    trace_helper.configure_trace_event_knob(::device_api::non_privileged::TRACE_EVENT_ID_PERFORMANCE_PERFCTR, 0);
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
  auto res = dev_->memcpy(data.data(),
                          (const void*)(rsp.trace_base + ALIGN(sizeof(::device_api::non_privileged::trace_control_t),
                                                               TRACE_BUFFER_REGION_ALIGNEMNT)),
                          rsp.trace_buffer_size * NUMBER_OF_TRACE_BUFFERS, etrtMemcpyDeviceToHost);

  auto result = et_runtime::tracing::DeviceAPI_DeviceFW_process_device_traces(&data[0], rsp.trace_buffer_size,
                                                                              NUMBER_OF_TRACE_BUFFERS);

  ASSERT_EQ(result, ::device_api::non_privileged::TRACE_STATUS_SUCCESS);

  // Verify specified event is disabled
  result = Test_Trace_verify_event_disable(&data[0], rsp.trace_buffer_size, NUMBER_OF_TRACE_BUFFERS,
                                           ::device_api::non_privileged::TRACE_EVENT_ID_PERFORMANCE_PERFCTR);

  ASSERT_TRUE(result);
}

TEST_F(DeviceFWTest, Trace_TestTraceStateKnob) {
  fs::path trace_kernel = fs::path(absl::GetFlag(FLAGS_trace_elf));
  auto& registry = dev_->codeRegistry();

  auto register_res = registry.registerKernel("main", {Kernel::ArgType::T_layer_dynamic_info}, trace_kernel.string());
  ASSERT_TRUE((bool)register_res);
  auto& kernel = std::get<1>(register_res.get());

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
  auto res = dev_->memcpy(data.data(),
                          (const void*)(rsp.trace_base + ALIGN(sizeof(::device_api::non_privileged::trace_control_t),
                                                               TRACE_BUFFER_REGION_ALIGNEMNT)),
                          rsp.trace_buffer_size * NUMBER_OF_TRACE_BUFFERS, etrtMemcpyDeviceToHost);

  auto result = et_runtime::tracing::DeviceAPI_DeviceFW_process_device_traces(&data[0], rsp.trace_buffer_size,
                                                                              NUMBER_OF_TRACE_BUFFERS);

  ASSERT_EQ(result, ::device_api::non_privileged::TRACE_STATUS_SUCCESS);

  // Verify if trace logging is disabled
  result = Test_Trace_verify_state_disable(&data[0], rsp.trace_buffer_size, NUMBER_OF_TRACE_BUFFERS);

  ASSERT_TRUE(result);
}

TEST_F(DeviceFWTest, Trace_TestTraceUartLoggingKnob) {
  fs::path trace_kernel = fs::path(absl::GetFlag(FLAGS_trace_elf));
  auto& registry = dev_->codeRegistry();

  auto register_res = registry.registerKernel("main", {Kernel::ArgType::T_layer_dynamic_info}, trace_kernel.string());
  ASSERT_TRUE((bool)register_res);
  auto& kernel = std::get<1>(register_res.get());

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
  auto res = dev_->memcpy(data.data(),
                          (const void*)(rsp.trace_base + ALIGN(sizeof(::device_api::non_privileged::trace_control_t),
                                                               TRACE_BUFFER_REGION_ALIGNEMNT)),
                          rsp.trace_buffer_size * NUMBER_OF_TRACE_BUFFERS, etrtMemcpyDeviceToHost);

  auto result = et_runtime::tracing::DeviceAPI_DeviceFW_process_device_traces(&data[0], rsp.trace_buffer_size,
                                                                              NUMBER_OF_TRACE_BUFFERS);

  ASSERT_EQ(result, ::device_api::non_privileged::TRACE_STATUS_SUCCESS);
}

TEST_F(DeviceFWTest, Trace_TestTraceBufferSizeKnob) {
  fs::path trace_kernel = fs::path(absl::GetFlag(FLAGS_trace_elf));
  auto& registry = dev_->codeRegistry();

  auto register_res = registry.registerKernel("main", {Kernel::ArgType::T_layer_dynamic_info}, trace_kernel.string());
  ASSERT_TRUE((bool)register_res);
  auto& kernel = std::get<1>(register_res.get());

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
  auto res = dev_->memcpy(data.data(),
                          (const void*)(rsp.trace_base + ALIGN(sizeof(::device_api::non_privileged::trace_control_t),
                                                               TRACE_BUFFER_REGION_ALIGNEMNT)),
                          rsp.trace_buffer_size * NUMBER_OF_TRACE_BUFFERS, etrtMemcpyDeviceToHost);

  // Verify if trace buffers are in good health
  success = Test_Trace_verify_buffer_size(&data[0], rsp.trace_buffer_size, NUMBER_OF_TRACE_BUFFERS);

  ASSERT_TRUE(success);
}

TEST_F(DeviceFWTest, Trace_TestNonContiguosHartIDs) {

  fs::path trace_kernel = fs::path(absl::GetFlag(FLAGS_trace_elf));
  auto& registry = dev_->codeRegistry();

  auto register_res = registry.registerKernel("main", {Kernel::ArgType::T_layer_dynamic_info}, trace_kernel.string());
  ASSERT_TRUE((bool)register_res);
  auto& kernel = std::get<1>(register_res.get());

  auto load_res = registry.moduleLoad(kernel.moduleID(), dev_.get());
  ASSERT_EQ(load_res.getError(), etrtSuccess);
  auto module_id = load_res.get();

  TraceHelper trace_helper(*dev_);

  // For maximum utilization, following log level will fully fill
  // the buffer by selected kernel.
  auto success = trace_helper.set_level_error();
  ASSERT_TRUE(success);

  ::device_api::non_privileged::discover_trace_buffer_rsp_t rsp = {0};

  // Discover the trace buffer properties
  rsp = trace_helper.discover_trace_buffer();
  ASSERT_TRUE(rsp.status);

  // Disable the trace logging
  success = trace_helper.configure_trace_state_knob(0);
  ASSERT_TRUE(success);

  // Flush the data
  success = trace_helper.prepare_trace_buffers();
  ASSERT_TRUE(success);

  // Clear the ring buffer and state
  success = trace_helper.reset_trace_buffers();
  ASSERT_TRUE(success);

  Kernel::layer_dynamic_info_t layer_info = {0};
  Kernel::LaunchArg arg;
  arg.type = Kernel::ArgType::T_layer_dynamic_info;
  arg.value.layer_dynamic_info = layer_info;
  auto args = std::vector<Kernel::LaunchArg>({arg});
  auto launch = kernel.createKernelLaunch(args);

  // non contiguous hart mask.
  auto harts_mask = 0xFEDCBA0987654321UL;
  // non contiguous shire mask.
  auto shire_mask = 0x1234567890ABCDEFUL;

  // Configure harts mask
  success = trace_helper.configure_trace_harts_mask_knob(harts_mask);
  ASSERT_TRUE(success);

  // Configure shire mask
  success = trace_helper.configure_trace_shire_mask_knob(shire_mask);
  ASSERT_TRUE(success);

  // Discover the trace buffer properties
  rsp = trace_helper.discover_trace_buffer();
  ASSERT_TRUE(rsp.status);

  //  Enable trace logging
  success = trace_helper.configure_trace_state_knob(1);
  ASSERT_TRUE(success);

  // Launch the kernel
  auto launch_res = launch->launchBlocking(&dev_->defaultStream());
  ASSERT_EQ(launch_res, etrtSuccess);

  // Get trace buffer properties
  rsp = trace_helper.discover_trace_buffer();
  ASSERT_TRUE(rsp.status);

  // Disable trace logging before reading the data
  success = trace_helper.configure_trace_state_knob(0);
  ASSERT_TRUE(success);

  // Flush any remaining data
  success = trace_helper.prepare_trace_buffers();
  ASSERT_TRUE(success);

  auto enabled_shires = 0; // This is to be calculated yet.
  for (uint8_t shire_id = 0; shire_id < sizeof(rsp.shire_mask) * 8; shire_id++) {
    enabled_shires += (1UL & (rsp.shire_mask >> shire_id));
  }

  auto enabled_harts_per_shire = 0; // This is to be calculated yet.
  for (uint8_t hart_id = 0; hart_id < sizeof(rsp.harts_mask) * 8; hart_id++) {
    enabled_harts_per_shire += (1UL & (rsp.harts_mask >> hart_id));
  }

  std::vector<uint8_t> shire_data(enabled_shires * enabled_harts_per_shire * rsp.trace_buffer_size);
  auto shire_counter = 0;

  trace_helper.extract_device_trace_buffers(rsp, shire_data.data());

  // Parse the trace data into protobuff
  auto result = et_runtime::tracing::DeviceAPI_DeviceFW_process_device_traces(&shire_data[0], rsp.trace_buffer_size,
                                                                              enabled_shires * enabled_harts_per_shire);

  ASSERT_EQ(result, ::device_api::non_privileged::TRACE_STATUS_SUCCESS);
}

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  google::InstallFailureSignalHandler();
  // Force logging in stderr and set min logging level
  FLAGS_minloglevel = 0;
  FLAGS_logtostderr = 1;
  testing::InitGoogleTest(&argc, argv);
  et_runtime::ParseCommandLineOptions(argc, argv, {"test_device_tracing.cc"});
  return RUN_ALL_TESTS();
}
