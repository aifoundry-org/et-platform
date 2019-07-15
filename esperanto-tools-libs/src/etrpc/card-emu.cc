#include "card-emu.h"
#include "et-rpc.h"
#include <assert.h>
#include <cstdlib>
#include <ctime>
#include <fcntl.h>
#include <malloc.h>
#include <memory.h>
#include <memory>
#include <signal.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <inttypes.h>

using namespace std;

void CardEmu::shutdown() { gBackendInterface_->shutdown(""); }

bool CardEmu::init() {
  gBackendInterface_->setupCommunicationSocket(sysemu_socket_path_.c_str());
  gBackendInterface_->acceptCommunicationSocket();
  return true;
}

MessageDefineRes CardEmu::defineReq(MessageDefineReq *msg_p) {
  printf("MessageDefineReq(devPtr=%p, size=%ld, is_exec=%d)\n", msg_p->devPtr,
         msg_p->size, msg_p->is_exec);

  if (msg_p->is_exec) {
    gBackendInterface_->defineExecMemory(msg_p->devPtr, msg_p->size);
  } else {
    gBackendInterface_->defineMemory(msg_p->devPtr, msg_p->size);
  }

  MessageDefineRes res_msg = {};
  return res_msg;
}

std::unique_ptr<MessageReadRes> CardEmu::readMemReq(MessageReadReq *msg_p) {
  printf("MessageReadReq(devPtr=%p, size=%ld)\n", msg_p->devPtr, msg_p->size);

  size_t mem_size = msg_p->size;
  size_t res_msg_size = sizeof(MessageReadRes) + mem_size;
  std::unique_ptr<MessageReadRes> res_msg_p =
      std::unique_ptr<MessageReadRes>((MessageReadRes *)malloc(res_msg_size));
  memset(res_msg_p.get(), 0, res_msg_size);
  res_msg_p->mem_size = mem_size;
  gBackendInterface_->readMemory(res_msg_p->mem, msg_p->devPtr, mem_size);
  return res_msg_p;
}

MessageWriteRes CardEmu::writeMemReq(MessageWriteReq *msg_p) {
  printf("MessageWriteReq(devPtr=%p, mem_size=%ld)\n", msg_p->devPtr,
         msg_p->mem_size);

  gBackendInterface_->writeMemory(msg_p->mem, msg_p->devPtr, msg_p->mem_size);

  MessageWriteRes res_msg = {};
  return res_msg;
}

MessageBootRes CardEmu::bootReq(MessageBootReq *msg_p) {
  printf("MessageBootReq(init_pc=0x%lx, trap_pc=0x%lx)\n", msg_p->init_pc,
         msg_p->trap_pc);

  gBackendInterface_->boot(msg_p->init_pc, msg_p->trap_pc);

  MessageBootRes res_msg = {};
  return res_msg;
}

MessageLaunchRes CardEmu::launchReq(MessageLaunchReq *msg_p) {
  printf("MessageLaunchReq(launch_pc=0x%lx)\n", msg_p->launch_pc);

    fprintf(stderr,
            "CardEmu::Going to execute kernel {0x%lx}\n"
            "  tensor_a = 0x%" PRIx64 "\n"
            "  tensor_b = 0x%" PRIx64 "\n"
            "  tensor_c = 0x%" PRIx64 "\n"
            "  tensor_d = 0x%" PRIx64 "\n"
            "  tensor_e = 0x%" PRIx64 "\n"
            "  tensor_f = 0x%" PRIx64 "\n"
            "  tensor_g = 0x%" PRIx64 "\n"
            "  tensor_h = 0x%" PRIx64 "\n"
            "  pc/id    = 0x%" PRIx64 "\n",
            msg_p->launch_pc,
            msg_p->params.tensor_a, msg_p->params.tensor_b, msg_p->params.tensor_c,
            msg_p->params.tensor_d, msg_p->params.tensor_e, msg_p->params.tensor_f,
            msg_p->params.tensor_g, msg_p->params.tensor_h, msg_p->params.kernel_id);

  gBackendInterface_->launch(msg_p->launch_pc, msg_p->params);

  MessageLaunchRes res_msg = {};
  return res_msg;
}
