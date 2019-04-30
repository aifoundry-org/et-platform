#ifndef ETRPC_ETRPC_H
#define ETRPC_ETRPC_H

#include <cstddef>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum MessageType {
  kMessageTypeBootReq,
  kMessageTypeBootRes,
  kMessageTypeDefineReq,
  kMessageTypeDefineRes,
  kMessageTypeReadReq,
  kMessageTypeReadRes,
  kMessageTypeWriteReq,
  kMessageTypeWriteRes,
  kMessageTypeLaunchReq,
  kMessageTypeLaunchRes,
};
typedef enum MessageType MessageType;

typedef struct MessageBootReq {
  uintptr_t init_pc;
  uintptr_t trap_pc;
} MessageBootReq;

typedef struct MessageBootRes {
} MessageBootRes;

typedef struct MessageDefineReq {
  void *devPtr;
  size_t size;
  int is_exec;
} MessageDefineReq;

typedef struct MessageDefineRes {
} MessageDefineRes;

typedef struct MessageReadReq {
  void *devPtr;
  size_t size;
} MessageReadReq;

typedef struct MessageReadRes {
  size_t mem_size;
  uint8_t mem[0];
} MessageReadRes;

typedef struct MessageWriteReq {
  void *devPtr;
  size_t mem_size;
  uint8_t mem[0];
} MessageWriteReq;

typedef struct MessageWriteRes {
} MessageWriteRes;

typedef struct MessageLaunchReq {
  uintptr_t launch_pc;
} MessageLaunchReq;

typedef struct MessageLaunchRes {
} MessageLaunchRes;

void sendMessage(int fd, const void *msg_p, MessageType msg_type,
                 size_t msg_size);
void *recvMessage(int fd, MessageType *msg_type_p);
int initSocket(bool is_server);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // ETRPC_ETRPC_H
