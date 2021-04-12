#include <algorithm>
#include <device-layer/IDeviceLayerMock.h>
#include <glog/logging.h>
#include "Utils.h"
#include <thread>

using namespace testing;
void run_stress_mem(rt::IRuntime* runtime, size_t bytes, int transactions, int streams, int threads, bool check_results=true) {
  std::vector<std::thread> threads_;

  for (int i = 0; i < threads; ++i) {
    threads_.emplace_back([=]{
      auto dev = runtime->getDevices()[0];
      std::vector<rt::StreamId> streams_(streams);
      std::vector<std::vector<std::byte>> host_src(transactions);
      std::vector<std::vector<std::byte>> host_dst(transactions);
      std::vector<void*> dev_mem(transactions);
      for (int j = 0; j < streams; ++j) {
        streams_[j] = runtime->createStream(dev);
        for (int k = 0; k < transactions / streams; ++k) {
          auto idx = k + j * transactions / streams;
          host_src[idx] = std::vector<std::byte>(bytes);
          host_dst[idx] = std::vector<std::byte>(bytes);
          //put random junk
          for (auto& v : host_src[idx]) {
            v = static_cast<std::byte>(rand() % 256);
          }
          dev_mem[idx] = runtime->mallocDevice(dev, bytes);
          runtime->memcpyHostToDevice(streams_[j], host_src[idx].data(), dev_mem[idx], bytes);
          runtime->memcpyDeviceToHost(streams_[j], dev_mem[idx], host_dst[idx].data(), bytes);
        }
      }
      for (int j = 0; j < streams; ++j) {
        runtime->waitForStream(streams_[j]);
        if (check_results) {
          for (int k = 0; k < transactions / streams; ++k) {
            auto idx = k + j * transactions / streams;
            runtime->freeDevice(dev, dev_mem[idx]);
            ASSERT_THAT(host_dst[idx], ElementsAreArray(host_src[idx]));
          }
        }
        runtime->destroyStream(streams_[j]);        
      };
      for (auto m: dev_mem) {
          runtime->freeDevice(dev, m);
      }
    });
  }
  for (auto& t: threads_) {
    t.join();
  }
}

class SysEmu: public Fixture {
public:
  SysEmu() {
    auto deviceLayer = dev::IDeviceLayer::createSysEmuDeviceLayer(getDefaultOptions());
    init(std::move(deviceLayer));
  }
};


TEST_F(SysEmu, 1KB_1_memcpys_1stream_20thread) {
  run_stress_mem(runtime_.get(), 1<<10, 1, 1, 20);
}

TEST_F(SysEmu, 1KB_1_memcpys_1stream_30thread) {
  run_stress_mem(runtime_.get(), 1<<10, 1, 1, 30);
}

TEST_F(SysEmu, 1KB_1_memcpys_1stream_50thread) {
  run_stress_mem(runtime_.get(), 1<<10, 1, 1, 50);
}

TEST_F(SysEmu, 1KB_1_memcpys_1stream_75thread) {
  run_stress_mem(runtime_.get(), 1<<10, 1, 1, 75);
}

/*TEST_F(SysEmu, 1KB_1_memcpys_1stream_100thread) {
  run_stress_mem(runtime_.get(), 1<<10, 1, 1, 100);
}*/

TEST_F(SysEmu, 1KB_100_memcpys_1stream_1thread) {
  run_stress_mem(runtime_.get(), 1 << 10, 1e2, 1, 1);
}

TEST_F(SysEmu, 1KB_100_memcpys_100stream_1thread) {
  run_stress_mem(runtime_.get(), 1<<10, 100, 100, 1);
}

TEST_F(SysEmu, 1M_20_memcpys_2stream_1thread) {
  run_stress_mem(runtime_.get(), 1<<20, 20, 2, 1);
}

TEST_F(SysEmu, 1M_20_memcpys_2stream_2thread) {
  run_stress_mem(runtime_.get(), 1<<20, 20, 2, 2);
}

TEST_F(SysEmu, 1KB_100_memcpys_2stream_2thread) {
  run_stress_mem(runtime_.get(), 1 << 10, 1e2, 2, 2);
}

TEST_F(SysEmu, 1KB_1_memcpys_1stream_1thread) {
  run_stress_mem(runtime_.get(), 1<<10, 1, 1, 1);
}

TEST_F(SysEmu, 1KB_1_memcpys_1stream_2thread) {
  run_stress_mem(runtime_.get(), 1<<10, 1, 1, 2);
}

TEST_F(SysEmu, 1KB_6_memcpys_1stream_1thread) {
  run_stress_mem(runtime_.get(), 1<<10, 6, 1, 1);
}

TEST_F(SysEmu, 1KB_6_memcpys_1stream_2thread) {
  run_stress_mem(runtime_.get(), 1<<10, 6, 1, 2);
}

TEST_F(SysEmu, 1KB_6_memcpys_2stream_1thread) {
  run_stress_mem(runtime_.get(), 1<<10, 6, 2, 1);
}

TEST_F(SysEmu, 1KB_6_memcpys_2stream_2thread) {
  run_stress_mem(runtime_.get(), 1<<10, 6, 2, 2);
}

TEST_F(SysEmu, 1KB_50_memcpys_10stream_2thread) {
  run_stress_mem(runtime_.get(), 1<<10, 50, 10, 2);
}

TEST_F(SysEmu, 1KB_2_memcpys_2stream_1thread) {
  run_stress_mem(runtime_.get(), 1<<10, 2, 2, 1);
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
