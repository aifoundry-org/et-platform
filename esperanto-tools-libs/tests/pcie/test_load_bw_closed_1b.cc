//******************************************************************************
// Copyright (C) 2018,2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include <esperanto/runtime/Core/CommandLineOptions.h>
#include <esperanto/runtime/Core/Device.h>
#include <esperanto/runtime/EsperantoRuntime.h>

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

// NUM_ITERATIONS shows how many loops the kernel will run
// Each minion has loads one address per iteration
#define NUM_ITERATIONS 500
#define NUM_MINIONS_PER_SHIRE 32
#define NUM_SHIRES 32
#define NUM_ADDRESSES NUM_SHIRES * NUM_MINIONS_PER_SHIRE * NUM_ITERATIONS
#define CACHE_LINE_SIZE 8
#define START_ADDRESS 0x8200000000ULL

using namespace et_runtime::device;
using namespace et_runtime;
namespace fs = std::experimental::filesystem;

ABSL_FLAG(std::string, kernels_dir, "",
          "Directory where different kernel ELF files are located");


// Test Fixture for creating a PCIE device
class PCIEKernelLaunchTest: public ::testing::Test {
protected:
  void SetUp() override {
    // Just use the commented line instead to run on sysemu
    //absl::SetFlag(&FLAGS_dev_target, DeviceTargetOption("sysemu_grpc"));  
    absl::SetFlag(&FLAGS_dev_target, DeviceTargetOption("pcie"));
    dev_ = std::make_shared<Device>(0);
    auto error = dev_->init();
    ASSERT_EQ(error, etrtSuccess);
  }

  void TearDown() override {
    dev_->resetDevice();
  }
  std::shared_ptr<Device> dev_;
};


//
// test_load_bw_closed_1b kernel test:
// Invokes the load_bw kernel and sends it a tensor
// with random pointers + the constraint that they all go to MC bank 7.
// Each minion will access 500 random addresses
// resulting into closed page requests equally distributes across all memory controllers.
TEST_F(PCIEKernelLaunchTest, load_bw_closed_1b_kernel) {

  // Get kernel info
  auto kernels_dir = absl::GetFlag(FLAGS_kernels_dir);
  fs::path load_bw_kernel_loc = fs::path(kernels_dir) / fs::path("load_bw.elf");
  std::cout <<  "Running: " << load_bw_kernel_loc.string() << "\n";
	  
  auto &registry = CodeRegistry::registry();

  // Random input initialization
  std::default_random_engine generator {};
 
  // TBD: Make random to see variations in BW.
  uint64_t seed_val = 1;
  std::uniform_int_distribution<uint64_t> row_distr(0,0x7FFF);
  std::uniform_int_distribution<uint64_t> col_distr(0,31);
  std::uniform_int_distribution<uint64_t> bank_mc_ms_distr(0,127);

  // Random addresses for row, column, mc/ms/bank to make closed page accesses
  // across all memory shires / memory controllers.
  // Set bank bits (12:10) to 7
  uint64_t input_array[NUM_ADDRESSES];
  generator.seed(seed_val);
  for (uint32_t i=0; i < NUM_ADDRESSES; i++) {
    uint64_t row_val = row_distr(generator);
    uint64_t col_val = col_distr(generator);
    uint64_t bank_mc_ms_val = bank_mc_ms_distr(generator);
    bank_mc_ms_val = bank_mc_ms_val | 0x070ULL;
    input_array[i] = ((row_val << 18) | (col_val << 13) | (bank_mc_ms_val << 6)) & 0xFFFFFFFFC0ULL;
    input_array[i] = input_array[i] + START_ADDRESS;
  }

  auto register_res = registry.registerKernel("main", {Kernel::ArgType::T_layer_dynamic_info},
                                              load_bw_kernel_loc.string());
  ASSERT_TRUE((bool)register_res);
  auto &kernel = std::get<1>(register_res.get());
  
  // Load the kernel on the device
  auto load_res = registry.moduleLoad(kernel.moduleID(), dev_.get());
  ASSERT_EQ(load_res.getError(), etrtSuccess);
  auto module_id = load_res.get();

  void *dev_ptr_in = 0;
  // Allocate the buffer on the device and get a pointer to that buffer
  auto status = dev_->mem_manager().malloc(&dev_ptr_in, NUM_ADDRESSES * sizeof(uint64_t));
  ASSERT_EQ(status, etrtSuccess);
  dev_->memcpy(dev_ptr_in, (void *) input_array, NUM_ADDRESSES * sizeof(uint64_t), etrtMemcpyHostToDevice);

  // Output tensor will hold minion ids and result of test
  void *dev_ptr_out = 0;
  status = dev_->mem_manager().malloc(&dev_ptr_out, NUM_SHIRES * NUM_MINIONS_PER_SHIRE * CACHE_LINE_SIZE * sizeof(uint64_t));
  ASSERT_EQ(status, etrtSuccess);

  // Launch the kernel where the first argument is the pointer to the buffer on the
  // device, 2nd is the total number of addresses, third the number of minions, and last the output buffer
  Kernel::layer_dynamic_info_t layer_info = {};
  layer_info.tensor_a = reinterpret_cast<uint64_t>(dev_ptr_in);
  layer_info.tensor_b = NUM_ADDRESSES;
  layer_info.tensor_c = NUM_SHIRES * NUM_MINIONS_PER_SHIRE;
  layer_info.tensor_d = reinterpret_cast<uint64_t>(dev_ptr_out);
  
  // Set the kernel launch arguments
  Kernel::LaunchArg arg;
  arg.type = Kernel::ArgType::T_layer_dynamic_info;
  arg.value.layer_dynamic_info = layer_info;
  auto args = std::vector<Kernel::LaunchArg>({arg});
  // Create a KernelLaunch object from this kernel
  auto launch = kernel.createKernelLaunch(args);
  // Do the actual launch and wait for the results
  auto launch_res = launch->launchBlocking(&dev_->defaultStream());
  ASSERT_EQ(launch_res, etrtSuccess);

  // Buffer to read out the data from the device
  std::vector<uint64_t> data(NUM_SHIRES * NUM_MINIONS_PER_SHIRE * CACHE_LINE_SIZE, 0xEEEEEEEEEEEEEEEEULL);

  // Read the data from the device
  auto res = dev_->memcpy(data.data(), dev_ptr_out, NUM_SHIRES * NUM_MINIONS_PER_SHIRE * CACHE_LINE_SIZE * sizeof(uint64_t), etrtMemcpyDeviceToHost);
  ASSERT_EQ(res, etrtSuccess);

  // The expected data should be the minions of each hart that finished in the beginning of the cache
  // line plus the result. Possible errors are "0" instead of minion_id, or the initial value of the data array
  auto data_ok = true;
  printf ("Minions finished: ");
  for (uint64_t minion_id=0; minion_id < NUM_SHIRES * NUM_MINIONS_PER_SHIRE; minion_id++) {
    data_ok &= (data[CACHE_LINE_SIZE * minion_id] == minion_id); 
    printf ("%lu ", data[CACHE_LINE_SIZE * minion_id]);
    if (((minion_id+1) % 32) == 0) {
      printf ("\n");
    }
  }

  ASSERT_EQ(data_ok, true);

  // Try to reset device ?
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
