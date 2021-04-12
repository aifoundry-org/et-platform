#include "Utils.h"
#include <algorithm>
#include <device-layer/IDeviceLayerMock.h>
#include <glog/logging.h>
#include <thread>

using namespace testing;
bool areEqual(int one, int other, int index, int thread, int stream) {
  (void)index;
  (void)thread;
  (void)stream;
  return one == other;
}
class SysEmu : public Fixture {
public:
  SysEmu() {
    auto deviceLayer = dev::IDeviceLayer::createSysEmuDeviceLayer(getDefaultOptions());
    init(std::move(deviceLayer));
    kernel_ = loadKernel("add_vector.elf");
  }
  void run_stress_kernel(size_t elems, int num_executions, int streams, int threads, bool check_results = true) {
    std::vector<std::thread> threads_;

    for (int i = 0; i < threads; ++i) {
      threads_.emplace_back([=] {
        auto dev = devices_[0];
        std::vector<rt::StreamId> streams_(streams);
        std::vector<std::vector<int>> host_src1(num_executions);
        std::vector<std::vector<int>> host_src2(num_executions);
        std::vector<std::vector<int>> host_dst(num_executions);
        std::vector<void*> dev_mem_src1(num_executions);
        std::vector<void*> dev_mem_src2(num_executions);
        std::vector<void*> dev_mem_dst(num_executions);
        for (int j = 0; j < streams; ++j) {
          streams_[j] = runtime_->createStream(dev);
          for (int k = 0; k < num_executions / streams; ++k) {
            DLOG(INFO) << "Num execution: " << k;
            auto idx = k + j * num_executions / streams;
            host_src1[idx] = std::vector<int>(elems);
            host_src2[idx] = std::vector<int>(elems);
            host_dst[idx] = std::vector<int>(elems);
            // put random junk
            for (auto& v : host_src1[idx]) {
              v = rand();
            }
            for (auto& v : host_src2[idx]) {
              v = rand();
            }
            dev_mem_src1[idx] = runtime_->mallocDevice(dev, elems * sizeof(int));
            dev_mem_src2[idx] = runtime_->mallocDevice(dev, elems * sizeof(int));
            dev_mem_dst[idx] = runtime_->mallocDevice(dev, elems * sizeof(int));
            runtime_->memcpyHostToDevice(streams_[j], host_src1[idx].data(), dev_mem_src1[idx], elems * sizeof(int));
            runtime_->memcpyHostToDevice(streams_[j], host_src2[idx].data(), dev_mem_src2[idx], elems * sizeof(int));
            struct {
              void* src1;
              void* src2;
              void* dst;
              int elements;
            } params{dev_mem_src1[idx], dev_mem_src2[idx], dev_mem_dst[idx], static_cast<int>(elems)};
            runtime_->kernelLaunch(streams_[j], kernel_, &params, sizeof(params), 0x1);
            runtime_->memcpyDeviceToHost(streams_[j], dev_mem_dst[idx], host_dst[idx].data(), elems * sizeof(int));
          }
        }
        for (int j = 0; j < streams; ++j) {
          runtime_->waitForStream(streams_[j]);
          if (check_results) {
            for (int k = 0; k < num_executions / streams; ++k) {
              auto idx = k + j * num_executions / streams;
              ASSERT_LT(idx, num_executions);
              runtime_->freeDevice(dev, dev_mem_src1[idx]);
              runtime_->freeDevice(dev, dev_mem_src2[idx]);
              runtime_->freeDevice(dev, dev_mem_dst[idx]);
              for (int l = 0; l < elems; ++l) {
                ASSERT_PRED5(areEqual, host_dst[idx][l], host_src1[idx][l] + host_src2[idx][l], l, i, j);
              }
            }
          }
          runtime_->destroyStream(streams_[j]);
        };
      });
    }
    for (auto& t : threads_) {
      t.join();
    }
  }
  rt::KernelId kernel_;
};

TEST_F(SysEmu, 256_ele_10_exe_10_st_2_th) {
  run_stress_kernel(1 << 8, 10, 10, 2);
}

TEST_F(SysEmu, 1024_ele_1_exe_1_st_1_th) {
  run_stress_kernel(1 << 10, 1, 1, 1);
}

TEST_F(SysEmu, 1024_ele_1_exe_1_st_2_th) {
  run_stress_kernel(1 << 10, 1, 1, 2);
}

TEST_F(SysEmu, 256_ele_10_exe_1_st_1_th) {
  run_stress_kernel(1 << 8, 10, 1, 1);
}

TEST_F(SysEmu, 1024_ele_100_exe_1_st_2_th) {
  run_stress_kernel(1 << 10, 100, 1, 2);
}

TEST_F(SysEmu, 128_ele_1_exe_1_st_2_th) {
  run_stress_kernel(1 << 7, 1, 1, 2);
}

TEST_F(SysEmu, 64_ele_1_exe_1_st_20_th) {
  run_stress_kernel(1 << 6, 1, 1, 20);
}

TEST_F(SysEmu, 64_ele_1_exe_1_st_15_th) {
  run_stress_kernel(1 << 6, 1, 1, 15);
}

TEST_F(SysEmu, 64_ele_1_exe_1_st_25_th) {
  run_stress_kernel(1 << 6, 1, 1, 25);
}

TEST_F(SysEmu, 64_ele_1_exe_1_st_35_th) {
  run_stress_kernel(1 << 6, 1, 1, 35);
}

TEST_F(SysEmu, 64_ele_1_exe_1_st_45_th) {
  run_stress_kernel(1 << 6, 1, 1, 45);
}

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  google::InstallFailureSignalHandler();
  google::SetCommandLineOption("GLOG_minloglevel", "0");
  // Force logging in stderr and set min logging level
  FLAGS_minloglevel = 0;
  FLAGS_logtostderr = 1;
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
