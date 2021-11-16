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

CommandSender::CommandSender(dev::IDeviceLayer& deviceLayer, int deviceId, int sqIdx)
  : deviceLayer_(deviceLayer)
  , deviceId_(deviceId)
  , sqIdx_(sqIdx) {
  runner_ = std::thread{std::bind(&CommandSender::runnerFunc, this)};
}
void CommandSender::setOnCommandSentCallback(CommandSentCallback callback) {
  std::lock_guard lock(mutex_);
  callback_ = std::move(callback);
}
Command* CommandSender::send(Command command) {
  std::unique_lock lock(mutex_);
  commands_.emplace_back(std::move(command));
  auto res = &commands_.back();
  RT_VLOG(MID) << "Adding command " << std::hex << res << " to the send queue. Enabled? "
               << (res->isEnabled_ ? "True" : "False");
  lock.unlock();
  condVar_.notify_one();
  return res;
}

Command* CommandSender::sendAfter(Command* existingCommand, Command command) {
  std::lock_guard lock(mutex_);
  auto it = std::find_if(begin(commands_), end(commands_), [=](const auto& elem) { return &elem == existingCommand; });
  if (it == end(commands_)) {
    throw Exception("Trying to send a command after a non-existing command");
  }
  return &*commands_.emplace(++it, std::move(command));
}

void Command::enable() {
  parent_.enable(*this);
}

void CommandSender::enable(Command& command) {
  std::unique_lock lock(mutex_);
  RT_VLOG(MID) << "Enabling command " << std::hex << &command;
  command.isEnabled_ = true;
  lock.unlock();
  condVar_.notify_one();
}

CommandSender::~CommandSender() {
  RT_LOG(INFO) << "Destroying commandSender for device: " << deviceId_ << " SQ:" << sqIdx_;
  running_ = false;
  condVar_.notify_one();
  runner_.join();
}

void CommandSender::runnerFunc() {

  while (running_) {
    std::unique_lock lock(mutex_);
    if (!commands_.empty() && commands_.front().isEnabled_) {
      auto& cmd = commands_.front();
      if (deviceLayer_.sendCommandMasterMinion(deviceId_, sqIdx_, cmd.commandData_.data(), cmd.commandData_.size(),
                                               cmd.isDma_)) {
        RT_VLOG(LOW) << ">>> Command sent: " << commandString(cmd.commandData_);
        if (callback_) {
          callback_(&cmd);
        }
        commands_.pop_front();
      } else {
        lock.unlock();
        RT_LOG(INFO) << "Submission queue " << sqIdx_
                     << " is full. Can't send command now, blocking the thread till an event has been dispatched.";
        uint64_t sq_bitmap;
        bool cq_available;
        deviceLayer_.waitForEpollEventsMasterMinion(deviceId_, sq_bitmap, cq_available, std::chrono::seconds(1));
        if (sq_bitmap & (1UL << sqIdx_)) {
          RT_LOG(WARNING) << "Submission queue " << sqIdx_
                          << " is still unavailable after waitForEpoll, trying again nevertheless";
        } else {
          RT_VLOG(LOW) << "Submission queue " << sqIdx_ << " available.";
        }
      }
    } else {
      // check if we are running and if there are any commands
      condVar_.wait(lock, [this] { return !running_ || (!commands_.empty() && commands_.front().isEnabled_); });
    }
  }
}