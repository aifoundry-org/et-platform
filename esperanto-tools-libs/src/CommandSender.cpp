/*-------------------------------------------------------------------------
 * Copyright (c) 2025 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------*/
#include "CommandSender.h"

#include "Utils.h"

#include <device-layer/IDeviceLayer.h>

#include <functional>
#include <iomanip>
#include <mutex>
#include <optional>
#include <thread>

using namespace rt;

std::string commandString(const std::vector<std::byte>& commandData);

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

CommandSender::CommandSender(dev::IDeviceLayer& deviceLayer, profiling::IProfilerRecorder* profiler, int deviceId,
                             int sqIdx)
  : deviceLayer_(deviceLayer)
  , profiler_(profiler)
  , deviceId_(deviceId)
  , sqIdx_(sqIdx) {
  runner_ = std::thread{std::bind(&CommandSender::runnerFunc, this)};
}
void CommandSender::setOnCommandSentCallback(CommandSentCallback callback) {
  SpinLock lock(mutex_);
  callback_ = std::move(callback);
}

void CommandSender::send(Command command) {
  SpinLock lock(mutex_);
  RT_VLOG(MID) << "Adding command (send) " << static_cast<int>(command.eventId_) << " to the send list. Enabled? "
               << (command.isEnabled_ ? "True" : "False");
  commands_.emplace_back(std::move(command));
  lock.unlock();
  condVar_.notify_one();
}

void CommandSender::sendBefore(EventId existingCommand, Command command) {
  SpinLock lock(mutex_);
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

void CommandSender::setCommandData(EventId command, std::vector<std::byte> data) {
  SpinLock lock(mutex_);
  auto it =
    std::find_if(begin(commands_), end(commands_), [command](const Command& elem) { return elem.eventId_ == command; });
  if (it == end(commands_)) {
    throw Exception("Trying to set data into  a non-existing command");
  }
  it->commandData_ = std::move(data);
}

void CommandSender::enable(EventId event) {
  RT_VLOG(MID) << "Enabling command " << static_cast<int>(event);
  SpinLock lock(mutex_);
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
  RT_LOG(INFO) << "Destroying commandSender for device: " << deviceId_ << " SQ: " << sqIdx_;
  running_ = false;
  condVar_.notify_one();
  runner_.join();
}

std::optional<EventId> CommandSender::getFirstDmaCommand() const {
  SpinLock lock(mutex_);
  auto it = std::find_if(begin(commands_), end(commands_), [](const auto& c) { return c.isDma_; });
  std::optional<EventId> result;
  if (it != end(commands_)) {
    result = it->eventId_;
  }
  return result;
}

void CommandSender::cancel(EventId event) {
  SpinLock lock(mutex_);
  RT_VLOG(MID) << "Cancel command " << static_cast<int>(event);
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
  profiling::IProfilerRecorder::setCurrentThreadName("Device " + std::to_string(deviceId_) + " command sender");

  while (running_) {
    try {
      SpinLock lock(mutex_);
      if (!commands_.empty() && commands_.front().isEnabled_) {
        auto& cmd = commands_.front();
        dev::CmdFlagMM flags;
        flags.isDma_ = cmd.isDma_;
        flags.isHpSq_ = false;
        flags.isP2pDma_ = cmd.isP2P_;
        RT_VLOG(MID) << ">>> Sending command: " << commandString(cmd.commandData_) << ". DeviceID: " << deviceId_
                     << " SQ: " << sqIdx_ << " EventId: " << static_cast<int>(cmd.eventId_);
        profiling::ProfileEvent event(profiling::Type::Instant, profiling::Class::CommandSent);
        if (deviceLayer_.sendCommandMasterMinion(deviceId_, sqIdx_, cmd.commandData_.data(), cmd.commandData_.size(),
                                                 flags)) {
          RT_VLOG(LOW) << ">>> Command sent: " << commandString(cmd.commandData_) << ". DeviceID: " << deviceId_
                       << " SQ: " << sqIdx_ << " EventId: " << static_cast<int>(cmd.eventId_);

          event.setEvent(cmd.eventId_);
          event.setStream(cmd.streamId_);
          event.setDeviceId(DeviceId(deviceId_));
          event.setParentId(cmd.parentEventId_);
          profiler_->record(event);

          if (callback_) {
            auto th = std::thread([callback = callback_, cmd, deviceId = deviceId_, threadId = nextCallbackThreadId_] {
              profiling::IProfilerRecorder::setCurrentThreadName(
                "Device " + std::to_string(deviceId) + " command sender callback thread " + std::to_string(threadId));
              callback(&cmd);
            });
            th.detach();
            nextCallbackThreadId_++;
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
        condVar_.wait(lock, [this] { return !running_ || (!commands_.empty() && commands_.front().isEnabled_); });
      }
    } catch (const std::exception& e) {
      RT_LOG(FATAL)
        << "Exception in command sender runner thread. DeviceLayer could be in a BAD STATE. Exception message: "
        << e.what();
    }
  }
}
