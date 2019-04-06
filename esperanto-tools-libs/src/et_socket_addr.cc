#include "et_socket_addr.h"

static char sock_path[PATH_MAX];

void etrtSetClientSocket(pid_t card_emu_pid) {
    sprintf(sock_path, "%s%d", "/tmp/card_emu_socket_", card_emu_pid);
}

char* etrtGetClientSocket() {
    return sock_path;
}