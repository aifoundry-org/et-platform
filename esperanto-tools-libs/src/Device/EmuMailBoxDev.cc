//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "EmuMailBoxDev.h"

#include "Support/Logging.h"
#include "TargetRPC.h"

#include <cassert>
#include <unistd.h>

namespace et_runtime {
namespace device {

EmuMailBoxDev::EmuMailBoxDev(RPCTarget &dev) : rpcDev_(dev) {}

bool EmuMailBoxDev::write(const void *data, ssize_t size) {
  // FIXME currently we do not take into consideration the maximum mailbox size
  RTDEBUG << "Mailbox Write, size: " << size << "\n";
  assert(false);
  return true;
}

ssize_t EmuMailBoxDev::read(void *data, ssize_t size, TimeDuration wait_time) {
  RTDEBUG << "Mailbox READ, size: \n";
  assert(false);
  return 0;
}

} // namespace device
} // namespace et_runtime
