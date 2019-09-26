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

#include "EmuMailBoxDev.h"
#include "esperanto/simulator-api.grpc.pb.h"

#include <inttypes.h>

using namespace std;
using namespace simulator_api;

namespace et_runtime {
namespace device {

RPCTarget::RPCTarget(int index, const std::string &p)
    : DeviceTarget(index), path_(p),
      mailboxDev_(std::make_unique<EmuMailBoxDev>(*this)) {}

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

bool RPCTarget::readDevMemMMIO(uintptr_t dev_addr, size_t size, void *buf) {
  // FIXME use "DMA" to do bulk mmio transfers
  return readDevMemDMA(dev_addr, size, buf);
}
bool RPCTarget::writeDevMemMMIO(uintptr_t dev_addr, size_t size,
                                const void *buf) {
  return writeDevMemDMA(dev_addr, size, buf);
}

bool RPCTarget::readDevMemDMA(uintptr_t dev_addr, size_t size, void *buf) {
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

bool RPCTarget::writeDevMemDMA(uintptr_t dev_addr, size_t size,
                               const void *buf) {
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

#if ENABLE_DEVICE_FW
/// @brief Read from target_sim the status header of the mailbox
std::tuple<bool, std::tuple<uint32_t, uint32_t>> RPCTarget::readMBStatus() {
  // Preparate request to read the mailbox status
  simulator_api::Request request;

  auto mb = new MailBoxMessage();
  mb->set_type(MailBoxAccessType::MB_READ);
  mb->set_req_status(MailBoxAccessStatus::MB_STATUS_NONE);

  auto mb_status = new MailBoxStatus();
  mb->set_allocated_status(mb_status);
  request.set_allocated_mailbox(mb);

  // Do RPC and wait for reply
  auto reply = doRPC(request);

  assert(reply.has_mailbox());

  auto &mb_resp = reply.mailbox();
  assert(mb_resp.type() == MailBoxAccessType::MB_READ);
  assert(mb_resp.req_status() == MailBoxAccessStatus::MB_STATUS_SUCCESS);
  assert(mb_resp.has_status());

  auto status = mb_resp.status();
  return {true, {status.master_status(), status.slave_status()}};
}

bool RPCTarget::writeMBStatus(uint32_t master_status, uint32_t slave_status) {
  // Prepare request to write the mailbox status
  simulator_api::Request request;

  auto mb = new MailBoxMessage();
  mb->set_type(MailBoxAccessType::MB_WRITE);
  mb->set_req_status(MailBoxAccessStatus::MB_STATUS_NONE);

  auto mb_status = new MailBoxStatus();
  mb_status->set_master_status(master_status);
  mb_status->set_slave_status(slave_status);
  mb->set_allocated_status(mb_status);
  request.set_allocated_mailbox(mb);

  // Do RPC and wait for reply
  auto reply = doRPC(request);
  assert(reply.has_mailbox());
  auto &mb_resp = reply.mailbox();
  // Check that the write was successful
  assert(mb_resp.type() == MailBoxAccessType::MB_WRITE);
  assert(mb_resp.req_status() == MailBoxAccessStatus::MB_STATUS_SUCCESS);
  assert(mb_resp.has_status());

  return true;
}

std::tuple<bool, device_fw::ringbuffer_s> RPCTarget::readRxRb() {
  // Read teh RX ringbuffer
  simulator_api::Request request;

  auto mb = new MailBoxMessage();
  mb->set_type(MailBoxAccessType::MB_READ);
  mb->set_req_status(MailBoxAccessStatus::MB_STATUS_NONE);

  auto rbi = new simulator_api::RingBuffer();
  mb->set_allocated_rx_ring_buffer(rbi);
  request.set_allocated_mailbox(mb);

  // Do RPC and wait for reply
  auto reply = doRPC(request);

  assert(reply.has_mailbox());
  auto &mb_resp = reply.mailbox();
  assert(mb_resp.type() == MailBoxAccessType::MB_READ);
  assert(mb_resp.req_status() == MailBoxAccessStatus::MB_STATUS_SUCCESS);
  assert(mb_resp.has_rx_ring_buffer());

  // Get RX ringbuffer state
  auto rb = mb_resp.rx_ring_buffer();
  device_fw::ringbuffer_s orb = {};
  orb.head_index = rb.head_index();
  orb.tail_index = rb.tail_index();
  assert(rb.queue().size() == RINGBUFFER_LENGTH);
  memcpy(orb.queue, rb.queue().data(), RINGBUFFER_LENGTH);

  return {true, orb};
}

bool RPCTarget::writeRxRb(const device_fw::ringbuffer_s &rb) {
  // Write the RX ring buffer, prepare request
  simulator_api::Request request;

  auto mb = new MailBoxMessage();
  mb->set_type(MailBoxAccessType::MB_WRITE);
  mb->set_req_status(MailBoxAccessStatus::MB_STATUS_NONE);

  auto rbw = new simulator_api::RingBuffer();
  // Write the ring buffer state
  rbw->set_head_index(rb.head_index);
  rbw->set_tail_index(rb.tail_index);
  rbw->set_queue(rb.queue, RINGBUFFER_LENGTH);
  mb->set_allocated_rx_ring_buffer(rbw);

  request.set_allocated_mailbox(mb);

  // Do RPC and wait for reply
  auto reply = doRPC(request);

  // Check that the write was succesful
  assert(reply.has_mailbox());
  auto &mb_resp = reply.mailbox();
  assert(mb_resp.type() == MailBoxAccessType::MB_WRITE);
  assert(mb_resp.req_status() == MailBoxAccessStatus::MB_STATUS_SUCCESS);
  assert(mb_resp.has_rx_ring_buffer());
  return true;
}

// FIXME in the future see if the two read*Rb and write*Rb functions
// could be combined

std::tuple<bool, device_fw::ringbuffer_s> RPCTarget::readTxRb() {
  // Read the TX ring buffer

  simulator_api::Request request;
  auto mb = new MailBoxMessage();
  mb->set_type(MailBoxAccessType::MB_READ);
  mb->set_req_status(MailBoxAccessStatus::MB_STATUS_NONE);

  auto rbi = new simulator_api::RingBuffer();
  mb->set_allocated_tx_ring_buffer(rbi);
  request.set_allocated_mailbox(mb);

  // Do RPC and wait for reply
  auto reply = doRPC(request);
  assert(reply.has_mailbox());
  auto &mb_resp = reply.mailbox();
  assert(mb_resp.type() == MailBoxAccessType::MB_READ);
  assert(mb_resp.req_status() == MailBoxAccessStatus::MB_STATUS_SUCCESS);
  assert(mb_resp.has_tx_ring_buffer());
  auto rb = mb_resp.tx_ring_buffer();

  device_fw::ringbuffer_s orb = {};
  orb.head_index = rb.head_index();
  orb.tail_index = rb.tail_index();
  assert(rb.queue().size() == RINGBUFFER_LENGTH);
  memcpy(orb.queue, rb.queue().data(), RINGBUFFER_LENGTH);

  return {true, orb};
}

bool RPCTarget::writeTxRb(const device_fw::ringbuffer_s &rb) {
  // Write the TX ringbuffer state
  simulator_api::Request request;

  auto mb = new MailBoxMessage();
  mb->set_type(MailBoxAccessType::MB_WRITE);
  mb->set_req_status(MailBoxAccessStatus::MB_STATUS_NONE);

  auto rbw = new simulator_api::RingBuffer();
  rbw->set_head_index(rb.head_index);
  rbw->set_tail_index(rb.tail_index);
  rbw->set_queue(rb.queue, RINGBUFFER_LENGTH);
  mb->set_allocated_tx_ring_buffer(rbw);
  request.set_allocated_mailbox(mb);

  // Do RPC and wait for reply
  auto reply = doRPC(request);
  assert(reply.has_mailbox());

  auto &mb_resp = reply.mailbox();
  assert(mb_resp.type() == MailBoxAccessType::MB_WRITE);
  assert(mb_resp.req_status() == MailBoxAccessStatus::MB_STATUS_SUCCESS);
  assert(mb_resp.has_tx_ring_buffer());

  return true;
}
#endif // ENABLE_DEVICE_FW

bool RPCTarget::raiseDevicePuPlicPcieMessageInterrupt() {
  simulator_api::Request request;
  auto device_interrupt = new DeviceInterrupt();
  device_interrupt->set_type(simulator_api::DeviceInterruptType::PU_PLIC_PCIE_MESSAGE_INTERRUPT);
  request.set_allocated_device_interrupt(device_interrupt);
  // Do RPC and wait for reply
  auto reply = doRPC(request);
  assert(reply.has_interrupt());
  auto &interrupt_rsp = reply.interrupt();
  assert(interrupt_rsp.success());
  return true;
}

bool RPCTarget::raiseDeviceMasterShireIpiInterrupt() {
  simulator_api::Request request;
  auto device_interrupt = new DeviceInterrupt();
  device_interrupt->set_type(simulator_api::DeviceInterruptType::MASTER_SHIRE_IPI_INTERRUPT);
  request.set_allocated_device_interrupt(device_interrupt);
  // Do RPC and wait for reply
  auto reply = doRPC(request);
  assert(reply.has_interrupt());
  auto &interrupt_rsp = reply.interrupt();
  assert(interrupt_rsp.success());
  return true;
}

bool RPCTarget::waitForHostInterrupt(TimeDuration wait_time) {
  simulator_api::Request request;
  auto host_interrupt = new HostInterrupt();
  request.set_allocated_host_interrupt(host_interrupt);
  // Do RPC and wait for reply
  auto reply = doRPC(request);
  assert(reply.has_interrupt());
  auto &interrupt_rsp = reply.interrupt();
  assert(interrupt_rsp.success());
  return true;
}

ssize_t RPCTarget::mboxMsgMaxSize() const {
  return mailboxDev_->mboxMaxMsgSize();
}

bool RPCTarget::mb_write(const void *data, ssize_t size) {
  bool res = false;
#if ENABLE_DEVICE_FW
  res = mailboxDev_->write(data, size);
#else
  abort();
#endif // ENABLE_DEVICE_FW
  return res;
}
ssize_t RPCTarget::mb_read(void *data, ssize_t size, TimeDuration wait_time) {
  ssize_t res = false;
#if ENABLE_DEVICE_FW
  res = mailboxDev_->read(data, size, wait_time);
#else
  abort();
#endif // ENABLE_DEVICE_FW
  return res;
}

bool RPCTarget::launch() {
  bool res = true;
#if !ENABLE_DEVICE_FW
  res = raiseDeviceMasterShireIpiInterrupt();
#endif // ENABLE_DEVICE_FW
  return res;
}

bool RPCTarget::boot(uint64_t pc) {
  simulator_api::Request request;
  auto boot_req = new BootReq();
  boot_req->set_pc(pc);
  request.set_allocated_boot(boot_req);
  // Do RPC and wait for reply
  auto reply = doRPC(request);
  assert(reply.has_boot());
  auto &boot_rsp = reply.boot();
  assert(boot_rsp.success());
  return true;
}

bool RPCTarget::shutdown() {
  simulator_api::Request request;
  auto shutdown_req = new ShutdownReq();
  request.set_allocated_shutdown(shutdown_req);
  // Do RPC and wait for reply
  auto reply = doRPC(request);
  assert(reply.has_shutdown());
  auto &shutdown_rsp = reply.shutdown();
  assert(shutdown_rsp.success());
  return true;
}

} // namespace device
} // namespace et_runtime
