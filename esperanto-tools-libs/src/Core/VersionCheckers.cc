//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "esperanto/runtime/Core/VersionCheckers.h"

#include "DeviceAPI/CommandsGen.h"
#include "DeviceAPI/ResponsesGen.h"
#include "Tracing/Tracing.h"
#include "esperanto/runtime/Common/ProjectAutogen.h"
#include "esperanto/runtime/Core/Device.h"
#include "esperanto/runtime/Support/Logging.h"

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
  auto &response = response_future.get().response();
  assert(response.response_info.message_id ==
         ::device_api::MBOX_DEVAPI_MESSAGE_ID_DEVICE_FW_VERSION_RSP);
  device_fw_commit_ = response.device_fw_commit;

  return device_fw_commit_;
}

DeviceAPIChecker::DeviceAPIChecker(Device &dev) : dev_(dev) {}

bool DeviceAPIChecker::getDeviceAPIVersion() {
  if (deviceQueried_) {
    return true;
  }
  auto def_stream = dev_.defaultStream();
  auto devapi_cmd =
      std::make_shared<device_api::devfw_commands::DeviceApiVersionCmd>(
          0, ESPERANTO_DEVICE_API_VERSION_MAJOR,
          ESPERANTO_DEVICE_API_VERSION_MINOR,
          ESPERANTO_DEVICE_API_VERSION_PATCH, DEVICE_API_HASH);

  dev_.addCommand(def_stream, devapi_cmd);

  auto response_future = devapi_cmd->getFuture();
  auto &response = response_future.get().response();
  assert(response.response_info.message_id ==
         ::device_api::MBOX_DEVAPI_MESSAGE_ID_DEVICE_API_VERSION_RSP);
  mmFWDevAPIMajor_ = response.major;
  mmFWDevAPIMinor_ = response.minor;
  mmFWDevAPIPatch_ = response.patch;
  mmFWDevAPIHash_ = response.api_hash;
  mmFWAccept_ = response.accept;
  deviceQueried_ = true;
  return true;
}

bool DeviceAPIChecker::isDeviceSupported() {
  auto success = getDeviceAPIVersion();
  if (!success) {
    RTERROR << "Failed to query the device for the version of the DeviceAPI";
    return false;
  }

  bool supported = true;
  if ((mmFWDevAPIMajor_ != ESPERANTO_DEVICE_API_VERSION_MAJOR) ||
      (mmFWAccept_ == false) ||
      (mmFWDevAPIMinor_ < ESPERANTO_DEVICE_API_VERSION_MAJOR)) {
    supported = false;
  }

  TRACE_Device_device_api_supported(supported, DEVICE_API_HASH,
                                    mmFWDevAPIHash_);
  return supported;
}

} // namespace et_runtime
