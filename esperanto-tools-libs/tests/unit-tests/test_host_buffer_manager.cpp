//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------
#include <device-layer/IDeviceLayerFake.h>
#pragma GCC diagnostic push
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wkeyword-macro"
#endif
#define private public
#pragma GCC diagnostic pop
#include "dma/HostBufferManager.h"
#undef private

#include "runtime/IRuntime.h"
#include <algorithm>
#include <gtest/gtest.h>
#include <hostUtils/logging/Logging.h>
#include <numeric>
#include <random>
#include <vector>
using namespace rt;

class HostBufferManagerF : public ::testing::Test {
public:
  void SetUp() override {
    runtime_ = IRuntime::create(&deviceLayer_);
    hbm_ = std::make_unique<HostBufferManager>(runtime_.get(), DeviceId{0});
    hb_ = std::make_unique<HostBuffer>(runtime_->allocateDmaBuffer(DeviceId{0}, 1024, true));
  }
  void TearDown() override {
    EXPECT_NO_THROW({ allocations_.clear(); });
    ASSERT_EQ(hb_->getMaxSize(), rt::kPageSize * rt::kNumPages);
    for (const auto& hm : hbm_->hostBuffers_) {
      ASSERT_EQ(hm->getMaxSize(), rt::kPageSize * rt::kNumPages);
    }
  }
  dev::IDeviceLayerFake deviceLayer_;
  rt::RuntimePtr runtime_;
  std::unique_ptr<HostBuffer> hb_;
  std::vector<HostAllocation> allocations_;
  std::unique_ptr<HostBufferManager> hbm_;
  uint32_t numInitialBuffers = 4;
};

TEST_F(HostBufferManagerF, simple) {
  EXPECT_THROW({ hb_->alloc(0); }, rt::Exception);
  EXPECT_THROW({ hb_->alloc(rt::kNumPages * kPageSize + 1); }, rt::Exception);
  EXPECT_NO_THROW({ hb_->alloc(rt::kNumPages * kPageSize); });
}

TEST_F(HostBufferManagerF, alloc_all_pages) {
  EXPECT_NO_THROW({
    for (int i = 0; i < rt::kNumPages; ++i) {
      allocations_.emplace_back(hb_->alloc(rt::kPageSize));
    }
  });
  EXPECT_THROW({ allocations_.emplace_back(hb_->alloc(1)); }, rt::Exception);
  allocations_.pop_back();
  EXPECT_NO_THROW({ allocations_.emplace_back(hb_->alloc(1)); });
}

TEST_F(HostBufferManagerF, alloc_free_loop) {
  std::default_random_engine e1(12389);
  for (int i = 0; i < 100000; ++i) {
    auto freeSpace = hb_->getMaxSize();
    if (freeSpace == 0) { // if no freeSpace, then free a random allocation
      auto rndIndex = std::uniform_int_distribution<long>{0, static_cast<long>(allocations_.size()) - 1}(e1);
      auto toBeDestroyed = begin(allocations_) + rndIndex;
      allocations_.erase(toBeDestroyed);
      freeSpace = hb_->getMaxSize();
      ASSERT_GT(freeSpace, 0);
    }
    std::uniform_int_distribution<size_t> uniform_dist(1U, freeSpace);
    auto size = uniform_dist(e1);
    allocations_.emplace_back(hb_->alloc(size));
    auto oldFreeSpace = freeSpace;
    freeSpace = hb_->getMaxSize();
    ASSERT_GT(oldFreeSpace, freeSpace);
  }
}

TEST_F(HostBufferManagerF, alloc_more_than_1_buffer) {
  auto allocSize = rt::kPageSize * rt::kNumPages * 3 / 2;
  auto allocations = hbm_->alloc(allocSize);
  EXPECT_EQ(allocations.size(), 2U);
  EXPECT_LE(allocSize, std::accumulate(begin(allocations), end(allocations), 0ULL,
                                       [](auto acum, const auto& alloc) { return acum + alloc.getSize(); }));
  EXPECT_EQ(hbm_->hostBuffers_[0]->getMaxSize(), 0U);

  // save hostBuffers_[1] size because we are using it again
  auto hb1Size = hbm_->hostBuffers_[1]->getMaxSize();
  EXPECT_GT(hb1Size, 0U);
  auto otherAlloc = hbm_->alloc(1U);
  ASSERT_GE(otherAlloc.size(), 1U);
  EXPECT_GT(hb1Size, hbm_->hostBuffers_[1]->getMaxSize());
  EXPECT_GT(hbm_->hostBuffers_[1]->getMaxSize(), 0U);
  allocations.emplace_back(std::move(otherAlloc[0]));
  allocations.clear();
  EXPECT_EQ(hbm_->hostBuffers_[0]->getMaxSize(), rt::kPageSize * rt::kNumPages);
  EXPECT_EQ(hbm_->hostBuffers_[1]->getMaxSize(), rt::kPageSize * rt::kNumPages);
  ASSERT_EQ(hbm_->hostBuffers_.size(), numInitialBuffers);
}

int main(int argc, char** argv) {
  logging::LoggerDefault logger_;
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
