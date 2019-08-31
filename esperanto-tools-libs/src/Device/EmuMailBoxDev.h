//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#ifndef ET_RUNTIME_DEVICE_EMUMAILBOXDEV_H
#define ET_RUNTIME_DEVICE_EMUMAILBOXDEV_H

#include "Support/TimeHelpers.h"

#include <unistd.h>

namespace et_runtime {
namespace device {

class RPCTarget;

///
/// @brief Helper class that impements the mailbox protocol.
///
/// This class provides the same interface as the MailBoxDev that wraps
/// the character device, but internally it fully implements the mailbox
/// protocol over the SimualorAPI. This is used when we communicate to
/// Device-FW over the SimulatorAPI to SysEmu or VCS
class EmuMailBoxDev {
public:
  /// @brief Construct a MailBox device passing the path to it
  EmuMailBoxDev(RPCTarget &rpcDevice);
  EmuMailBoxDev(EmuMailBoxDev &) = delete;

  bool write(const void *data, ssize_t size);
  ssize_t read(void *data, ssize_t size,
               TimeDuration wait_time = TimeDuration::max());

private:
  RPCTarget &rpcDev_;
};
} // namespace device

} // namespace et_runtime

#endif // ET_RUNTIME_DEVICE_EMUMAILBOXDEV_H
