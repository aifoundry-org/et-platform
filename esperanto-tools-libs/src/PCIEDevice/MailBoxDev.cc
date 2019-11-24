//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "MailBoxDev.h"

#include "esperanto/runtime/Support/Logging.h"

#include <cassert>
#include <thread>
#include <unistd.h>

namespace et_runtime {
namespace device {

MailBoxDev::MailBoxDev(const std::experimental::filesystem::path &char_dev)
    : CharacterDevice(char_dev) {
  mbox_max_msg_size_ = queryMboxMaxMsgSize();
  RTDEBUG << "Maximum mbox message size: " << mbox_max_msg_size_ << "\n";
}

MailBoxDev::MailBoxDev(MailBoxDev &&other)
    : CharacterDevice(std::move(other)) {}

bool MailBoxDev::ready(TimeDuration wait_time) {
  uint64_t ready = 0;
  auto start = Clock::now();
  auto end = start + wait_time;
  static const TimeDuration polling_interval = std::chrono::seconds(1);

  // FIXME SW-642: currently we query only the master minion fix the ioctl
  // once the driver is fixed
  while (ready == 0) {
    bool valid = false;
    auto res = ioctl(GET_MBOX_READY, &ready);
    std::tie(valid, ready) = res;
    if (!valid) {
      RTERROR << "Failed to get the status of the mailbox \n";
      std::terminate();
    }
    RTDEBUG << "MailBox: Ready Value: " << ready << "\n";
    if (ready) {
      return true;
    }
    std::this_thread::sleep_for(polling_interval);
    if (end < Clock::now()) {
      RTERROR << "Mailbox not ready in time \n";
      return false;
    }
  }
  return (bool)ready;
}

bool MailBoxDev::reset() {
  uint64_t unused;
  // FIXME SW-642: currently we query only the master minion fix the ioctl
  // once the driver is fixed
  auto valid = ioctl_set(RESET_MBOX, &unused);
  if (!valid) {
    RTERROR << "Failed to g \n";
    std::terminate();
  }
  RTDEBUG << "MailBox: Reset done \n";
  return true;
}

ssize_t MailBoxDev::queryMboxMaxMsgSize() {
  ssize_t mb_size;
  // FIXME SW-642: currently we query only the master minion fix the ioctl
  // once the driver is adapted
  auto [valid, res] = ioctl(GET_MBOX_MAX_MSG, &mb_size);
  if (!valid) {
    RTERROR << "Failed to get maximum mailbox message size \n";
    std::terminate();
  }
  return res;
}

bool MailBoxDev::write(const void *data, ssize_t size) {
  // FIXME currently we do not take into consideration the maximum mailbox size
  RTDEBUG << "Mailbox Write, size: " << size << "\n";
  auto write_size = ::write(fd_, data, size);
  assert(write_size == size);
  return true;
}

ssize_t MailBoxDev::read(void *data, ssize_t size, TimeDuration wait_time) {
  auto start = Clock::now();
  auto end = start + wait_time;
  // FIXME random polling interval
  static const TimeDuration polling_interval = std::chrono::seconds(1);
  ssize_t mb_read = 0;
  while (mb_read == 0) {
    mb_read = ::read(fd_, data, size);
    std::this_thread::sleep_for(polling_interval);
    if (end < Clock::now()) {
      break;
    }
  }

  RTDEBUG << "Mailbox READ, size: " << mb_read << "\n";
  return mb_read;
}

} // namespace device
} // namespace et_runtime
