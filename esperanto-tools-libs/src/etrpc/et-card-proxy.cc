#include "et-card-proxy.h"
#include "Support/Logging.h"
#include "card-emu.h"
#include "esprt_iface.h"
#include "et-rpc.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <malloc.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define STRINGIFY(s) STRINGIFY_INTERNAL(s)
#define STRINGIFY_INTERNAL(s) #s

#define ABORT_IF(fail_cond, msg)                                               \
  do {                                                                         \
    if (fail_cond) {                                                           \
      fprintf(stderr, __FILE__ ":" STRINGIFY(__LINE__) ": %s: ERROR: %s\n",    \
              __PRETTY_FUNCTION__, msg);                                       \
      abort();                                                                 \
    }                                                                          \
  } while (0)

CardProxy::CardProxy(const std::string &con) : connection_(con) {
  card_emu_ = std::make_unique<CardEmu>(connection_);
}

CardProxy::~CardProxy() {
}

void cpDefineDevMem(CardProxy *card_proxy, uintptr_t dev_addr, size_t size,
                    bool is_exec) {
  RTINFO << "Card Proxy Define Dev Mem: dev_addr: " << std::hex << dev_addr
         << " size: " << size << " is_exec: " << is_exec;

    MessageDefineReq msg = {};
    msg.devPtr = (void *)dev_addr;
    msg.size = size;
    msg.is_exec = is_exec;
    card_proxy->card_emu().defineReq(&msg);
}

void cpReadDevMem(CardProxy *card_proxy, uintptr_t dev_addr, size_t size,
                  void *buf) {
  RTINFO << "Card Proxy Read Dev Mem: dev_addr: " << std::hex << dev_addr
         << " size: " << size << "  buf: " << buf;

    MessageReadReq msg = {};
    msg.devPtr = (void *)dev_addr;
    msg.size = size;

    auto resp = card_proxy->card_emu().readMemReq(&msg);

    assert(resp->mem_size == size);
    memcpy(buf, resp->mem, size);
}

void cpWriteDevMem(CardProxy *card_proxy, uintptr_t dev_addr, size_t size,
                   const void *buf) {
  RTINFO << "Card Proxy Write Dev Mem dev_addr: " << std::hex << dev_addr
         << " size: " << size << " buf: " << buf;
  size_t mem_size = size;
  size_t msg_size = sizeof(MessageWriteReq) + mem_size;
  MessageWriteReq *msg_p = (MessageWriteReq *)malloc(msg_size);
  memset(msg_p, 0, msg_size);
  msg_p->devPtr = (void *)dev_addr;
  msg_p->mem_size = mem_size;
  memcpy(msg_p->mem, buf, mem_size);
  card_proxy->card_emu().writeMemReq(msg_p);
  free(msg_p);
}

void cpLaunch(CardProxy *card_proxy, uintptr_t launch_pc) {
  RTINFO << "Card Proxy Launch pc: " << std::hex << launch_pc;

  MessageLaunchReq msg = {};
  msg.launch_pc = launch_pc;
  card_proxy->card_emu().launchReq(&msg);
}

void cpBoot(CardProxy *card_proxy, uintptr_t init_pc, uintptr_t trap_pc) {
  RTINFO << "Card Proxy Boot init_pc: " << std::hex << init_pc
         << " trap_pc: " << std::hex << trap_pc;
  MessageBootReq msg = {};
  msg.init_pc = init_pc;
  msg.trap_pc = trap_pc;
  card_proxy->card_emu().bootReq(&msg);
}
