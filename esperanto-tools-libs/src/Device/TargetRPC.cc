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

#include "esperanto/simulator-api.grpc.pb.h"

using namespace std;
using namespace simulator_api;

namespace et_runtime {
namespace device {

RPCTarget::RPCTarget(const std::string &path) : DeviceTarget(path) {
}

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

bool RPCTarget::getStaticConfiguration() {
  assert(true);
  return false;
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

bool RPCTarget::defineDevMem(uintptr_t dev_addr, size_t size, bool is_exec) {
  simulator_api::Request request;
  auto card_emu = new CardEmuReq();
  auto define_mem = new CardEmuDefineMemReq();
  define_mem->set_devptr(dev_addr);
  define_mem->set_size(size);
  define_mem->set_is_exec(is_exec);
  card_emu->set_allocated_define_mem(define_mem);
  request.set_allocated_card_emu(card_emu);
  // Do RPC
  auto reply = doRPC(request);
  assert(reply.has_card_emu());
  auto &card_emu_resp = reply.card_emu();
  assert(card_emu_resp.has_define_mem());
  auto &define_mem_resp = card_emu_resp.define_mem();
  assert(define_mem_resp.success());
  return true;
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

bool RPCTarget::launch(uintptr_t launch_pc) {
  // Send an Execute command
  simulator_api::Request request;
  auto card_emu = new CardEmuReq();
  auto execute = new CardEmuExecuteReq();
  execute->set_thread0_pc(launch_pc);
  execute->set_thread1_pc(launch_pc);
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
