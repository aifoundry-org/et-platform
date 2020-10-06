//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#ifndef ET_RUNTIME_DEVICE_RPC_H
#define ET_RUNTIME_DEVICE_RPC_H

/// @file

#include "esperanto/runtime/Core/DeviceTarget.h"

#include "Core/DeviceFwTypes.h"
#include "EmuMailBoxDev.h"
#include <esperanto/simulator-api.grpc.pb.h>
#include <grpc/support/log.h>
#include <grpcpp/grpcpp.h>

#include <cassert>
#include <memory>
#include <string>

using grpc::Channel;
using grpc::ClientAsyncResponseReader;
using grpc::ClientContext;
using grpc::CompletionQueue;
using grpc::Status;
using simulator_api::Reply;
using simulator_api::Request;
using simulator_api::SimAPI;
using simulator_api::MailBoxTarget;

namespace et_runtime {
namespace device {

/// @class RPCTarget
class RPCTarget : public DeviceTarget {
public:
  RPCTarget(int index, const std::string &str);
  virtual ~RPCTarget() = default;

  TargetType type() override { return DeviceTarget::TargetType::DeviceGRPC; }
  bool init() override;
  bool postFWLoadInit() override;
  bool deinit() override;

  virtual bool getStatus() override;
  virtual DeviceInformation getStaticConfiguration() override;
  virtual bool submitCommand() override;
  virtual bool registerResponseCallback() override;
  virtual bool registerDeviceEventCallback() override;
  bool readDevMemMMIO(uintptr_t dev_addr, size_t size, void *buf) final;
  bool writeDevMemMMIO(uintptr_t dev_addr, size_t size, const void *buf) final;
  bool readDevMemDMA(uintptr_t dev_addr, size_t size, void *buf) final;
  bool writeDevMemDMA(uintptr_t dev_addr, size_t size, const void *buf) final;

  /// @brief Shutdown target
  bool shutdown() override;

  /// @brief Return the absolute base DRAM address we can access
  uintptr_t dramBaseAddr() const override;

  /// @brief Return the size of DRAM we can write to in bytes.
  uintptr_t dramSize() const override;

  // Interace for interacting with the MailBox state using the simulator API

  /// @brief Get mailbox emu device
  EmuMailBoxDev &mailboxDev() { return *mailboxDev_; }

  /// @brief Read access to the target mailbox
  bool mailboxReadAccess(MailBoxTarget target, uint32_t offset, size_t size, void *data);

  /// @brief Write access to the target mailbox
  bool mailboxWriteAccess(MailBoxTarget target, uint32_t offset, size_t size, const void *data);

  /// @brief Boot specific Minions of a Shire
  bool boot_shire(uint32_t shire_id, uint32_t thread0_enable, uint32_t thread1_enable);

  /// @brief Raise the PU PLIC PCIe Message Interrupt in the target "device"
  /// in which case this is the simulator
  bool raiseDevicePuPlicPcieMessageInterrupt();

  /// @brief Raise the SPIO PLIC PCIe Message Interrupt in the target "device"
  /// in which case this is the simulator
  bool raiseDeviceSpioPlicPcieMessageInterrupt();

  /// @brief Wait to receive an interrupt from the device or timeout.
  ///
  /// @param[in] wait_time : Time to wait to receive the interrupt, but default
  ///  block indefinitely.
  bool waitForHostInterrupt(TimeDuration wait_time = TimeDuration::max());

  /// @brief get the maximum mailbox message
  ssize_t mboxMsgMaxSize() const final;

  /// @brief Write a full mailbox message, this corresponds to the API that the PCIE device exposes
  bool mb_write(const void *data, ssize_t size) final;

  /// @brief READ a full mailbox message, this corresponds to the API that the PCIE device exposes
  ssize_t mb_read(void *data, ssize_t size, TimeDuration wait_time = TimeDuration::max()) final;

  // FIXME populate this class with the interface for doing MMIO and raising
  // interrupts to the target device. This interface is to be used by the
  // EmuMailBoxDev to implement the underlying MailBoxProtocol

protected:
  std::string socket_path_;
  std::shared_ptr<grpc::Channel> channel_;
  std::unique_ptr<simulator_api::SimAPI::Stub> stub_;
  grpc::CompletionQueue cq_;

  std::unique_ptr<EmuMailBoxDev> mailboxDev_;
  std::pair<bool, simulator_api::Reply> doRPC(const simulator_api::Request &req);
};

} // namespace device
} // namespace et_runtime

#endif // ET_RUNTIME_DEVICE_RPC_H
