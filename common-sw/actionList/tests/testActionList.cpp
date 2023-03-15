//******************************************************************************
// Copyright (C) 2021, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "actionList/ActionList.h"
#include "actionList/Runner.h"
#include <atomic>
#include <chrono>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <logging/Logger.h>
#include <thread>

#include "gtest/gtest.h"

using namespace actionList;
using namespace ::testing;

class MockAction : public actionList::IAction {
public:
  MOCK_METHOD0(update, bool());
  MOCK_METHOD0(onFinish, void());
};

TEST(ActionListTest, UpdateRemovesFinishedAction) {
  actionList::ActionList list;

  // create a mock action
  auto mockAction = std::make_unique<MockAction>();

  // expect the mock action to be updated once
  EXPECT_CALL(*mockAction, update()).WillOnce(::testing::Return(true));
  EXPECT_CALL(*mockAction, onFinish()).Times(1);

  // add the mock action to the list
  list.addAction(std::move(mockAction));
  EXPECT_EQ(list.getNumActions(), 1);

  // update the list
  list.update();

  // expect the list to be empty
  EXPECT_EQ(list.getNumActions(), 0);
}

TEST(ActionListTest, RemovesActionAfter3Updates) {
  // create a mock action that takes 3 updates to finish
  auto mockAction = std::make_unique<MockAction>();
  int calledTimes = 0;
  EXPECT_CALL(*mockAction, update()).Times(3).WillRepeatedly(InvokeWithoutArgs([&calledTimes] {
    calledTimes++;
    return calledTimes >= 3;
  }));
  EXPECT_CALL(*mockAction, onFinish()).Times(1);

  // create an action list and add the mock action to it
  ActionList actionList;
  actionList.addAction(std::move(mockAction));
  EXPECT_EQ(actionList.getNumActions(), 1);

  // update the action list 3 times
  for (int i = 0; i < 3; ++i) {
    actionList.update();
  }

  // check that the action list is empty
  EXPECT_EQ(actionList.getNumActions(), 0);
}

TEST(ActionListTest, RemovesActionAfter3UpdatesWith1ExtraActionLater) {
  // create a mock action that takes 3 updates to finish and another mock action
  auto mockAction = std::make_unique<MockAction>();
  auto mockAction2 = std::make_unique<MockAction>();
  int calledTimes = 0;
  EXPECT_CALL(*mockAction, update()).Times(3).WillRepeatedly(Invoke([&calledTimes] {
    calledTimes++;
    return calledTimes >= 3;
  }));
  EXPECT_CALL(*mockAction, onFinish()).Times(1);
  EXPECT_CALL(*mockAction2, update()).Times(1).WillOnce(Return(true));
  EXPECT_CALL(*mockAction2, onFinish()).Times(1);

  // create an action list and add the mock action to it
  ActionList actionList;
  actionList.addAction(std::move(mockAction));
  actionList.addAction(std::move(mockAction2));
  EXPECT_EQ(actionList.getNumActions(), 2);

  // update the action list 3 times
  for (int i = 0; i < 3; ++i) {
    EXPECT_EQ(actionList.getNumActions(), 2);
    actionList.update();
  }

  // both actions will be already done, so the list should be empty
  EXPECT_EQ(actionList.getNumActions(), 0);
}

// Test adding actions during update and onFinish
TEST(ActionListTest, AddActionDuringExecution) {
  ActionList actionList;
  auto action1 = std::make_unique<MockAction>();
  auto action2 = std::make_unique<MockAction>();

  // Mock expectations for action1
  EXPECT_CALL(*action1, update()).WillOnce(testing::Invoke([&] {
    // Add action2 during execution of action1's update
    actionList.addAction(std::move(action2));
    return true;
  }));
  EXPECT_CALL(*action1, onFinish()).Times(1);

  // Mock expectations for action2
  EXPECT_CALL(*action2, update()).WillOnce(testing::Return(true));
  EXPECT_CALL(*action2, onFinish()).Times(1);

  // Add action1 to the list and call update
  actionList.addAction(std::move(action1));
  actionList.update();

  // Verify that action1 and action2 were executed and removed from the list
  EXPECT_EQ(actionList.getNumActions(), 0);
}

TEST(RunnerTest, AddAction) {
  ActionList actionList;
  auto runner = std::make_unique<Runner>(std::move(actionList));

  auto mockAction = std::make_unique<MockAction>();
  EXPECT_CALL(*mockAction, update()).WillOnce(::testing::Return(true));
  EXPECT_CALL(*mockAction, onFinish()).Times(1);
  runner->addAction(std::move(mockAction));
  std::this_thread::sleep_for(std::chrono::milliseconds(1));
  runner.reset();
}
TEST(RunnerTest, AddActionABlockingAction) {
  auto mockAction = std::make_unique<MockAction>();
  int calledTimes = 0;
  EXPECT_CALL(*mockAction, update()).Times(2).WillRepeatedly(InvokeWithoutArgs([&calledTimes] {
    calledTimes++;
    return calledTimes >= 2;
  }));
  EXPECT_CALL(*mockAction, onFinish()).Times(1);
  ActionList actionList;
  actionList.addAction(std::move(mockAction));
  auto runner = std::make_unique<Runner>(std::move(actionList));

  for (int i = 0; i < 1000; ++i) {
    mockAction = std::make_unique<MockAction>();
    EXPECT_CALL(*mockAction, update()).WillOnce(::testing::Return(true));
    EXPECT_CALL(*mockAction, onFinish()).Times(1);
    runner->addAction(std::move(mockAction));
  }
  runner->update();
  std::this_thread::sleep_for(std::chrono::milliseconds(1));
  runner.reset();
}

TEST(RunnerTest, AddMultipleAction) {
  ActionList actionList;
  auto runner = std::make_unique<Runner>(std::move(actionList));
  for (int i = 0; i < 10; ++i) {
    auto mockAction = std::make_unique<MockAction>();
    EXPECT_CALL(*mockAction, update()).WillOnce(::testing::Return(true));
    EXPECT_CALL(*mockAction, onFinish()).Times(1);
    runner->addAction(std::move(mockAction));
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(1));
  runner.reset();
}

int main(int argc, char** argv) {
  logging::LoggerDefault logger_;
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
