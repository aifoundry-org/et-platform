#include "backend.h"
#include "et-rpc.h"
#include <assert.h>
#include <cstdlib>
#include <ctime>
#include <fcntl.h>
#include <malloc.h>
#include <memory.h>
#include <signal.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

typedef enum { kBackendUbt, kBackendVcs, kBackendSysEmu } BackendType_t;

Backend *gBackendInterface = 0;
std::string gTempDirectory = "";

static void make_temp_directory() {
  while (true) {
    int tmp_val = std::rand();
    std::stringstream stream;
    stream << std::hex << "/tmp/cardemu_" << tmp_val << "/";
    gTempDirectory = stream.str();

    if (mkdir(gTempDirectory.c_str(), 0744) != -1) {
      break;
    }
  }
}

static void shutdown() {
  if (gBackendInterface) {
    gBackendInterface->shutdown(gTempDirectory.c_str());
  }

  rmdir(gTempDirectory.c_str());

  gTempDirectory = "";

  delete gBackendInterface;
  gBackendInterface = 0;
}

/*
static void shutdown_handler(int sig, siginfo_t *siginfo, void *context) {
  printf("Shutting down.\n");

  shutdown();
}
*/

void run(BackendType_t backend_type) {
  printf("Starting...\n");
  int fd = initSocket(true);
  printf("Connection accepted.\n");

  make_temp_directory();

  switch (backend_type) {
  case kBackendSysEmu:
    gBackendInterface = new BackendSysEmu();
    break;
    // FIXME  Disalbed for now
  // case kBackendVcs:
  //   gBackendInterface = new BackendVcs();
  //   break;
  // case kBackendUbt:
  //   gBackendInterface = new BackendUbt();
  //   break;
  default:
    fprintf(stderr, "Unknown backend\n");
    exit(EXIT_FAILURE);
  }

  gBackendInterface->startup(gTempDirectory.c_str());

  while (true) {
    MessageType msg_type;
    void *any_msg_p = recvMessage(fd, &msg_type);
    if (!any_msg_p) {
      // peer has performed an orderly shutdown
      close(fd);
      break;
    }

    switch (msg_type) {
    case kMessageTypeDefineReq: {
      MessageDefineReq *msg_p = (MessageDefineReq *)any_msg_p;
      printf("MessageDefineReq(devPtr=%p, size=%ld, is_exec=%d)\n",
             msg_p->devPtr, msg_p->size, msg_p->is_exec);

      if (msg_p->is_exec) {
        gBackendInterface->defineExecMemory(msg_p->devPtr, msg_p->size);
      } else {
        gBackendInterface->defineMemory(msg_p->devPtr, msg_p->size);
      }

      MessageDefineRes res_msg = {};
      sendMessage(fd, &res_msg, kMessageTypeDefineRes,
                  sizeof(MessageDefineRes));
      break;
    }

    case kMessageTypeReadReq: {
      MessageReadReq *msg_p = (MessageReadReq *)any_msg_p;
      printf("MessageReadReq(devPtr=%p, size=%ld)\n", msg_p->devPtr,
             msg_p->size);

      size_t mem_size = msg_p->size;
      size_t res_msg_size = sizeof(MessageReadRes) + mem_size;
      MessageReadRes *res_msg_p = (MessageReadRes *)malloc(res_msg_size);
      memset(res_msg_p, 0, res_msg_size);
      res_msg_p->mem_size = mem_size;
      gBackendInterface->readMemory(res_msg_p->mem, msg_p->devPtr, mem_size);
      sendMessage(fd, res_msg_p, kMessageTypeReadRes, res_msg_size);
      free(res_msg_p);
      break;
    }

    case kMessageTypeWriteReq: {
      MessageWriteReq *msg_p = (MessageWriteReq *)any_msg_p;
      printf("MessageWriteReq(devPtr=%p, mem_size=%ld)\n", msg_p->devPtr,
             msg_p->mem_size);

      gBackendInterface->writeMemory(msg_p->mem, msg_p->devPtr,
                                     msg_p->mem_size);

      MessageWriteRes res_msg = {};
      sendMessage(fd, &res_msg, kMessageTypeWriteRes, sizeof(MessageReadRes));
      break;
    }

    case kMessageTypeBootReq: {
      MessageBootReq *msg_p = (MessageBootReq *)any_msg_p;
      printf("MessageBootReq(init_pc=0x%lx, trap_pc=0x%lx)\n", msg_p->init_pc,
             msg_p->trap_pc);

      gBackendInterface->boot(msg_p->init_pc, msg_p->trap_pc);

      MessageBootRes res_msg = {};
      sendMessage(fd, &res_msg, kMessageTypeBootRes, sizeof(MessageBootRes));
      break;
    }

    case kMessageTypeLaunchReq: {
      MessageLaunchReq *msg_p = (MessageLaunchReq *)any_msg_p;
      printf("MessageLaunchReq(launch_pc=0x%lx)\n", msg_p->launch_pc);

      gBackendInterface->launch(msg_p->launch_pc);

      MessageLaunchRes res_msg = {};
      sendMessage(fd, &res_msg, kMessageTypeLaunchRes,
                  sizeof(MessageLaunchRes));
      break;
    }

    default:
      fprintf(stderr, "Received unexpected message type: %d\n", msg_type);
      exit(EXIT_FAILURE);
    }

    free(any_msg_p);
  }

  printf("Session ended.\n");
  shutdown();
}

/*
int main(int argc, char **argv) {
  // Shutdown with Ctrl-C
  {
    struct sigaction act;

    memset(&act, 0, sizeof(act));
    act.sa_sigaction = &shutdown_handler;
    act.sa_flags = SA_SIGINFO;
    sigaction(SIGINT, &act, NULL);
  }

  std::srand(std::time(nullptr));

  BackendType_t backend_type = kBackendUbt;

  std::string exe_name = std::string(argv[0]);
  bool is_once = false;

  for (char **cur_arg = &argv[1]; *cur_arg; cur_arg++) {
    std::string param = std::string(*cur_arg);

    if (param == "--once") {
      is_once = true;
    }
  }

  if (exe_name.find("card-emu-vcs") != std::string::npos) {
    backend_type = kBackendVcs;
  } else if (exe_name.find("card-emu-sysemu") != std::string::npos) {
    backend_type = kBackendSysEmu;
  }

  do {
    run(backend_type);
  } while (!is_once);

  return 0;
}
*/
