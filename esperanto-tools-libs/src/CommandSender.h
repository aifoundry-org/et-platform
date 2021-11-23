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

#include "ProfilerImp.h"
#include "runtime/Types.h"

#include <device-layer/IDeviceLayer.h>

#include <condition_variable>
#include <cstddef>
#include <functional>
#include <list>
#include <mutex>
#include <thread>
#include <vector>

namespace rt {
struct Command {
  void enable();
  std::vector<std::byte> commandData_;
  class CommandSender& parent_;
  EventId evt_;
  bool isDma_ = false;
  bool isEnabled_ = false;
};

class CommandSender {
public:
  using CommandSentCallback = std::function<void(Command*)>;
  explicit CommandSender(dev::IDeviceLayer& deviceLayer, profiling::IProfilerRecorder& profiler, int deviceId,
                         int sqIdx);
  ~CommandSender();

  // returns true if there is a higher prioritary command which is disabled.
  bool IsThereAnyPreviousDisabledCommand(const Command* command) const;

  // returns a pointer to the recently inserted Command. This is useful, for example to change the isEnabled_ flag if
  // needed. We are using a deque to store the commands, and we only pop and emplace; so references won't be invalidated
  Command* send(Command command);

  // sendBefore works similar to send, but receives an existing Command as a parameter; the new Command will be inserted
  // before that existing command (so will be executed later)
  Command* sendBefore(const Command* existingCommand, Command command);

  // if the Command is not yet running, it will be removed from the queue
  void cancel(const Command* command);

  void enable(Command& command);
  void setOnCommandSentCallback(CommandSentCallback callback);

private:
  CommandSender(const CommandSender&) = delete;
  CommandSender& operator=(const CommandSender&) = delete;
  CommandSender(CommandSender&&) = delete;
  CommandSender& operator=(CommandSender&&) = delete;

  void runnerFunc();
  mutable std::mutex mutex_;
  std::list<Command> commands_;
  std::thread runner_;
  std::condition_variable condVar_;
  dev::IDeviceLayer& deviceLayer_;
  profiling::IProfilerRecorder& profiler_;
  CommandSentCallback callback_;
  int deviceId_;
  int sqIdx_;
  bool running_ = true;
};
} // namespace rt