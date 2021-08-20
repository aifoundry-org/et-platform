//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "dma/PageManager.h"
#include "runtime/Types.h"
#include <algorithm>
#include <gtest/gtest.h>
#include <hostUtils/logging/Logging.h>
#include <random>
#include <vector>
using namespace rt;

class PageManagerF : public ::testing::Test {
public:
  void SetUp() override {
    auto allocator = [](size_t size) { return reinterpret_cast<std::byte*>(malloc(size)); };
    auto deallocator = [](std::byte* ptr) { return free(ptr); };
    pm_ = std::make_unique<PageManager>(allocator, deallocator);
  }
  void TearDown() override {
    EXPECT_NO_THROW({ allocations_.clear(); });
    ASSERT_EQ(pm_->getMaxSize(), rt::kPageSize * rt::kNumPages);
  }
  std::vector<PageGroup> allocations_;
  std::unique_ptr<PageManager> pm_;
};

TEST_F(PageManagerF, simple) {
  EXPECT_THROW({ pm_->alloc(0); }, rt::Exception);
  EXPECT_THROW({ pm_->alloc(rt::kNumPages * kPageSize + 1); }, rt::Exception);
  EXPECT_NO_THROW({ pm_->alloc(rt::kNumPages * kPageSize); });
}

TEST_F(PageManagerF, alloc_all_pages) {
  EXPECT_NO_THROW({
    for (int i = 0; i < rt::kNumPages; ++i) {
      allocations_.emplace_back(pm_->alloc(rt::kPageSize));
    }
  });
  EXPECT_THROW({ allocations_.emplace_back(pm_->alloc(1)); }, rt::Exception);
  allocations_.pop_back();
  EXPECT_NO_THROW({ allocations_.emplace_back(pm_->alloc(1)); });
}

TEST_F(PageManagerF, alloc_free_loop) {
  std::default_random_engine e1(12389);
  for (int i = 0; i < 100000; ++i) {
    auto freeSpace = pm_->getMaxSize();
    if (freeSpace == 0) { // if no freeSpace, then free a random allocation
      auto rndIndex = std::uniform_int_distribution<long>{0, static_cast<long>(allocations_.size()) - 1}(e1);
      auto toBeDestroyed = begin(allocations_) + rndIndex;
      allocations_.erase(toBeDestroyed);
      freeSpace = pm_->getMaxSize();
      ASSERT_GT(freeSpace, 0);
    }
    std::uniform_int_distribution<size_t> uniform_dist(1U, freeSpace);
    auto size = uniform_dist(e1);
    allocations_.emplace_back(pm_->alloc(size));
    auto oldFreeSpace = freeSpace;
    freeSpace = pm_->getMaxSize();
    ASSERT_GT(oldFreeSpace, freeSpace);
  }
}

int main(int argc, char** argv) {
  logging::LoggerDefault logger_;
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
