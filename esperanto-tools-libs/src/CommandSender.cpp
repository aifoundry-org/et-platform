/*-------------------------------------------------------------------------
 * Copyright (C) 2021, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/
#include "CommandSender.h"
#include "Utils.h"
#include <functional>
#include <iomanip>
#include <mutex>
#include <optional>
#include <thread>
using namespace rt;

std::string commandString(const std::vector<std::byte>& commandData) {
  std::stringstream ss;
  ss << "Sent command: 0x";
  for (auto byte : commandData) {
    ss << std::hex << std::setfill('0') << std::setw(2) << std::to_integer<int>(byte);
  }
  return ss.str();
}

template <typename List> std::string getCommandPtrs(List& commands) {
  std::stringstream ss;
  ss << "|";
  for (auto it = begin(commands); it != end(commands); ++it) {
    ss << std::hex << &*it << " enabled? " << (it->isEnabled_ ? "True" : "False") << "|";
  }
  return ss.str();
}

CommandSender::CommandSender(dev::IDeviceLayer& deviceLayer, profiling::IProfilerRecorder& profiler, int deviceId,
                             int sqIdx)
  : deviceLayer_(deviceLayer)
  , profiler_(profiler)
  , deviceId_(deviceId)
  , sqIdx_(sqIdx) {
  runner_ = std::thread{std::bind(&CommandSender::runnerFunc, this)};
}
void CommandSender::setOnCommandSentCallback(CommandSentCallback callback) {
  std::lock_guard lock(mutex_);
  callback_ = std::move(callback);
}

void CommandSender::send(Command command) {
  std::unique_lock lock(mutex_);
  RT_VLOG(MID) << "Adding command (send) " << static_cast<int>(command.eventId_) << " to the send list. Enabled? "
               << (command.isEnabled_ ? "True" : "False");
  commands_.emplace_back(std::move(command));
  lock.unlock();
  condVar_.notify_one();
}

void CommandSender::sendBefore(EventId existingCommand, Command command) {
  std::unique_lock lock(mutex_);
  RT_VLOG(MID) << "Adding command (sendBefore) " << static_cast<int>(command.eventId_) << " to the send list. Enabled? "
               << (command.isEnabled_ ? "True" : "False");
  auto it = std::find_if(begin(commands_), end(commands_),
                         [existingCommand](const Command& elem) { return elem.eventId_ == existingCommand; });
  if (it == end(commands_)) {
    throw Exception("Trying to send a command before a non-existing command");
  }
  commands_.emplace(it, std::move(command));
  lock.unlock();
  condVar_.notify_one();
}

void Command::enable() {
  parent_.enable(eventId_);
}

void CommandSender::enable(EventId event) {
  std::unique_lock lock(mutex_);
  auto it =
    std::find_if(begin(commands_), end(commands_), [event](const Command& elem) { return elem.eventId_ == event; });
  if (it == end(commands_)) {
    throw Exception("Trying to enable a non-existing command");
  }
  it->isEnabled_ = true;
  lock.unlock();
  condVar_.notify_one();
}

CommandSender::~CommandSender() {
  RT_LOG(INFO) << "Destroying commandSender for device: " << deviceId_ << " SQ:" << sqIdx_;
  running_ = false;
  condVar_.notify_one();
  runner_.join();
}

std::optional<EventId> CommandSender::getTopPrioritaryCommand() const {
  std::lock_guard lock(mutex_);
  auto it = std::find_if(begin(commands_), end(commands_), [](const auto& c) { return !c.isEnabled_; });
  std::optional<EventId> result;
  if (it != end(commands_)) {
    result = it->eventId_;
  }
  return result;
}

void CommandSender::cancel(EventId event) {
  std::unique_lock lock(mutex_);
  auto it =
    std::find_if(begin(commands_), end(commands_), [event](const Command& elem) { return elem.eventId_ == event; });
  if (it != end(commands_)) {
    commands_.erase(it);
    lock.unlock();
    condVar_.notify_one();
  } else {
    RT_LOG(WARNING) << "Trying to remove a command which doesnt exist.";
  }
}

void CommandSender::runnerFunc() {

  while (running_) {
    std::unique_lock lock(mutex_);
    if (!commands_.empty() && commands_.front().isEnabled_) {
      auto& cmd = commands_.front();
      if (deviceLayer_.sendCommandMasterMinion(deviceId_, sqIdx_, cmd.commandData_.data(), cmd.commandData_.size(),
                                               cmd.isDma_)) {
        RT_VLOG(LOW) << ">>> Command sent: " << commandString(cmd.commandData_);

        profiling::ProfileEvent event(profiling::Type::Instant, profiling::Class::CommandSent);
        event.setEvent(cmd.eventId_);
        event.setStream(StreamId(sqIdx_));
        event.setDeviceId(DeviceId(deviceId_));
        profiler_.record(event);

        if (callback_) {
          callback_(&cmd);
        }
        commands_.pop_front();
      } else {
        lock.unlock();
        RT_LOG(INFO) << "Submission queue " << sqIdx_
                     << " is full. Can't send command now, blocking the thread till an event has been dispatched.";
        uint64_t sq_bitmap = 0;
        bool cq_available = false;
        while (!(sq_bitmap & (1UL << sqIdx_))) {
          deviceLayer_.waitForEpollEventsMasterMinion(deviceId_, sq_bitmap, cq_available, std::chrono::seconds(1));
          if (!running_) {
            break;
          }
          if (sq_bitmap == 0 && !cq_available) {
            RT_VLOG(LOW) << "Didn't get any epoll events (timedout). Trying to send the command again.";
            break;
          }
        }
      }
    } else {
      // check if we are running and if there are any commands
      condVar_.wait(lock, [this] { return !running_ || (!commands_.empty() && commands_.front().isEnabled_); });
    }
  }
}
