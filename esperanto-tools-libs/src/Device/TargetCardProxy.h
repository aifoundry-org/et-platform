//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#ifndef ET_RUNTIME_DEVICE_CARD_PROXY_H
#define ET_RUNTIME_DEVICE_CARD_PROXY_H

#include "Core/DeviceTarget.h"
#include "etrpc/card-emu.h"
#include "etrpc/et-card-proxy.h"

#include <cassert>
#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace et_runtime {
namespace device {

class SysEmuLauncher;

///
/// @brief Connect to the SysEMU using the CardProxy
///
/// This class is a uses the CardProxy struct and socket
/// mechanism to connext to SysEMU. This class will be deprecated
/// once we move to the new gPRC mechanism
class CardProxyTarget final : public DeviceTarget {
public:
  /// @brief Construct the Card Proxy target
  ///
  /// @param[in] path  Path card proxy socket
  CardProxyTarget(const std::string &path);
  virtual ~CardProxyTarget();

  bool init() override;
  bool deinit() override;
  bool getStatus() override;
  bool getStaticConfiguration() override;
  bool submitCommand() override;
  bool registerResponseCallback() override;
  bool registerDeviceEventCallback() override;
  bool alive() override;
  bool defineDevMem(uintptr_t dev_addr, size_t size, bool is_exec) override;
  bool readDevMem(uintptr_t dev_addr, size_t size, void *buf) override;
  bool writeDevMem(uintptr_t dev_addr, size_t size, const void *buf) override;
  bool launch(uintptr_t launch_pc) override;
  bool boot(uintptr_t init_pc, uintptr_t trap_pc) override;

private:
  const std::string name_ = "CardProxy";
  std::unique_ptr<CardProxy> card_proxy_; ///< CardProxy used by the current API
  std::future<bool>
      simulator_status_; ///< Future holding the simulator thread status;
  std::mutex simulator_end_mutex_; ///< Mutex to block terminating the simulator
  std::unique_lock<std::mutex>
      simulator_end_lock_; ///< Lock to prevent simulator from starting
  std::string connection_; ///< Path fo the socket used to talk to sysemu
  std::unique_ptr<SysEmuLauncher>
      sys_emu_; ///< Object responsible for lauching and monitoring
                /// the sysemu simulator

  /// @brief Wait for a connectio from SysEmu
  void waitForConnection();
  /// @brief Fork SysEmu processs and wait until we are requested to stop
  bool launchSimulator();

  bool simulator_running();
};

} // namespace device
} // namespace et_runtime

#endif // ET_RUNTIME_DEVICE_CARD_PROXY_H
