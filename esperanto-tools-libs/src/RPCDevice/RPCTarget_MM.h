//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#ifndef ET_RUNTIME_DEVICE_RPC_MM_H
#define ET_RUNTIME_DEVICE_RPC_MM_H

/// @file

#include "esperanto/runtime/Core/DeviceTarget.h"

#include "Core/DeviceFwTypes.h"
#include "EmuVirtQueueDev.h"
#include <esperanto/simulator-api.grpc.pb.h>
#include <grpc/support/log.h>
#include <grpcpp/grpcpp.h>

#include <atomic>
#include <cassert>
#include <memory>
#include <string>
#include <thread>

using grpc::Channel;
using grpc::ClientAsyncResponseReader;
using grpc::ClientContext;
using grpc::CompletionQueue;
using grpc::Status;
using simulator_api::Reply;
using simulator_api::Request;
using simulator_api::SimAPI;

namespace et_runtime {
namespace device {

///
/// @brief This class provides is responsible for creating rpc Calls
///
/// All the RPC calls will be communicated over the SimulatorAPI to
/// SysEmu through this class.
class RPCGenerator {
public:
  /// @brief Construct RPC Generator
  RPCGenerator();
  RPCGenerator(RPCGenerator &) = delete;

  /// @brief RPC initializations
  bool rpcInit(const std::string &socketPath);

  /// @brief Do RPC request from simulator-api
  std::pair<bool, simulator_api::Reply> doRPC(const simulator_api::Request &req);

  /// @brief Boot specific Minions of a Shire
  bool rpcBootShire(uint32_t shire_id, uint32_t thread0_enable, uint32_t thread1_enable);

  /// @brief Memory read access to the target virtual queue
  bool rpcMemoryRead(uint64_t devAddr, uint64_t size, void *data);

  /// @brief Memory write access to the target virtual queue
  bool rpcMemoryWrite(uint64_t devAddr, uint64_t size, const void *data);

  /// @brief Memory read access to the target virtual queue (SRAM/DRAM)
  bool rpcVirtQueueRead(uint64_t devAddr, uint64_t size, void *data);

  /// @brief Memory write access to the target virtual queue (SRAM/DRAM)
  bool rpcVirtQueueWrite(uint64_t devAddr, uint64_t size, const void *data);

  /// @brief Raise PU PLIC PCIe Message Interrupt to the simulator
  bool rpcRaiseDevicePuPlicPcieMessageInterrupt();

  /// @brief Raise SPIO PLIC PCIe Message Interrupt to the simulator
  bool rpcRaiseDeviceSpioPlicPcieMessageInterrupt();

  /// @brief Wait to receive an interrupt from the device or timeout.
  /// @param[in] queueId : Blocks until interrupt occurs on queueId
  /// @param[in] wait_time : Time to wait to receive the interrupt, but default block indefinitely.
  bool rpcWaitForHostInterrupt(uint8_t queueId, TimeDuration wait_time = TimeDuration::max());

  /// @brief Wait to receive an interrupt on any queue from the device.
  /// @param[in] wait_time : Time to wait to receive the interrupt, but default block indefinitely.
  uint32_t rpcWaitForHostInterruptAny(TimeDuration wait_time = TimeDuration::max());

  /// @brief RPC request to shutdown
  bool rpcShutdown();

private:
  std::unique_ptr<simulator_api::SimAPI::Stub> stub_;
};

/// @class RPCTargetMM
class RPCTargetMM : public DeviceTarget {
public:
  RPCTargetMM(int index, const std::string &str);
  virtual ~RPCTargetMM() = default;

  TargetType type() override { return DeviceTarget::TargetType::DeviceGRPC; }
  bool init() override;
  bool boot(uint32_t shire_id, uint32_t thread0_enable, uint32_t thread1_enable);
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

  /// @brief Return the absolute base DRAM address we can access
  uintptr_t dramBaseAddr() const override;

  /// @brief Return the size of DRAM we can write to in bytes.
  uintptr_t dramSize() const override;

  /// TODO SW-4821: remove this with the mailbox implementation
  ssize_t mboxMsgMaxSize() const final;

  /// TODO SW-4821: remove this with the mailbox implementation
  bool mb_write(const void *data, ssize_t size) final;

  /// TODO SW-4821: remove this with the mailbox implementation
  ssize_t mb_read(void *data, ssize_t size, TimeDuration wait_time = TimeDuration::max()) final;

  /// @brief Discover virtual queues info
  bool virtQueuesDiscover(TimeDuration wait_time = TimeDuration::max()) final;

  /// @brief Write a full virtual queue message, this corresponds to the API that the PCIE device exposes
  bool virtQueueWrite(const void *data, ssize_t size, uint8_t queueId) final;

  /// @brief READ a full virtual queue message, this corresponds to the API that the PCIE device exposes
  ssize_t virtQueueRead(void *data, ssize_t size, uint8_t queueId, TimeDuration wait_time = TimeDuration::max()) final;

  /// @brief wait for EPOLLIN or EPOLLOUT event on any of the virtual queue
  bool waitForEpollEvents(uint32_t &sq_bitmap, uint32_t &cq_bitmap) final;

  /// @brief VirtQueue buffer size
  uint16_t virtQueueBufSize() { return queueBufSize_; };

  /// @brief Shutdown target
  bool shutdown() override;

protected:
  std::string socket_path_;

private:
  std::shared_ptr<RPCGenerator> rpcGen_;
  std::vector<std::pair<uint8_t, std::unique_ptr<EmuVirtQueueDev>>> queueStorage_;
  uint16_t queueBufSize_;
  std::atomic<bool> host_mem_access_finish_;
  std::thread host_mem_access_thread_;

  void hostMemoryAccessThread();

  /// @brief Create virtual queue emu device paired with queueId
  std::pair<uint8_t, EmuVirtQueueDev &> createVirtQueue(uint8_t queueId, uint64_t queueBaseAddr, uint64_t queueHeaderAddr);

  /// @brief Get queue pointer using queueId
  EmuVirtQueueDev* getVirtQueue(uint8_t queueId);

  /// @brief Destroy queue paired with queueId
  bool destroyVirtQueue(uint8_t queueId);
};

} // namespace device
} // namespace et_runtime

#endif // ET_RUNTIME_DEVICE_RPC_MM_H
