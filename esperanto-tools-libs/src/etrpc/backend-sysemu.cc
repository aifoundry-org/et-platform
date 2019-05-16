#include "backend.h"
#include <assert.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <unistd.h>
#include <vector>

char sock_name[PATH_MAX];

const char *BackendSysEmu::getCommunicationFile() {
  sprintf(sock_name, "%s_%d", "sysemu_socket", getpid());
  return sock_name;
}

void BackendSysEmu::startup(const char *work_dir) {
}

void BackendSysEmu::boot(uint64_t init_pc, uint64_t trap_pc) {
  char cmd = kIPIContinue;
  writeBytesIntoFd(&cmd, 1);
}
