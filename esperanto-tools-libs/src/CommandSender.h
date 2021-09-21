/*-------------------------------------------------------------------------
 * Copyright (C) 2021, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/

#pragma once
#include <condition_variable>
#include <cstddef>
#include <device-layer/IDeviceLayer.h>
#include <list>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace rt {
struct Command {
  void enable();
  std::vector<std::byte> commandData_;
  class CommandSender& parent_;
  bool isDma_ = false;
  bool isEnabled_ = false;
};
class CommandSender {

public:
  explicit CommandSender(dev::IDeviceLayer& deviceLayer, int deviceId, int sqIdx);
  ~CommandSender();
  // returns a pointer to the recently inserted Command. This is useful, for example to change the isEnabled_ flag if
  // needed. We are using a deque to store the commands, and we only pop and emplace; so references won't be invalidated
  Command* send(Command command);
  void enable(Command& command);

private:
  CommandSender(const CommandSender&) = delete;
  CommandSender& operator=(const CommandSender&) = delete;
  CommandSender(CommandSender&&) = delete;
  CommandSender& operator=(CommandSender&&) = delete;

  void runnerFunc();
  std::mutex mutex_;
  std::queue<Command> commands_;
  std::thread runner_;
  std::condition_variable condVar_;
  dev::IDeviceLayer& deviceLayer_;
  int deviceId_;
  int sqIdx_;
  bool running_ = true;
};
} // namespace rt