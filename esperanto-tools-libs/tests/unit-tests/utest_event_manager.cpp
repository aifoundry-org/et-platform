//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "runtime/IRuntime.h"
#include <chrono>
#include <cstdio>
#include <glog/logging.h>
#include <gtest/gtest.h>
#include <thread>
#include <functional>
#include <atomic>
#define private public
#include "EventManager.h"

using namespace rt;

class EventManagerF : public ::testing::Test {
public:
  void TearDown() override {
    // ensure there are not missing events
    ASSERT_TRUE(em_.onflyEvents_.empty());
    ASSERT_TRUE(em_.blockedThreads_.empty());
  }

  std::thread createAndBlockThread(
    EventId evt, bool detach, std::function<void()> functor) {
    auto t = std::thread([=]() {
      VLOG(5) << "Thread " << std::this_thread::get_id() << " blocking.";
      em_.blockUntilDispatched(evt);
      functor();
      VLOG(5) << "Thread " << std::this_thread::get_id() << " after block.";
    });
    if (detach) {
      t.detach();
    }
    return t;
  };

  void createBlockedThreadsAndEvents(int numEvents, int numThreads, bool detach = true, std::function<void()> functor = [](){}) {
    ASSERT_TRUE(events_.empty());
    ASSERT_TRUE(threads_.empty());
    for (int i = 0; i < numEvents; ++i) {
      events_.emplace_back(em_.getNextId());
    }
    for (int i = 0; i < numThreads; ++i) {
      threads_.emplace_back(createAndBlockThread(events_[i % numEvents], detach, functor));
    }
  }

  std::vector<EventId> events_;
  std::vector<std::thread> threads_;
  EventManager em_;
};

TEST_F(EventManagerF, simple) {
  EventId ev{8193};
  //any event not onfly will be considered dispatched
  EXPECT_TRUE(em_.isDispatched(ev));

  ev = em_.getNextId();
  //we just asked for the event so it shouldn't be dispatched
  EXPECT_FALSE(em_.isDispatched(ev));
  em_.dispatch(ev);
  EXPECT_TRUE(em_.isDispatched(ev));

  //we can't dispatch an event that its not onfly
  EXPECT_THROW(em_.dispatch(ev), Exception);
}

TEST_F(EventManagerF, severalEvents) {
  std::vector<EventId> events;
  for (int i = 0; i < 100; ++i) {
    events.emplace_back(em_.getNextId());
    EXPECT_FALSE(em_.isDispatched(events.back()));
  }
  // dispatch only even
  for (int i = 0; i < 100; i += 2) {
    em_.dispatch(events[i]);
  }

  // check even are dispatched, odd aren't
  for (int i = 0; i < 100; i += 2) {
    EXPECT_TRUE(em_.isDispatched(events[i]));
    EXPECT_FALSE(em_.isDispatched(events[i + 1]));
  }

  EXPECT_EQ(em_.onflyEvents_.size(), 50);

  // also dispatch odd events
  for (int i = 1; i < 100; i += 2) {
    em_.dispatch(events[i]);
  }

  // check all are dispatched
  for (int i = 0; i < 100; ++i) {
    EXPECT_TRUE(em_.isDispatched(events[i]));
  }
}

TEST_F(EventManagerF, blockingThreads) {
  int nEvents = 10;
  int nThreads = 10000;
  std::atomic<int> unblockedThreads = 0;

  createBlockedThreadsAndEvents(nEvents, nThreads, false, [&](){
    unblockedThreads.fetch_add(1);
  });

  for (auto evt : events_) {
    int prev = unblockedThreads.load();
    em_.dispatch(evt);
    int after = unblockedThreads.load();
    EXPECT_GE(after, prev);
  }

  for (auto& t : threads_) {
    t.join();
  }
  EXPECT_EQ(unblockedThreads.load(), nThreads);
}

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  google::InstallFailureSignalHandler();
  google::SetCommandLineOption("GLOG_minloglevel", "0");
  google::SetCommandLineOption("GLOG_logtostderr", "1");
  FLAGS_minloglevel = 0;
  FLAGS_logtostderr = 1;
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
