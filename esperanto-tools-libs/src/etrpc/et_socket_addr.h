#ifndef ET_SOCKET_ADDR_H
#define ET_SOCKET_ADDR_H

#include <Common/et-misc.h>

#include <string>

EXAPI void etrtSetClientSocketPath(const std::string &socket_path);
EXAPI void  etrtSetClientSocket(pid_t card_emu_pid);
EXAPI const char *etrtGetClientSocket();

#endif // ET_SOCKET_ADDR_H
