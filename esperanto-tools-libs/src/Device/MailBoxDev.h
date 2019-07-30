//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#ifndef ET_RUNTIME_DEVICE_MAILBOXDEV_H
#define ET_RUNTIME_DEVICE_MAILBOXDEV_H

#include "CharDevice.h"

#include "Support/TimeHelpers.h"

#include <chrono>
#include <experimental/filesystem>

namespace et_runtime {
namespace device {

///
/// @brief Helper class that encapsulates the interactions with a character
/// device that implements the MailBox protocol to the device
class MailBoxDev final : public CharacterDevice {
public:
  // The following values have been extracted
  static constexpr int RINGBUFFER_LENGTH = (254 * 8);
  static constexpr int RINGBUFFER_MAX_LENGTH = (RINGBUFFER_LENGTH - 1U);
  static constexpr int MBOX_MAX_MESSAGE_LENGTH = RINGBUFFER_MAX_LENGTH;

  /// @brief Construct a MailBox device passing the path to it
  MailBoxDev(const std::experimental::filesystem::path &char_dev);
  /// @bried Copying the device has been disabled
  MailBoxDev(const MailBoxDev &rhs) = delete;
  /// @brief Move constructor for the device
  MailBoxDev(MailBoxDev &&other);

  bool write(const void *data, ssize_t size);
  ssize_t read(void *data, ssize_t size,
               TimeDuration wait_time = TimeDuration::max());
};
} // namespace device

} // namespace et_runtime

#endif // ET_RUNTIME_DEVICE_MAILBOXDEV_H
