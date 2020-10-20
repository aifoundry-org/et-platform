//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#ifndef ET_RUNTIME_DEVICE_SYSEMU_MM_H
#define ET_RUNTIME_DEVICE_SYSEMU_MM_H

/// @file

#include "RPCDevice/RPCTarget_MM.h"

#include "RPCDevice/SysEmuLauncher.h"

#include <cassert>
#include <future>
#include <memory>
#include <string>

namespace et_runtime {
namespace device {

class SysEmuLauncher;

/// @class TargetSysEmu
class SysEmuTargetMM final : public RPCTargetMM {
public:
  SysEmuTargetMM(int index);
  ~SysEmuTargetMM();

  TargetType type() override { return DeviceTarget::TargetType::SysEmuGRPC_VQ_MM; }
  bool init() override;
  bool postFWLoadInit() override;
  bool deinit() override;
  bool getStatus() override;
  DeviceInformation getStaticConfiguration() override;
  bool submitCommand() override;
  bool registerResponseCallback() override;
  bool registerDeviceEventCallback() override;

private:
  std::future<bool> simulator_status_; ///< Future holding the simulator thread status;
  std::unique_ptr<SysEmuLauncher> sys_emu_; ///< Object responsible for lauching and monitoring
                                            ///  the sysemu simulator

  std::string name_ = "SysEmuGRPC_VQ_MM";
  /// @brief Wait for a connectio from SysEmu
  void waitForConnection();
  /// @brief Fork SysEmu processs and wait until we are requested to stop
  bool launchSimulator();

  bool simulator_running();
  bool alive() override;
};

} // namespace device
} // namespace et_runtime

#endif // ET_RUNTIME_DEVICE_SYSEMU_MM_H
