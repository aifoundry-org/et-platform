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
  char sock_path[PATH_MAX];
  sprintf(sock_path, "%s/%s", work_dir, getCommunicationFile());
  setupCommunicationSocket(sock_path);

  // Path to esperanto-soc sets by env.sh
  const char *bemu_path = getenv("BEMU");
  std::vector<std::string> execute_args = {
      std::string(bemu_path) + "/sys_emu/build/sys_emu", "-minions",
      "FFFFFFFF",            // All minions enabled
      "-shires", "FFFFFFFF", // All shires enabled
      std::string("-api_comm"),
      std::string(work_dir) + "/" +
          getCommunicationFile(), // Communication file
      "-mins_dis", // Disable minions by default as booting is done through an
                   // exec command
      // "-l","-lm","0" // Enable logging of minion0
      "-max_cycles", "800000", // Limiting number of virtual simulation cycles.
                               // Increase if needed
  };

  createEmuProcess(execute_args[0].c_str(), execute_args);

  acceptCommunicationSocket();
}

void BackendSysEmu::boot(uint64_t init_pc, uint64_t trap_pc) {
  char cmd = kIPIContinue;
  writeBytesIntoFd(&cmd, 1);
}
