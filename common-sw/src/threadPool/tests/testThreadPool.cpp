//******************************************************************************
// Copyright (c) 2025 Ainekko, Co.
// SPDX-License-Identifier: Apache-2.0
//------------------------------------------------------------------------------

#include "hostUtils/threadPool/ThreadPool.h"
#include <atomic>
#include <chrono>
#include <gtest/gtest.h>
#include <hostUtils/logging/Logger.h>
#include <thread>
using namespace threadPool;
TEST(ThreadPool, simple) {
  bool taskExecuted = false;
  {
    ThreadPool tp(5);
    tp.pushTask([&taskExecuted] { taskExecuted = true; });
  }
  ASSERT_TRUE(taskExecuted);
}

TEST(ThreadPool, 1000tasks) {
  std::atomic<int> acum = 0;
  {
    ThreadPool tp(20, false, true);
    for (int i = 1; i <= 1000; ++i) {
      tp.pushTask([&acum, i] { acum += i; });
    }
  }
  ASSERT_EQ(acum, (1000 * 1001) / 2);
}

int main(int argc, char** argv) {
  logging::LoggerDefault logger_;
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
