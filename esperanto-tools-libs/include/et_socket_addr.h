#ifndef ET_SOCKET_ADDR_H
#define ET_SOCKET_ADDR_H

#include <Common/et-misc.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <stdio.h>
#include <unistd.h>

EXAPI void  etrtSetClientSocket(pid_t card_emu_pid);
EXAPI char* etrtGetClientSocket();

#endif // ET_SOCKET_ADDR_H
