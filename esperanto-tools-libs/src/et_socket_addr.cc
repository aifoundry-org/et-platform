#include "et_socket_addr.h"

#include <Common/et-misc.h>
#include <fcntl.h>
#include <fmt/format.h>
#include <linux/limits.h>
#include <stdio.h>
#include <string>
#include <unistd.h>

static std::string sock_path;

void etrtSetClientSocketPath(const std::string &path) { sock_path = path; }

void etrtSetClientSocket(pid_t card_emu_pid) {
  sock_path = fmt::format("/tmp/card_emu_socket_{}", card_emu_pid);
}

const char *etrtGetClientSocket() { return sock_path.c_str(); }
