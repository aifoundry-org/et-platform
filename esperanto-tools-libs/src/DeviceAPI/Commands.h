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

#include "esperanto/runtime/DeviceAPI/Command.h"
#include "esperanto/runtime/Support/HelperMacros.h"
#include "esperanto/runtime/etrt-bin.h"

#include <atomic>
#include <cassert>
#include <condition_variable>
#include <vector>

namespace et_runtime {

class Device;

namespace device_api {

/// @namespace pcie_responses
///
/// This namespace contains all classes that are Responses to a PCIe Command
/// and perform data-tranfers
namespace pcie_responses {

/// @brief
class ReadResponse final : public ResponseBase {
public:
  // dummy type
  using response_devapi_t = bool;

  ReadResponse() = default;

  static constexpr MBOXMessageTypeID responseTypeID() { return 0; }

private:
};

/// @brief
class WriteResponse final : public ResponseBase {
public:
  // dummy type
  using response_devapi_t = bool;

  WriteResponse() = default;

  static constexpr MBOXMessageTypeID responseTypeID() { return 0; }
};

} // namespace pcie_responses

/// @namespace pcie_commands
///
/// This namespace contains all Commands we can issue to PCIe to do data-transfers
namespace pcie_commands {
/// @brief
class ReadCommand final : public Command<pcie_responses::ReadResponse> {

public:
  ReadCommand(void *dstHostPtr, const void *srcDevPtr, size_t count)
      : dstHostPtr(dstHostPtr), srcDevPtr(srcDevPtr), count(count) {}
  etrtError execute(et_runtime::Device *device_target) override;

  // FIXME invalid command id
  MBOXMessageTypeID commandTypeID() const override { return 0; }

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

  // FIXME invalid command id
  MBOXMessageTypeID commandTypeID() const override { return 0; }

private:
  void *dstDevPtr;
  const void *srcHostPtr;
  size_t count;
};

} // namespace pcie_commands


} // namespace device_api

} // namespace et_runtime

#endif // ET_RUNTIME_MEMORY_COMMANDS_H
