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

#include <esperanto-fw/firmware_helpers/layout.h>

#include <inttypes.h>
#include <thread>
#include <vector>

using namespace std;
using namespace simulator_api;

namespace et_runtime {
namespace device {

namespace {
  TimeDuration kPollingInterval = std::chrono::milliseconds(100);
  // TODO: Remove when device interface registers are available
  const uint64_t kVirtQueueDescAddr = 0x8005100000ULL;
  const uint16_t kAlignmentSize = 64;
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

uint32_t RPCGenerator::rpcWaitForHostInterruptAny(TimeDuration wait_time) {
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

bool RPCGenerator::rpcWaitForHostInterrupt(uint8_t queueId, TimeDuration wait_time) {
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
  return device_alive_ = rpcGen_->rpcInit(socket_path_);
}

bool RPCTargetMM::boot(uint32_t shire_id, uint32_t thread0_enable, uint32_t thread1_enable) {
  return rpcGen_->rpcBootShire(shire_id, thread0_enable, thread1_enable);
}

bool RPCTargetMM::virtQueuesDiscover(TimeDuration wait_time) {
  auto start = Clock::now();
  auto end = start + wait_time;
  struct device_fw::vqueue_desc vqDescMM;

  uint8_t ready = 0;
  while (1) {
    auto success = rpcGen_->rpcWaitForHostInterruptAny() > 0;
    assert(success);

    success = rpcGen_->rpcMemoryRead(kVirtQueueDescAddr, sizeof(ready), &ready);
    assert(success);

    if (ready == 1) {
      success = rpcGen_->rpcMemoryRead(kVirtQueueDescAddr, sizeof(vqDescMM), &vqDescMM);
      assert(success);

      queueCount_ = vqDescMM.queue_count;
      queueBufCount_ = vqDescMM.queue_element_count;
      queueBufSize_ = vqDescMM.queue_element_size;
      maxMsgSize_ = queueBufSize_ - sizeof(device_fw::vqueue_buf_header);
      RTINFO << "VirtQueue Desc: queueCount_: " << queueCount_
             << " queueBufCount_: " << queueBufCount_
             << " queueBufSize_: " << queueBufSize_;

      // Set host ready
      vqDescMM.host_ready = 1;
      success = rpcGen_->rpcMemoryWrite(kVirtQueueDescAddr + offsetof(device_fw::vqueue_desc, host_ready),
                                        sizeof(vqDescMM.host_ready), &vqDescMM.host_ready);
      assert(success);
      break;
    }
    std::this_thread::sleep_for(kPollingInterval);
    if (end < Clock::now()) {
      break;
    }
  }

  if (ready != 1) {
    return false;
  }

  uint64_t queueInfoAddr = kVirtQueueDescAddr + sizeof(device_fw::vqueue_desc);
  uint64_t queueBaseAddr = vqDescMM.queue_addr;

  assert(queueCount_ < 32);
  for (uint8_t queueId = 0; queueId < queueCount_; queueId++) {
    auto res = createVirtQueue(queueId, queueBaseAddr, queueInfoAddr);
    assert(get<0>(res) == queueId);

    queueInfoAddr += sizeof(device_fw::vqueue_info);

    auto alignedQueueSize =
      ((queueBufSize_ * queueBufCount_ - 1) / kAlignmentSize + 1 ) * kAlignmentSize;
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

    // we are resetting the virtual queues
    success = rpcGen_->rpcWaitForHostInterrupt(queueId, std::chrono::seconds(30));

    // For DeviceFW reset the virtqueue as well and wait for device-fw to be ready
    success = virtQueueDev->ready(std::chrono::seconds(20));
    assert(success);

    success = virtQueueDev->reset(std::chrono::seconds(20));
    assert(success);

    success = virtQueueDev->ready(std::chrono::seconds(20));
    assert(success);
  }
  return true;
}

bool RPCTargetMM::deinit() {
  for (uint8_t queueId = 0; queueId < queueCount_; queueId++) {
    assert(destroyVirtQueue(queueId));
  }
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

ssize_t RPCTargetMM::mb_read(void *data, ssize_t size, TimeDuration wait_time) {
  assert(true);
  return 0;
}

bool RPCTargetMM::virtQueueWrite(const void *data, ssize_t size, uint8_t queueId) {
  assert(device_alive_);
  assert((size_t)size <= maxMsgSize_);
  auto virtQueueDev = getVirtQueue(queueId);
  return virtQueueDev->write(data, size);
}

ssize_t RPCTargetMM::virtQueueRead(void *data, ssize_t size, uint8_t queueId, TimeDuration wait_time) {
  assert(device_alive_);
  assert((size_t)size <= maxMsgSize_);
  auto virtQueueDev = getVirtQueue(queueId);
  return virtQueueDev->read(data, size, wait_time);
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
  // Mark the device as not alive ahead of time so that we can
  // do a proper teardown
  device_alive_ = false;
  return rpcGen_->rpcShutdown();
}

} // namespace device
} // namespace et_runtime
