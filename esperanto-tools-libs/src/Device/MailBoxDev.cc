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

namespace et_runtime {
namespace device {

MailBoxDev::MailBoxDev(const std::experimental::filesystem::path &char_dev)
    : CharacterDevice(char_dev) {}

MailBoxDev::MailBoxDev(MailBoxDev &&other)
    : CharacterDevice(std::move(other)) {}
} // namespace device
} // namespace et_runtime
