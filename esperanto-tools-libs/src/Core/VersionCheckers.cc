//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "Core/VersionCheckers.h"

#include "Common/ProjectAutogen.h"
#include "Core/Device.h"
#include "DeviceAPI/CommandsGen.h"
#include "DeviceAPI/ResponsesGen.h"
#include "Support/Logging.h"

#include <memory>
#include <string>

namespace et_runtime {

GitVersionChecker::GitVersionChecker(Device &dev) : dev_(dev) {
  std::string hash(ET_RUNTIME_GIT_HASH);
  if (hash.find("dirty") != std::string::npos) {
    RTERROR << "Current commit is dirty " << hash
            << " please commit any changes";
    std::terminate();
  }
  memcpy(&runtime_sw_commit_, hash.data(), sizeof(runtime_sw_commit_));
}

uint64_t GitVersionChecker::deviceFWHash() {
  if (device_fw_commit_ != 0) {
    return device_fw_commit_;
  }

  auto def_stream = dev_.defaultStream();

  auto devfw_cmd =
      std::make_shared<device_api::devfw_commands::DeviceFwVersionCmd>(
          0, runtime_sw_commit_);

  dev_.addCommand(def_stream, devfw_cmd);

  auto response_future = devfw_cmd->getFuture();
  auto response_wrapper = response_future.get();
  device_fw_commit_ = response_wrapper.response().device_fw_commit;

  return device_fw_commit_;
}

} // namespace et_runtime
