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
  /// @brief Construct a MailBox device passing the path to it
  MailBoxDev(const std::experimental::filesystem::path &char_dev);
  /// @bried Copying the device has been disabled
  MailBoxDev(const MailBoxDev &rhs) = delete;
  /// @brief Move constructor for the device
  MailBoxDev(MailBoxDev &&other);

  /// @brief Query if the mailbox devie is ready provide timeout to wait for
  bool ready(TimeDuration wait_time = TimeDuration::max());

  /// @brief Reret the mailbox device and discard any pending mailbox messages
  /// from the device
  bool reset();

  /// @brief Return the maximum size of a mailbox message
  ssize_t mboxMaxMsgSize() const { return mbox_max_msg_size_; }

  /// @brief Write message pointed by pointer data of size "size". Return true
  /// of success
  bool write(const void *data, ssize_t size);

  /// @brief Read message of size "size" in buffer data. Wait until wait_time
  /// expires otherwise return false.
  ssize_t read(void *data, ssize_t size,
               TimeDuration wait_time = TimeDuration::max());

protected:
  ssize_t mbox_max_msg_size_ = -1; ///< Maximum size of mailbox message;

  /// @brief Return the maximum mbox size using the respective IOCTL
  ssize_t queryMboxMaxMsgSize();
};
} // namespace device

} // namespace et_runtime

#endif // ET_RUNTIME_DEVICE_MAILBOXDEV_H
