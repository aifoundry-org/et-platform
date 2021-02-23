//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "TargetRPC.h"

#include "EmuMailBoxDev.h"
#include "esperanto/runtime/Support/Logging.h"
#include "esperanto/runtime/Support/TimeHelpers.h"
#include "esperanto/simulator-api.grpc.pb.h"

#include "esperanto/runtime/Common/layout.h"

#include <inttypes.h>

using namespace std;
using namespace simulator_api;

namespace et_runtime {
namespace device {

namespace {
  const uint64_t kVirtQueueDescAddr = 0x8005100000ULL;
}

RPCTarget::RPCTarget(int index, const std::string &p)
    : DeviceTarget(index), socket_path_(p),
      mailboxDev_(std::make_unique<EmuMailBoxDev>(*this, MailboxTarget::MAILBOX_TARGET_MM)) {}

bool RPCTarget::init() {
  grpc::ChannelArguments ch_args;
  ch_args.SetMaxReceiveMessageSize(-1);
  channel_ = grpc::CreateCustomChannel(socket_path_, grpc::InsecureChannelCredentials(), ch_args);
  stub_ = SimAPI::NewStub(channel_);
  device_alive_ = true;
  return true;
}

bool RPCTarget::postFWLoadInit() {
  // We expect that the device-cw is already loaded and "booted" at this point
  // we are resetting the mailboxes
  auto success = rpcWaitForHostInterrupt();
  assert(success);

  // For DeviceFW reset the mailbox as well and wait for device-fw to be ready
  success = mailboxDev_->ready(std::chrono::seconds(20));
  assert(success);

  success = mailboxDev_->reset();
  assert(success);

  success = mailboxDev_->ready(std::chrono::seconds(20));
  assert(success);

  return true;
}

bool RPCTarget::deinit() { return shutdown(); }

uintptr_t RPCTarget::dramBaseAddr() const { return DRAM_MEMMAP_BEGIN; }

uintptr_t RPCTarget::dramSize() const { return DRAM_MEMMAP_SIZE; }

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

bool RPCTarget::virtQueuesDiscover(TimeDuration wait_time) {
  assert(false);
  return false;
}

bool RPCTarget::virtQueueWrite(const void *data, ssize_t size, uint8_t queueId) {
  assert(false);
  return false;
}

ssize_t RPCTarget::virtQueueRead(void *data, ssize_t size, uint8_t queueId) {
  assert(false);
  return false;
}

bool RPCTarget::waitForEpollEvents(uint32_t &sq_bitmap, uint32_t &cq_bitmap) {
  assert(false);
  return false;
}

std::pair<bool, simulator_api::Reply> RPCTarget::doRPC(const simulator_api::Request& request) {
  simulator_api::Reply reply;
  Status status;
  grpc::ClientContext context;
  // Wait until the server is up do not fail immediately
  context.set_wait_for_ready(true);
  status = stub_->SimCommand(&context, request, &reply);
  return {status.ok(), reply};
}

bool RPCTarget::readDevMemMMIO(uintptr_t dev_addr, size_t size, void *buf) {
  return readDevMemDMA(dev_addr, size, buf);
}

bool RPCTarget::writeDevMemMMIO(uintptr_t dev_addr, size_t size, const void *buf) {
  return writeDevMemDMA(dev_addr, size, buf);
}

bool RPCTarget::readDevMemDMA(uintptr_t dev_addr, size_t size, void *buf) {
  return rpcMemoryRead(dev_addr, size, buf);
}

bool RPCTarget::writeDevMemDMA(uintptr_t dev_addr, size_t size, const void *buf) {
  return rpcMemoryWrite(dev_addr, size, buf);
}

ssize_t RPCTarget::mboxMsgMaxSize() const {
  return mailboxDev_->mboxMaxMsgSize();
}

bool RPCTarget::mb_write(const void *data, ssize_t size) {
  return mailboxDev_->write(data, size);
}

ssize_t RPCTarget::mb_read(void *data, ssize_t size) {
  return mailboxDev_->read(data, size);
}

/* RPC */

bool RPCTarget::rpcBootShire(uint32_t shire_id, uint32_t thread0_enable, uint32_t thread1_enable) {
  simulator_api::Request request;
  auto boot_req = new BootReq();
  boot_req->set_shire_id(shire_id);
  boot_req->set_thread0_enable(thread0_enable);
  boot_req->set_thread1_enable(thread1_enable);
  request.set_allocated_boot(boot_req);
  // Do RPC and wait for reply
  auto reply_res = doRPC(request);
  if (!reply_res.first) {
    return false;
  }
  auto reply = reply_res.second;

  assert(reply.has_boot());
  auto &boot_rsp = reply.boot();
  assert(boot_rsp.success());
  return true;
}

bool RPCTarget::rpcMemoryRead(uint64_t dev_addr, uint64_t size, void *buf) {
  // Create request
  simulator_api::Request request;
  auto mem = new MemoryAccess();
  mem->set_type(MemoryAccessType::MEMORY_READ);
  mem->set_status(MemoryAccessStatus::MEMORY_ACCESS_STATUS_NONE);
  mem->set_addr(dev_addr);
  mem->set_size(size);
  request.set_allocated_memory(mem);
  // Do RPC and wait for reply
  auto reply_res = doRPC(request);
  if (!reply_res.first) {
    return false;
  }
  auto reply = reply_res.second;
  assert(reply.has_memory());

  auto &mem_resp = reply.memory();
  assert(mem_resp.type() == MemoryAccessType::MEMORY_READ);
  assert(mem_resp.addr() == dev_addr);
  assert(mem_resp.size() == size);
  assert(mem_resp.status() == MemoryAccessStatus::MEMORY_ACCESS_STATUS_SUCCESS);
  memcpy(buf, reinterpret_cast<const void *>(mem_resp.data().c_str()), size);
  return true;
}

bool RPCTarget::rpcMemoryWrite(uint64_t dev_addr, uint64_t size, const void *buf) {
  // Create request
  simulator_api::Request request;
  auto mem = new MemoryAccess();
  mem->set_type(MemoryAccessType::MEMORY_WRITE);
  mem->set_status(MemoryAccessStatus::MEMORY_ACCESS_STATUS_NONE);
  mem->set_addr(dev_addr);
  mem->set_size(size);
  mem->set_data(buf, size);
  request.set_allocated_memory(mem);
  // Do RPC and wait for reply
  auto reply_res = doRPC(request);
  if (!reply_res.first) {
    return false;
  }
  auto reply = reply_res.second;
  assert(reply.has_memory());
  auto &mem_resp = reply.memory();
  assert(mem_resp.type() == MemoryAccessType::MEMORY_WRITE);
  assert(mem_resp.addr() == dev_addr);
  assert(mem_resp.size() == size);
  assert(mem_resp.status() == MemoryAccessStatus::MEMORY_ACCESS_STATUS_SUCCESS);
  return true;
}

bool RPCTarget::rpcMailboxRead(MailboxTarget target, uint32_t offset, uint32_t size, void *data) {
  // Create request
  simulator_api::Request request;
  auto mb = new MailboxAccess();
  mb->set_target(target);
  mb->set_type(MailboxAccessType::MAILBOX_READ);
  mb->set_status(MailboxAccessStatus::MAILBOX_ACCESS_STATUS_NONE);
  mb->set_offset(offset);
  mb->set_size(size);
  request.set_allocated_mailbox(mb);
  // Do RPC and wait for reply
  auto reply_res = doRPC(request);
  if (!reply_res.first) {
    return false;
  }
  auto reply = reply_res.second;
  assert(reply.has_mailbox());

  auto &mb_resp = reply.mailbox();
  assert(mb_resp.target() == target);
  assert(mb_resp.type() == MailboxAccessType::MAILBOX_READ);
  assert(mb_resp.status() == MailboxAccessStatus::MAILBOX_ACCESS_STATUS_SUCCESS);
  assert(mb_resp.offset() == offset);
  assert(mb_resp.size() == size);
  memcpy(data, reinterpret_cast<const void *>(mb_resp.data().c_str()), size);
  return true;
}

bool RPCTarget::rpcMailboxWrite(MailboxTarget target, uint32_t offset, uint32_t size,
                                const void *data) {
  // Create request
  simulator_api::Request request;
  auto mb = new MailboxAccess();
  mb->set_target(target);
  mb->set_type(MailboxAccessType::MAILBOX_WRITE);
  mb->set_status(MailboxAccessStatus::MAILBOX_ACCESS_STATUS_NONE);
  mb->set_offset(offset);
  mb->set_size(size);
  mb->set_data(data, size);
  request.set_allocated_mailbox(mb);
  // Do RPC and wait for reply
  auto reply_res = doRPC(request);
  if (!reply_res.first) {
    return false;
  }
  auto reply = reply_res.second;
  assert(reply.has_mailbox());
  auto &mb_resp = reply.mailbox();
  assert(mb_resp.target() == target);
  assert(mb_resp.type() == MailboxAccessType::MAILBOX_WRITE);
  assert(mb_resp.status() == MailboxAccessStatus::MAILBOX_ACCESS_STATUS_SUCCESS);
  assert(mb_resp.offset() == offset);
  assert(mb_resp.size() == size);
  return true;
}

bool RPCTarget::rpcRaiseDevicePuPlicPcieMessageInterrupt() {
  simulator_api::Request request;
  auto device_interrupt = new DeviceInterrupt();
  device_interrupt->set_type(
      simulator_api::DeviceInterruptType::PU_PLIC_PCIE_MESSAGE_INTERRUPT);
  request.set_allocated_device_interrupt(device_interrupt);
  // Do RPC and wait for reply
  auto reply_res = doRPC(request);
  if (!reply_res.first) {
    return false;
  }
  auto reply = reply_res.second;

  assert(reply.has_interrupt());
  auto &interrupt_rsp = reply.interrupt();
  assert(interrupt_rsp.success());
  return true;
}

bool RPCTarget::rpcRaiseDeviceSpioPlicPcieMessageInterrupt() {
  simulator_api::Request request;
  auto device_interrupt = new DeviceInterrupt();
  device_interrupt->set_type(
      simulator_api::DeviceInterruptType::SPIO_PLIC_MBOX_HOST_INTERRUPT);
  request.set_allocated_device_interrupt(device_interrupt);
  // Do RPC and wait for reply
  auto reply_res = doRPC(request);
  if (!reply_res.first) {
    return false;
  }
  auto reply = reply_res.second;

  assert(reply.has_interrupt());
  auto &interrupt_rsp = reply.interrupt();
  assert(interrupt_rsp.success());
  return true;
}

bool RPCTarget::rpcWaitForHostInterrupt() {
  simulator_api::Request request;
  auto host_interrupt = new HostInterrupt();
  // mailbox implementation uses only zeroth bit interrupt
  host_interrupt->set_interrupt_bitmap(0x1);
  request.set_allocated_host_interrupt(host_interrupt);
  // Do RPC and wait for reply
  auto reply_res = doRPC(request);

  RTINFO << "Interrupt reply received, result: " << reply_res.first;
  if (!reply_res.first) {
    return false;
  }
  auto reply = reply_res.second;

  assert(reply.has_interrupt());
  auto &interrupt_rsp = reply.interrupt();
  if (not interrupt_rsp.success()) {
    RTINFO << "No interrupt raised";
    return false;
  }
  return true;
}

bool RPCTarget::shutdown() {
  // Mark the device as not alive ahead of time so that we can
  // do a proper teardown
  device_alive_ = false;

  simulator_api::Request request;
  auto shutdown_req = new ShutdownReq();
  request.set_allocated_shutdown(shutdown_req);
  // Do RPC and wait for reply
  auto reply_res = doRPC(request);
  if (!reply_res.first) {
    return false;
  }
  auto reply = reply_res.second;

  assert(reply.has_shutdown());
  auto &shutdown_rsp = reply.shutdown();
  assert(shutdown_rsp.success());
  RTINFO << "RPC device shutdown";
  return true;
}

} // namespace device
} // namespace et_runtime
