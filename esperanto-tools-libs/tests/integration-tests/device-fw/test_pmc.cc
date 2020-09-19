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

#include <esperanto/runtime/Core/CommandLineOptions.h>
#include <esperanto/runtime/Core/Device.h>
#include <esperanto/runtime/EsperantoRuntime.h>
#include "Tracing/Tracing.h"
#include "device-fw-testing-helpers.h"
#include "esperanto/runtime/Core/DeviceHelpers.h"
#include "esperanto/runtime/Core/TraceHelper.h"
#include "esperanto/runtime/Core/PmcConfig.h"

#include <absl/flags/flag.h>
#include <array>
#include <chrono>
#include <cstdio>
#include <experimental/filesystem>
#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <random>
#include <string>
#include <vector>
#include <fstream>

// Set-up macros for input/output sizes
#define ONE_MB 1048576
#define TWO_MB 2*ONE_MB

#define INPUT_SIZE TWO_MB
#define OUTPUT_SIZE ONE_MB

#define CACHE_LINE_SIZE 64
#define PMC_CFG_SIZE 520     // 15 per shire plus 5 mem memshire 15x32 + 5x8  = 520: TODO: Add master shire
#define PMC_LOG_SIZE (2048 * (CACHE_LINE_SIZE / 8))

#define MIN_CYCLES 1
#define RETINST_HART0 2
#define RETINST_HART1 3
#define TLOADS 14
#define TSTORES 16
#define TFMAS 26
#define TREDUCES 27
#define TQUANTS 28
#define ETLINK_REQS 22
#define ETLINK_RESPS 23
#define ICACHE_REQS 8
#define ICACHE_RESPS 9
#define SC_CTL_STATUS_MASK 0x200100
#define L2_READS 0x4250
#define MSG_SEND 0x4
#define MS_CTL_STATUS_MASK 0xC00600
#define MESH_READS 0x00001FDULL;
#define MESH_WRITES 0x00001FEULL;

using namespace et_runtime::device;
using namespace et_runtime;
namespace fs = std::experimental::filesystem;

ABSL_FLAG(std::string, kernels_dir, "",
          "Directory where different kernel ELF files are located");


