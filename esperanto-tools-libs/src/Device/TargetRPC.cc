//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "Device/TargetRPC.h"
#include <inttypes.h>
#include "esperanto/simulator-api.grpc.pb.h"

using namespace std;
using namespace simulator_api;

namespace et_runtime {
namespace device {

RPCTarget::RPCTarget(int index, const std::string &p)
    : DeviceTarget(index), path_(p) {}

bool RPCTarget::init() {
  grpc::ChannelArguments ch_args;
  ch_args.SetMaxReceiveMessageSize(-1);
  channel_ = grpc::CreateCustomChannel(
      path_, grpc::InsecureChannelCredentials(), ch_args);
  stub_ = SimAPI::NewStub(channel_);
  return true;
}

bool RPCTarget::deinit() { return shutdown(); }

bool RPCTarget::getStatus() {
  assert(true);
  return false;
}

DeviceInformation RPCTarget::getStaticConfiguration() {
  assert(true);
  return {};
}

bool RPCTarget::submitCommand() {
  assert(true);
  return false;
}

bool RPCTarget::registerResponseCallback() {
  assert(true);
  return false;
}

bool RPCTarget::registerDeviceEventCallback() {
  assert(true);
  return false;
}

simulator_api::Reply RPCTarget::doRPC(const simulator_api::Request &request) {
  simulator_api::Reply reply;
  Status status;
  grpc::ClientContext context;
  // Wait until the server is up do not fail immediately
  context.set_wait_for_ready(true);
  status = stub_->SimCommand(&context, request, &reply);
  assert(status.ok());
  return reply;
}

bool RPCTarget::readDevMem(uintptr_t dev_addr, size_t size, void *buf) {
  // Create request
  simulator_api::Request request;
  auto dma = new DMAAccess();
  dma->set_type(DMAAccessType::DMA_READ);
  dma->set_status(DMAAccessStatus::DMAStatus_NONE);
  dma->set_addr(dev_addr);
  dma->set_size(size);
  request.set_allocated_dma(dma);
  // Do RPC and wait for reply
  auto reply = doRPC(request);
  assert(reply.has_dma());
  auto &dma_resp = reply.dma();
  assert(dma_resp.type() == DMAAccessType::DMA_READ);
  assert(dma_resp.addr() == dev_addr);
  assert(dma_resp.size() == size);
  assert(dma_resp.status() == DMAAccessStatus::DMAStatus_SUCCESS);
  memcpy(buf, reinterpret_cast<const void *>(dma_resp.data().c_str()), size);
  return true;
}

bool RPCTarget::writeDevMem(uintptr_t dev_addr, size_t size, const void *buf) {
  // Create request
  simulator_api::Request request;
  auto dma = new DMAAccess();
  dma->set_type(DMAAccessType::DMA_WRITE);
  dma->set_status(DMAAccessStatus::DMAStatus_NONE);
  dma->set_addr(dev_addr);
  dma->set_size(size);
  dma->set_data(buf, size);
  request.set_allocated_dma(dma);
  // Do RPC and wait for reply
  auto reply = doRPC(request);
  assert(reply.has_dma());
  auto &dma_resp = reply.dma();
  assert(dma_resp.type() == DMAAccessType::DMA_WRITE);
  assert(dma_resp.addr() == dev_addr);
  assert(dma_resp.size() == size);
  assert(dma_resp.status() == DMAAccessStatus::DMAStatus_SUCCESS);
  return true;
}

bool RPCTarget::launch(uintptr_t launch_pc, const layer_dynamic_info *params) {

      fprintf(stderr,
            "RPCTarget::Going to execute kernel {0x%lx}\n"
            "  tensor_a = 0x%" PRIx64 "\n"
            "  tensor_b = 0x%" PRIx64 "\n"
            "  tensor_c = 0x%" PRIx64 "\n"
            "  tensor_d = 0x%" PRIx64 "\n"
            "  tensor_e = 0x%" PRIx64 "\n"
            "  tensor_f = 0x%" PRIx64 "\n"
            "  tensor_g = 0x%" PRIx64 "\n"
            "  tensor_h = 0x%" PRIx64 "\n"
            "  pc/id    = 0x%" PRIx64 "\n",
            launch_pc,
            params->tensor_a, params->tensor_b, params->tensor_c,
            params->tensor_d, params->tensor_e, params->tensor_f,
            params->tensor_g, params->tensor_h, params->kernel_id);

  // Send an Execute command
  simulator_api::Request request;
  auto card_emu = new CardEmuReq();
  auto execute = new CardEmuExecuteReq();
  execute->set_launch_pc(launch_pc);
  execute->set_params(params, sizeof(*params));
  //execute->set_thread1_pc(launch_pc);
  card_emu->set_allocated_execute(execute);
  request.set_allocated_card_emu(card_emu);
  // Check execute response
  {
    auto reply = doRPC(request);
    assert(reply.has_card_emu());
    auto &card_emu_resp = reply.card_emu();
    assert(card_emu_resp.has_execute());
    auto &execute_resp = card_emu_resp.execute();
    assert(execute_resp.success());
  }

  // Send a sync command
  auto sync = new CardEmuSyncReq();
  sync->set_do_sync(true);
  card_emu->set_allocated_sync(sync);
  // Check execute response
  {
    auto reply = doRPC(request);
    assert(reply.has_card_emu());
    auto &card_emu_resp = reply.card_emu();
    assert(card_emu_resp.has_sync());
    auto &sync_resp = card_emu_resp.sync();
    assert(sync_resp.success());
  }
  return true;
}

bool RPCTarget::boot(uintptr_t init_pc, uintptr_t trap_pc) {
  simulator_api::Request request;
  auto card_emu = new CardEmuReq();
  // Send boot request
  auto boot = new CardEmuBootReq();
  boot->set_init_pc(init_pc);
  boot->set_trap_pc(trap_pc);
  card_emu->set_allocated_boot(boot);
  request.set_allocated_card_emu(card_emu);
  // Do RPC
  auto reply = doRPC(request);
  assert(reply.has_card_emu());
  auto &card_emu_resp = reply.card_emu();
  assert(card_emu_resp.has_boot());
  auto &boot_resp = card_emu_resp.boot();
  assert(boot_resp.success());
  return true;
}

bool RPCTarget::shutdown() {
  // Send a Shutdown comand to the target
  simulator_api::Request request;
  auto card_emu = new CardEmuReq();
  auto shutdown = new CardEmuShutdownReq();
  shutdown->set_shutdown(true);
  card_emu->set_allocated_shutdown(shutdown);
  request.set_allocated_card_emu(card_emu);
  // Do RPC
  auto reply = doRPC(request);
  assert(reply.has_card_emu());
  auto &card_emu_resp = reply.card_emu();
  assert(card_emu_resp.has_shutdown());
  auto &shutdown_resp = card_emu_resp.shutdown();
  assert(shutdown_resp.success());
  return true;
}

} // namespace device
} // namespace et_runtime
