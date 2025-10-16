/*-------------------------------------------------------------------------
 * Copyright (c) 2025 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------*/

#include "Utils.h"
#include "gtest/gtest.h"
#include <chrono>
#include <thread>

TEST(SpinLock, simple) {
  std::mutex m;
  SpinLock lock(m);
}

TEST(SpinLock, 1000_threads) {
  std::mutex m;
  int count = 0;
  int numthreads = 1000;
  std::vector<std::thread> threads;
  for (auto j = 0; j < numthreads; ++j) {
    threads.emplace_back([&m, &count] {
      SpinLock lock(m);
      count++;
    });
  }
  for (auto& t : threads) {
    t.join();
  }
  threads.clear();
  EXPECT_EQ(count, numthreads);
  for (auto j = 0; j < numthreads; ++j) {
    threads.emplace_back([&m, &count] {
      SpinLock lock(m, std::chrono::milliseconds(1));
      count--;
    });
  }
  for (auto& t : threads) {
    t.join();
  }
  EXPECT_EQ(count, 0);
}

TEST(SpinLock, force_contention) {
  std::mutex m;
  int count = 0;
  int numthreads = 10;
  std::vector<std::thread> threads;
  for (auto j = 0; j < numthreads; ++j) {
    threads.emplace_back([&m, &count] {
      SpinLock lock(m);
      count++;
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    });
  }
  for (auto& t : threads) {
    t.join();
  }
  EXPECT_EQ(count, numthreads);
}

int main(int argc, char** argv) {
  logging::LoggerDefault logger_;
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
