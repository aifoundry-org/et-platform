#ifndef ET_SOCKET_ADDR_H
#define ET_SOCKET_ADDR_H


#include <unistd.h>
#include <fcntl.h>
#include <et-misc.h>
#include <linux/limits.h>
#include <stdio.h>

EXAPI void  etrtSetClientSocket(pid_t card_emu_pid);
EXAPI char* etrtGetClientSocket();

#endif //ET_SOCKET_ADDR_H