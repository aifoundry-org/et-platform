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

#include "Core/DeviceTarget.h"

#include <esperanto/simulator-api.grpc.pb.h>
#include <grpc/support/log.h>
#include <grpcpp/grpcpp.h>

#include <cassert>
#include <memory>
#include <string>

#include "../etrpc/et-rpc.h"

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

class RPCTarget : public DeviceTarget {
public:
  RPCTarget(int index, const std::string &str);
  virtual ~RPCTarget() = default;

  bool init() override;
  bool deinit() override;

  virtual bool getStatus() override;
  virtual DeviceInformation getStaticConfiguration() override;
  virtual bool submitCommand() override;
  virtual bool registerResponseCallback() override;
  virtual bool registerDeviceEventCallback() override;
  bool defineDevMem(uintptr_t dev_addr, size_t size, bool is_exec) final;
  bool readDevMem(uintptr_t dev_addr, size_t size, void *buf) final;
  bool writeDevMem(uintptr_t dev_addr, size_t size, const void *buf) final;
  bool launch(uintptr_t launch_pc, const layer_dynamic_info *params) final;
  virtual bool boot(uintptr_t init_pc, uintptr_t trap_pc);
  bool shutdown();

protected:
  std::string path_;
  std::shared_ptr<grpc::Channel> channel_;
  std::unique_ptr<simulator_api::SimAPI::Stub> stub_;
  grpc::CompletionQueue cq_;

  simulator_api::Reply doRPC(const simulator_api::Request &req);
};

} // namespace device
} // namespace et_runtime

#endif // ET_RUNTIME_DEVICE_RPC_H
