#include "et-card-proxy.h"
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

void cpOpen(CardProxy *card_proxy, bool is_use_dev) {
  if (is_use_dev) {
    card_proxy->fd = open(ESPRT_DEVICE_FILENAME, O_RDWR | O_CLOEXEC);
    if (card_proxy->fd < 0) {
      fprintf(stderr, "Can't open Esperanto device file '%s': %s\n",
              ESPRT_DEVICE_FILENAME, strerror(errno));
      abort();
    }
  } else {
    card_proxy->fd = initSocket(false);
  }
  card_proxy->is_use_dev = is_use_dev;
}

void cpClose(CardProxy *card_proxy) {
  if (card_proxy->fd != -1) {
    close(card_proxy->fd);
    card_proxy->fd = -1;
  }
}

void cpDefineDevMem(CardProxy *card_proxy, uintptr_t dev_addr, size_t size,
                    bool is_exec) {
  if (card_proxy->is_use_dev) {
    struct esprt_ioctl_cmd_define_t cmd = {
        .addr = dev_addr,
        .size = size,
        .is_exec = is_exec,
    };
    ABORT_IF(ioctl(card_proxy->fd, ESPRT_IOCTL_CMD_DEFINE, &cmd) != 0,
             "ESPRT_IOCTL_CMD_DEFINE failed");
    return;
  }

  {
    MessageDefineReq msg = {};
    msg.devPtr = (void *)dev_addr;
    msg.size = size;
    msg.is_exec = is_exec;
    sendMessage(card_proxy->fd, &msg, kMessageTypeDefineReq,
                sizeof(MessageDefineReq));
  }

  {
    MessageType msg_type;
    MessageDefineRes *msg_p =
        (MessageDefineRes *)recvMessage(card_proxy->fd, &msg_type);
    ABORT_IF(msg_type != kMessageTypeDefineRes, "Expect kMessageTypeDefineRes");
    free(msg_p);
  }
}

void cpReadDevMem(CardProxy *card_proxy, uintptr_t dev_addr, size_t size,
                  void *buf) {
  if (card_proxy->is_use_dev) {
    struct esprt_ioctl_cmd_read_t cmd = {
        .addr = dev_addr,
        .size = size,
        .user_buf = buf,
    };
    ABORT_IF(ioctl(card_proxy->fd, ESPRT_IOCTL_CMD_READ, &cmd) != 0,
             "ESPRT_IOCTL_CMD_READ failed");
    return;
  }

  {
    MessageReadReq msg = {};
    msg.devPtr = (void *)dev_addr;
    msg.size = size;
    sendMessage(card_proxy->fd, &msg, kMessageTypeReadReq,
                sizeof(MessageReadReq));
  }

  {
    MessageType msg_type;
    MessageReadRes *msg_p =
        (MessageReadRes *)recvMessage(card_proxy->fd, &msg_type);
    ABORT_IF(msg_type != kMessageTypeReadRes, "Expect kMessageTypeReadRes");
    assert(msg_p->mem_size == size);
    memcpy(buf, msg_p->mem, size);
    free(msg_p);
  }
}

void cpWriteDevMem(CardProxy *card_proxy, uintptr_t dev_addr, size_t size,
                   const void *buf) {
  if (card_proxy->is_use_dev) {
    struct esprt_ioctl_cmd_write_t cmd = {
        .addr = dev_addr,
        .size = size,
        .user_buf = (void *)buf,
    };
    ABORT_IF(ioctl(card_proxy->fd, ESPRT_IOCTL_CMD_WRITE, &cmd) != 0,
             "ESPRT_IOCTL_CMD_WRITE failed");
    return;
  }

  {
    size_t mem_size = size;
    size_t msg_size = sizeof(MessageWriteReq) + mem_size;
    MessageWriteReq *msg_p = (MessageWriteReq *)malloc(msg_size);
    memset(msg_p, 0, msg_size);
    msg_p->devPtr = (void *)dev_addr;
    msg_p->mem_size = mem_size;
    memcpy(msg_p->mem, buf, mem_size);
    sendMessage(card_proxy->fd, msg_p, kMessageTypeWriteReq, msg_size);
    free(msg_p);
  }

  {
    MessageType msg_type;
    MessageWriteRes *msg_p =
        (MessageWriteRes *)recvMessage(card_proxy->fd, &msg_type);
    ABORT_IF(msg_type != kMessageTypeWriteRes, "Expect kMessageTypeWriteRes");
    free(msg_p);
  }
}

void cpLaunch(CardProxy *card_proxy, uintptr_t launch_pc) {
  if (card_proxy->is_use_dev) {
    struct esprt_ioctl_cmd_launch_t cmd = {
        .launch_pc = launch_pc,
    };
    ABORT_IF(ioctl(card_proxy->fd, ESPRT_IOCTL_CMD_LAUNCH, &cmd) != 0,
             "ESPRT_IOCTL_CMD_LAUNCH failed");
    return;
  }

  {
    MessageLaunchReq msg = {};
    msg.launch_pc = launch_pc;
    sendMessage(card_proxy->fd, &msg, kMessageTypeLaunchReq,
                sizeof(MessageLaunchReq));
  }

  {
    MessageType msg_type;
    MessageLaunchRes *msg_p =
        (MessageLaunchRes *)recvMessage(card_proxy->fd, &msg_type);
    ABORT_IF(msg_type != kMessageTypeLaunchRes, "Expect kMessageTypeLaunchRes");
    free(msg_p);
  }
}

void cpBoot(CardProxy *card_proxy, uintptr_t init_pc, uintptr_t trap_pc) {
  if (card_proxy->is_use_dev) {
    struct esprt_ioctl_cmd_boot_t cmd = {
        .init_pc = init_pc,
        .trap_pc = trap_pc,
    };
    ABORT_IF(ioctl(card_proxy->fd, ESPRT_IOCTL_CMD_BOOT, &cmd) != 0,
             "ESPRT_IOCTL_CMD_BOOT failed");
    return;
  }

  {
    MessageBootReq msg = {};
    msg.init_pc = init_pc;
    msg.trap_pc = trap_pc;
    sendMessage(card_proxy->fd, &msg, kMessageTypeBootReq,
                sizeof(MessageBootReq));
  }

  {
    MessageType msg_type;
    MessageBootRes *msg_p =
        (MessageBootRes *)recvMessage(card_proxy->fd, &msg_type);
    ABORT_IF(msg_type != kMessageTypeBootRes, "Expect kMessageTypeBootRes");
    free(msg_p);
  }
}
