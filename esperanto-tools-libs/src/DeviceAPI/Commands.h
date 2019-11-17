//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#ifndef ET_RUNTIME_MEMORY_COMMANDS_H
#define ET_RUNTIME_MEMORY_COMMANDS_H

#include "DeviceAPI/Command.h"

#include "Support/HelperMacros.h"

#include "etrt-bin.h"

#include <atomic>
#include <cassert>
#include <condition_variable>
#include <vector>

namespace et_runtime {

class Device;

namespace device_api {

namespace pcie_responses {

/// @brief
class ReadResponse final : public ResponseBase {
public:
  ReadResponse() = default;

private:
};

/// @brief
class WriteResponse final : public ResponseBase {
public:
  WriteResponse() = default;
};

} // namespace pcie_responses

namespace pcie_commands {
/// @brief
class ReadCommand final : public Command<pcie_responses::ReadResponse> {

public:
  ReadCommand(void *dstHostPtr, const void *srcDevPtr, size_t count)
      : dstHostPtr(dstHostPtr), srcDevPtr(srcDevPtr), count(count) {}
  etrtError execute(et_runtime::Device *device_target) override;

private:
  void *dstHostPtr;
  const void *srcDevPtr;
  size_t count;
};

class WriteCommand final : public Command<pcie_responses::WriteResponse> {

public:
  WriteCommand(void *dstDevPtr, const void *srcHostPtr, size_t count)
      : dstDevPtr(dstDevPtr), srcHostPtr(srcHostPtr), count(count) {}
  etrtError execute(et_runtime::Device *device_target) override;

private:
  void *dstDevPtr;
  const void *srcHostPtr;
  size_t count;
};

} // namespace pcie_commands

// TODO the launch command is to be removed and replaced in the future
// with DeviceAPI commands

/// @brief
class LaunchResponse final : public ResponseBase {
public:
  LaunchResponse() = default;
};

/// @brief
class LaunchCommand final : public Command<LaunchResponse> {

public:
  LaunchCommand(uintptr_t kernel_pc, const std::vector<uint8_t> &args_buff,
                const std::string &kernel_name)
      : kernel_pc(kernel_pc), args_buff(args_buff), kernel_name(kernel_name) {}
  etrtError execute(et_runtime::Device *device_target) override;

private:
  uintptr_t kernel_pc;
  std::vector<uint8_t> args_buff;
  std::string kernel_name;
};

} // namespace device_api

} // namespace et_runtime

#endif // ET_RUNTIME_MEMORY_COMMANDS_H
