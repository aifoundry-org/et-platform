//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "RPCTarget_MM.h"

#include "EmuVirtQueueDev.h"
#include "esperanto/runtime/Support/Logging.h"
#include "esperanto/simulator-api.grpc.pb.h"

#include "esperanto/runtime/Common/layout.h"

#include <inttypes.h>
#include <thread>
#include <vector>

using namespace std;
using namespace simulator_api;

namespace et_runtime {
namespace device {

namespace {
#include "esperanto/runtime/Common/layout.h"

  TimeDuration kPollingInterval = std::chrono::milliseconds(10);

  // To know if virtual queues lie in Mbox region
  const uint64_t kMboxRegionStart = 0x0020007000;
  const uint64_t kMboxRegionEnd = 0x0020008000;

  const uint64_t kMMDevIntfRegAddr = kMboxRegionStart + 0x400UL;
  // TODO: SW-4216: With BAR mappings, need to read this address from specified BAR type and BAR offset in kMMDevIntfRegAddr
  const uint64_t kMMVqueueBase = DEVICE_MM_VQUEUE_BASE;
}

RPCGenerator::RPCGenerator() {}

bool RPCGenerator::rpcInit(const std::string &socketPath) {
  std::shared_ptr<grpc::Channel> channel;
  grpc::ChannelArguments ch_args;
  ch_args.SetMaxReceiveMessageSize(-1);
  channel = grpc::CreateCustomChannel(socketPath, grpc::InsecureChannelCredentials(), ch_args);
  stub_ = SimAPI::NewStub(channel);
  return true;
}

std::pair<bool, simulator_api::Reply> RPCGenerator::doRPC(const simulator_api::Request &request) {
  simulator_api::Reply reply;
  Status status;
  grpc::ClientContext context;
  // Wait until the server is up do not fail immediately
  context.set_wait_for_ready(true);
  status = stub_->SimCommand(&context, request, &reply);
  return {status.ok(), reply};
}

/* RPC */

bool RPCGenerator::rpcBootShire(uint32_t shire_id, uint32_t thread0_enable, uint32_t thread1_enable) {
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

bool RPCGenerator::rpcMemoryRead(uint64_t dev_addr, uint64_t size, void *buf) {
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

bool RPCGenerator::rpcMemoryWrite(uint64_t dev_addr, uint64_t size, const void *buf) {
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

/* Mailbox SRAM region doesn't allow rpcMemoryRead access so using Mailbox RPC
 * request to read virtual queues from Mailbox region
 * TODO: Either enable direct memory access to SRAM regions or add
 * VirtualQueueAccess RPC calls
 */
bool RPCGenerator::rpcVirtQueueRead(uint64_t dev_addr, uint64_t size, void *buf) {
  if (dev_addr >= kMboxRegionStart && dev_addr <= kMboxRegionEnd) {
    uint32_t offset = (uint32_t)(dev_addr - kMboxRegionStart);
    // Create request
    simulator_api::Request request;
    auto mb = new MailboxAccess();
    mb->set_target(MailboxTarget::MAILBOX_TARGET_MM);
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
    assert(mb_resp.target() == MailboxTarget::MAILBOX_TARGET_MM);
    assert(mb_resp.type() == MailboxAccessType::MAILBOX_READ);
    assert(mb_resp.status() == MailboxAccessStatus::MAILBOX_ACCESS_STATUS_SUCCESS);
    assert(mb_resp.offset() == offset);
    assert(mb_resp.size() == size);
    memcpy(buf, reinterpret_cast<const void *>(mb_resp.data().c_str()), size);
    return true;
  }
  else {
    return rpcMemoryRead(dev_addr, size, buf);
  }
}

/* Mailbox SRAM region doesn't allow rpcMemoryWrite access so using Mailbox RPC
 * request to read virtual queues from Mailbox region
 * TODO: Either enable direct memory access to SRAM regions or add
 * VirtualQueueAccess RPC calls
 */
bool RPCGenerator::rpcVirtQueueWrite(uint64_t dev_addr, uint64_t size, const void *buf) {
  if (dev_addr >= kMboxRegionStart && dev_addr <= kMboxRegionEnd) {
    uint32_t offset = (uint32_t)(dev_addr- kMboxRegionStart);
    // Create request
    simulator_api::Request request;
    auto mb = new MailboxAccess();
    mb->set_target(MailboxTarget::MAILBOX_TARGET_MM);
    mb->set_type(MailboxAccessType::MAILBOX_WRITE);
    mb->set_status(MailboxAccessStatus::MAILBOX_ACCESS_STATUS_NONE);
    mb->set_offset(offset);
    mb->set_size(size);
    mb->set_data(buf, size);
    request.set_allocated_mailbox(mb);
    // Do RPC and wait for reply
    auto reply_res = doRPC(request);
    if (!reply_res.first) {
      return false;
    }
    auto reply = reply_res.second;
    assert(reply.has_mailbox());
    auto &mb_resp = reply.mailbox();
    assert(mb_resp.target() == MailboxTarget::MAILBOX_TARGET_MM);
    assert(mb_resp.type() == MailboxAccessType::MAILBOX_WRITE);
    assert(mb_resp.status() == MailboxAccessStatus::MAILBOX_ACCESS_STATUS_SUCCESS);
    assert(mb_resp.offset() == offset);
    assert(mb_resp.size() == size);
    return true;
  }
  else {
    return rpcMemoryWrite(dev_addr, size, buf);
  }
}

bool RPCGenerator::rpcRaiseDevicePuPlicPcieMessageInterrupt() {
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

bool RPCGenerator::rpcRaiseDeviceSpioPlicPcieMessageInterrupt() {
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

uint32_t RPCGenerator::rpcWaitForHostInterruptAny() {
  simulator_api::Request request;
  auto host_interrupt = new HostInterrupt();
  host_interrupt->set_interrupt_bitmap(0xffffffff);
  request.set_allocated_host_interrupt(host_interrupt);
  // Do RPC and wait for reply
  auto reply_res = doRPC(request);

  RTINFO << "Interrupt reply received";
  if (!reply_res.first) {
    return 0;
  }
  auto reply = reply_res.second;

  assert(reply.has_interrupt());
  auto &interrupt_rsp = reply.interrupt();
  if (not interrupt_rsp.success()) {
    RTINFO << "No interrupt raised";
    return 0;
  }
  return interrupt_rsp.interrupt_bitmap();
}

bool RPCGenerator::rpcWaitForHostInterrupt(uint8_t queueId) {
  simulator_api::Request request;
  auto host_interrupt = new HostInterrupt();
  host_interrupt->set_interrupt_bitmap(0x1U << queueId);
  request.set_allocated_host_interrupt(host_interrupt);
  // Do RPC and wait for reply
  auto reply_res = doRPC(request);

  RTINFO << "Interrupt reply received";
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

bool RPCGenerator::rpcShutdown() {
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


RPCTargetMM::RPCTargetMM(int index, const std::string &p)
    : DeviceTarget(index),
      socket_path_(p),
      rpcGen_(std::make_shared<RPCGenerator> ()) {}

bool RPCTargetMM::init() {
  bool ret = rpcGen_->rpcInit(socket_path_);
  if (!ret)
    return false;
  host_mem_access_finish_.store(false);
  host_mem_access_thread_ = std::thread(&RPCTargetMM::hostMemoryAccessThread, this);
  return true;
}

bool RPCTargetMM::boot(uint32_t shire_id, uint32_t thread0_enable, uint32_t thread1_enable) {
  return rpcGen_->rpcBootShire(shire_id, thread0_enable, thread1_enable);
}

bool RPCTargetMM::virtQueuesDiscover(TimeDuration wait_time) {
  auto start = Clock::now();
  auto end = start + wait_time;
  uint16_t queueElementAlign;
  uint16_t queueControlSize;
  device_fw::MM_DEV_INTF_REG_s MMDevIntf;

  auto success = rpcGen_->rpcWaitForHostInterruptAny() > 0;
  assert(success);

  int32_t status = 0;
  while (1) {
    success = rpcGen_->rpcVirtQueueRead(kMMDevIntfRegAddr + offsetof(device_fw::MM_DEV_INTF_REG_s, status),
                                        sizeof(status), &status);
    assert(success);

    // Ensure that MM device interface registers are initialized. This also ignores the state where SP is not up.
    if ((status >= device_fw::MM_DEV_INTF_MM_BOOT_STATUS_DEV_INTF_READY_INITIALIZED) ||
        (status == device_fw::MM_DEV_INTF_MM_BOOT_STATUS_MM_SP_MB_TIMEOUT)) {
      success = rpcGen_->rpcVirtQueueRead(kMMDevIntfRegAddr, sizeof(MMDevIntf), &MMDevIntf);
      assert(success);

      queueCount_ = MMDevIntf.mm_vq.vq_count;
      queueBufCount_ = MMDevIntf.mm_vq.size_info.element_count;
      queueBufSize_ = MMDevIntf.mm_vq.size_info.element_size;
      queueElementAlign = MMDevIntf.mm_vq.size_info.element_alignment;
      queueControlSize = MMDevIntf.mm_vq.size_info.control_size;
      maxMsgSize_ = queueBufSize_ - sizeof(device_fw::vqueue_buf_header);
      RTINFO << "VirtQueue Desc: queueCount_: " << (uint16_t)queueCount_
             << " queueBufCount_: " << queueBufCount_
             << " queueBufSize_: " << queueBufSize_
             << " queueElementAlign: " << queueElementAlign
             << " queueControlSize: " << queueControlSize;
      break;
    }
    std::this_thread::sleep_for(kPollingInterval);
    if (end < Clock::now()) {
      // Discovery failed
      return false;
    }
  }

  uint64_t queueInfoAddr = kMMVqueueBase;
  uint64_t queueBaseAddr = queueInfoAddr + queueControlSize;

  assert(queueCount_ < 32);
  for (uint8_t queueId = 0; queueId < queueCount_; queueId++) {
    auto res = createVirtQueue(queueId, queueBaseAddr, queueInfoAddr);
    assert(get<0>(res) == queueId);

    queueInfoAddr += sizeof(device_fw::vqueue_info);

    auto alignedQueueSize =
      ((queueBufSize_ * queueBufCount_ - 1) / queueElementAlign + 1 ) * queueElementAlign;
    queueBaseAddr += 2 * alignedQueueSize;
  }
  return true;
}

bool RPCTargetMM::postFWLoadInit() {
  assert(device_alive_);
  // We expect that the device-fw is already loaded and "booted" at this point
  auto success = virtQueuesDiscover(std::chrono::seconds(300));
  assert(success);

  for (uint8_t queueId = 0; queueId < queueCount_; queueId++) {
    auto virtQueueDev = getVirtQueue(queueId);

    // reset the virtqueue and wait for device-fw to be ready
    success = virtQueueDev->ready(std::chrono::seconds(20));
    assert(success);

    success = virtQueueDev->reset(std::chrono::seconds(200));
    assert(success);

    success = virtQueueDev->ready(std::chrono::seconds(20));
    assert(success);
  }
  return true;
}

bool RPCTargetMM::deinit() {
  return shutdown();
}

uintptr_t RPCTargetMM::dramBaseAddr() const { return DRAM_MEMMAP_BEGIN; }

uintptr_t RPCTargetMM::dramSize() const { return DRAM_MEMMAP_SIZE; }

bool RPCTargetMM::getStatus() {
  assert(true);
  return false;
}

DeviceInformation RPCTargetMM::getStaticConfiguration() {
  assert(true);
  return {};
}

bool RPCTargetMM::submitCommand() {
  assert(true);
  return false;
}

bool RPCTargetMM::registerResponseCallback() {
  assert(true);
  return false;
}

bool RPCTargetMM::registerDeviceEventCallback() {
  assert(true);
  return false;
}

void RPCTargetMM::hostMemoryAccessThread(void)
{
  // This thread sends "Get Host Memory Accesses" to deal with DMA requests from the simulator
  while (!host_mem_access_finish_.load()) {
    simulator_api::Request request;
    auto req = new GetHostMemoryAccessReq();
    request.set_allocated_get_host_memory_access_req(req);

    // Do RPC and wait for reply
    auto reply_res = rpcGen_->doRPC(request);
    if (!reply_res.first) {
      continue;
    }

    auto reply = reply_res.second;
    assert(reply.has_get_host_memory_access_resp());
    auto &resp = reply.get_host_memory_access_resp();
    // Check if the response is a Host memory access Read or Write
    if (resp.has_read_resp()) {
      auto &read = resp.read_resp();
      uint64_t id = read.id();
      uint64_t host_addr = read.host_addr();
      uint64_t size = read.size();
      RTINFO << "Device requested host memory read: addr: " << std::hex << host_addr << ", size: " << size;

      // Read data from this process' virtual memory space and reply it back
      uint8_t *data = new uint8_t[size];
      memcpy(reinterpret_cast<void *>(data), reinterpret_cast<const void *>(host_addr), size);

      // Send a request with the data
      simulator_api::Request simapi_req;
      auto read_done_req = new HostMemoryAccessReadDoneReq();
      read_done_req->set_id(id);
      read_done_req->set_data(data, size);
      simapi_req.set_allocated_host_memory_access_read_done_req(read_done_req);

      // Do RPC and wait for reply
      auto reply_res = rpcGen_->doRPC(simapi_req);
      if (reply_res.first) {
        assert(reply_res.second.has_host_memory_access_read_done_resp());
      }

      // Free data
      delete[] data;
    } else if (resp.has_write_resp()) {
      auto &write = resp.write_resp();
      uint64_t host_addr = write.host_addr();
      uint64_t size = write.size();
      RTINFO << "Device requested host memory write: addr: " << std::hex << host_addr << ", size: " << size;
      const void *data = reinterpret_cast<const void *>(write.data().c_str());
      // Write data to this process' virtual memory space
      memcpy(reinterpret_cast<void *>(host_addr), data, size);
    } else {
      assert(false);
    }
  }
}

std::pair<uint8_t, EmuVirtQueueDev &>
RPCTargetMM::createVirtQueue(uint8_t queueId, uint64_t queueBaseAddr, uint64_t queueInfoAddr) {
  auto elem = std::find_if(queueStorage_.begin(), queueStorage_.end(),
                           [queueId](decltype(queueStorage_)::value_type &e) {
                             return std::get<0>(e) == queueId;
                           });
  if (elem != queueStorage_.end()) {
    auto &[id, ptr] = *elem;
    return {id, *ptr.get()};
  }

  auto newVirtQueue = std::make_unique<EmuVirtQueueDev>(rpcGen_, queueId,
                                                    queueBaseAddr, queueInfoAddr,
                                                    queueBufCount_, queueBufSize_,
                                                    false);
  auto val =
      std::move(std::make_pair(queueId, std::move(newVirtQueue)));
  queueStorage_.emplace_back(std::move(val));
  return {queueId, *newVirtQueue};
}

EmuVirtQueueDev* RPCTargetMM::getVirtQueue(uint8_t queueId) {

  auto elem = std::find_if(queueStorage_.begin(), queueStorage_.end(),
                           [queueId](decltype(queueStorage_)::value_type &e) {
                             return std::get<0>(e) == queueId;
                           });
  if (elem == queueStorage_.end()) {
    return nullptr;
  }
  return std::get<1>(*elem).get();
}

bool RPCTargetMM::destroyVirtQueue(uint8_t queueId) {
  queueStorage_.erase(
      std::remove_if(queueStorage_.begin(), queueStorage_.end(),
                     [queueId](const decltype(queueStorage_)::value_type &e) {
                       return std::get<0>(e) == queueId;
                     }),
      queueStorage_.end());
  return true;
}

bool RPCTargetMM::readDevMemMMIO(uintptr_t dev_addr, size_t size, void *buf) {
  assert(device_alive_);
  return rpcGen_->rpcMemoryRead(dev_addr, size, buf);
}

bool RPCTargetMM::writeDevMemMMIO(uintptr_t dev_addr, size_t size, const void *buf) {
  assert(device_alive_);
  return rpcGen_->rpcMemoryWrite(dev_addr, size, buf);
}

bool RPCTargetMM::readDevMemDMA(uintptr_t dev_addr, size_t size, void *buf) {
  return readDevMemMMIO(dev_addr, size, buf);
}

bool RPCTargetMM::writeDevMemDMA(uintptr_t dev_addr, size_t size, const void *buf) {
  return writeDevMemMMIO(dev_addr, size, buf);
}

ssize_t RPCTargetMM::mboxMsgMaxSize() const {
  assert(true);
  return 0;
}

bool RPCTargetMM::mb_write(const void *data, ssize_t size) {
  assert(true);
  return false;
}

ssize_t RPCTargetMM::mb_read(void *data, ssize_t size) {
  assert(true);
  return 0;
}

bool RPCTargetMM::virtQueueWrite(const void *data, ssize_t size, uint8_t queueId) {
  assert(device_alive_);
  assert((size_t)size <= maxMsgSize_);
  auto virtQueueDev = getVirtQueue(queueId);
  return virtQueueDev->write(data, size);
}

ssize_t RPCTargetMM::virtQueueRead(void *data, ssize_t size, uint8_t queueId) {
  assert(device_alive_);
  assert((size_t)size <= maxMsgSize_);
  auto virtQueueDev = getVirtQueue(queueId);
  return virtQueueDev->read(data, size);
}

bool RPCTargetMM::waitForEpollEvents(uint32_t &sq_bitmap, uint32_t &cq_bitmap) {
  assert(device_alive_);
  auto bitmap = rpcGen_->rpcWaitForHostInterruptAny();
  if (bitmap == 0) {
    return false;
  }
  for (uint8_t queueId = 0; queueId < queueCount_; queueId++) {
    if (bitmap & (0x1U << queueId)) {
      auto virtQueueDev = getVirtQueue(queueId);
      assert(virtQueueDev != nullptr);
      if (virtQueueDev->checkForEventEPOLLOUT()) {
        sq_bitmap |= (0x1U << queueId);
      }
      if (virtQueueDev->checkForEventEPOLLIN()) {
        cq_bitmap |= (0x1U << queueId);
      }
    }
  }
  return true;
}

bool RPCTargetMM::shutdown() {
  // Mark the device as not alive ahead of time so that we can do a proper teardown
  device_alive_ = false;
  host_mem_access_finish_.store(true);
  rpcGen_->rpcShutdown();

  for (uint8_t queueId = 0; queueId < queueCount_; queueId++) {
    assert(destroyVirtQueue(queueId));
  }

  host_mem_access_thread_.join();

  return true;
}

} // namespace device
} // namespace et_runtime