TEST_F(DeviceFWTest, DeviceAPI_PMCTracing) {

  // ================= Init: Registry and random input ===========================

  // Get kernel info
  auto kernels_dir = absl::GetFlag(FLAGS_kernels_dir);
  uint64_t test_num = 0;

  // You can use the _fc test without any change here.
  // The 2s test should run on the Zebu BEMU build and the FC test on the FC build
  fs::path pmc_test_2s_kernel_loc = fs::path(kernels_dir) / "pmc_test_2s_";
  pmc_test_2s_kernel_loc += fs::path(std::to_string(test_num));
  pmc_test_2s_kernel_loc /= fs::path("pmc_test_2s_" + std::to_string(test_num)+ ".elf");
  std::cout <<  "Running: " << test_num << pmc_test_2s_kernel_loc.string() << "\n";

  std::string seed_file_name = fs::current_path().string();

  seed_file_name = seed_file_name + std::string("/../../device-software/test-compute-kernels/pmc_test_2s/pmc_test_2s_")
    + std::to_string(test_num) + std::string("/tensor_rand_seed");
  std::ifstream seed_file;
  seed_file.open(seed_file_name);
  if (seed_file) {
    uint64_t compile_seed_val;
    seed_file >> compile_seed_val;
    printf ("CODE SEED (header files) = %lu\n", compile_seed_val);
  } else {
    printf ("Error opening %s, code seed unknown\n", seed_file_name.c_str());
  }

  // Random input initialization
  std::default_random_engine generator {};

  // For 2S since we run on commit CI use always the same seed
  uint64_t seed_val = 1;
  std::uniform_int_distribution<uint8_t> distribution(0,255);

  uint8_t input_array[TWO_MB];
  generator.seed(seed_val);
  for (uint32_t i=0; i < TWO_MB; i++) {
      input_array[i] = distribution(generator);
  }

  printf ("DATA SEED: %lu\n", seed_val);

  // PMC config buffer setup -- these are not used by test
  // but keep them around so that results can be interpreted.
  std::string hart_event_info[16] = { "(M) Minion cycles, hart 0",
                                 "(M) Minion cycles, hart 1",
                                 "(M) Retired inst. even hart",
                                 "(M) Tensor FMAs",
                                 "(M) Retired inst. odd hart",
                                 "(M) Reduces",
                                 "(M) Tensor Loads",
                                 "(M) Tensor Stores",
                                 "(N) ET Link Reqs",
                                 "(N) IC Reqs from minions",
                                 "(N) ET Link Resps",
                                 "(N) IC Resps to minions",
                                 "(SC) Cycles",
                                 "(SC) L2 Read Reqs",
                                 "(SC) Msg Send",
                                 "(MS)" };

  std::string sc_event_info[4] = {"Bank0", "Bank1", "Bank2", "Bank3" };
  std::string ms_event_info[4] = {"Cycles", "Read Reqs", "Write Reqs", "-----"};

  uint64_t shire_pmc_cfg[15] = { MIN_CYCLES, RETINST_HART0, RETINST_HART1,
                                 TLOADS, MIN_CYCLES, TFMAS, TREDUCES, TSTORES,
                                 ETLINK_REQS, ETLINK_RESPS, ICACHE_REQS, ICACHE_RESPS, 
                                 SC_CTL_STATUS_MASK, L2_READS ,MSG_SEND};

  uint64_t pmc_cfg[PMC_CFG_SIZE];

  for (uint64_t s=0; s < 32; s++) {
      for (uint64_t cnt=0; cnt < 15; cnt++) {
          pmc_cfg[s*15+cnt] = shire_pmc_cfg[cnt];
      }
  }
  for (uint64_t cnt = PMC_CFG_SIZE - 40; cnt < PMC_CFG_SIZE; cnt = cnt+5) {
      pmc_cfg[cnt] = MS_CTL_STATUS_MASK;
      pmc_cfg[cnt+1] = MESH_READS;
      pmc_cfg[cnt+2] = MESH_WRITES;
      pmc_cfg[cnt+3] = 0;
      pmc_cfg[cnt+4] = 0;
  }

  // =============================== Zebu part ======================================

  auto &registry = dev_->codeRegistry();
  auto register_res = registry.registerKernel("main", {Kernel::ArgType::T_layer_dynamic_info},
                                              pmc_test_2s_kernel_loc.string());
  ASSERT_TRUE((bool)register_res);
  auto &zebu_kernel = std::get<1>(register_res.get());

  auto load_res = registry.moduleLoad(zebu_kernel.moduleID(), dev_.get());
  ASSERT_EQ(load_res.getError(), etrtSuccess);
  auto module_id = load_res.get();

  TraceHelper trace_helper(*dev_); 
  auto trace_success = 0;

  trace_success = trace_helper.configure_trace_state_knob(0);
  ASSERT_TRUE(trace_success);

  trace_success = trace_helper.prepare_trace_buffers();
  ASSERT_TRUE(trace_success);

  trace_success = trace_helper.reset_trace_buffers();
  ASSERT_TRUE(trace_success);

  trace_success = trace_helper.configure_trace_state_knob(1);
  ASSERT_TRUE(trace_success);

  // Allocate the buffer on the device and get a pointer to that buffer
  BufferDebugInfo info;
  auto mres = dev_->mem_manager().mallocConstant(INPUT_SIZE, info);
  ASSERT_TRUE((bool)mres);
  auto dev_ptr_in = mres.get();
  auto status = dev_->memcpy(dev_ptr_in, HostBuffer(input_array), TWO_MB);
  ASSERT_EQ(status, etrtSuccess);

  // New configuration buffer -- copy it over to the device and then
  // send a PmcConfig command.
  mres = dev_->mem_manager().mallocConstant(PMC_CFG_SIZE * sizeof(uint64_t), info);
  ASSERT_TRUE((bool)mres);
  auto pmc_dev_ptr_in = mres.get();
  status = dev_->memcpy(pmc_dev_ptr_in, HostBuffer(pmc_cfg), PMC_CFG_SIZE * sizeof(uint64_t));
  ASSERT_EQ(status, etrtSuccess);
  PmcConfig pmc_config(*(dev_.get()));
  auto pmc_resp = pmc_config.set_pmc_config(reinterpret_cast<uint64_t>(pmc_dev_ptr_in.ptr()));
  ASSERT_TRUE((bool) pmc_resp);

  mres = dev_->mem_manager().mallocConstant(OUTPUT_SIZE, info);
  ASSERT_TRUE((bool)mres);
  auto dev_ptr_out = mres.get();

  // Launch the kernel
  Kernel::layer_dynamic_info_t zebu_layer_info = {};
  zebu_layer_info.tensor_a = reinterpret_cast<uint64_t>(dev_ptr_in.ptr());
  zebu_layer_info.tensor_b = INPUT_SIZE;
  zebu_layer_info.tensor_c = reinterpret_cast<uint64_t>(dev_ptr_out.ptr());
  zebu_layer_info.tensor_d = OUTPUT_SIZE;
  //zebu_layer_info.tensor_e = 0; // reinterpret_cast<uint64_t>(zebu_pmc_dev_ptr_in.ptr());
  //zebu_layer_info.tensor_f = 0; // PMC_CFG_SIZE;
  //zebu_layer_info.tensor_g = 0; //reinterpret_cast<uint64_t>(zebu_pmc_dev_ptr_out.ptr());
  //zebu_layer_info.tensor_h = 0; //PMC_LOG_SIZE * 8;

  // Set the kernel launch arguments
  Kernel::LaunchArg zebu_arg;
  zebu_arg.type = Kernel::ArgType::T_layer_dynamic_info;
  zebu_arg.value.layer_dynamic_info = zebu_layer_info;
  auto zebu_args = std::vector<Kernel::LaunchArg>({zebu_arg});
  // Create a KernelLaunch object from this kernel
  auto zebu_launch = zebu_kernel.createKernelLaunch(zebu_args);
  // Do the actual launch and wait for the results
  auto zebu_launch_res = zebu_launch->launchBlocking(&dev_->defaultStream());
  ASSERT_EQ(zebu_launch_res, etrtSuccess);

  // Read the data from the device
  std::vector<uint32_t> zebu_data(OUTPUT_SIZE, 0xEEEEEEEE);

  auto res = dev_->memcpy(HostBuffer(zebu_data.data()), dev_ptr_out, OUTPUT_SIZE);
  ASSERT_EQ(res, etrtSuccess);

  auto rsp = trace_helper.discover_trace_buffer();
  ASSERT_TRUE(rsp.status);

  trace_success = trace_helper.configure_trace_state_knob(0);
  ASSERT_TRUE(trace_success);

  trace_success = trace_helper.prepare_trace_buffers();
  ASSERT_TRUE(trace_success);

  std::vector<uint8_t> data(rsp.trace_buffer_size * NUMBER_OF_TRACE_BUFFERS);
  res = dev_->memcpy(
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

}

int main(int argc, char **argv) {
  google::InitGoogleLogging(argv[0]);
  // Force logging in stderr and set min logging level
  FLAGS_minloglevel = 0;
  FLAGS_logtostderr = 1;
  testing::InitGoogleTest(&argc, argv);
  et_runtime::ParseCommandLineOptions(argc, argv, {"test_pcie_launch_kernels.cc"});
  return RUN_ALL_TESTS();
}
