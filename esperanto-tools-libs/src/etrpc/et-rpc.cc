#include "et-rpc.h"
#include "Support/Logging.h"
#include "et_socket_addr.h"
#include <assert.h>
#include <malloc.h>
#include <memory.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

typedef size_t MessageHeader;

static MessageHeader packMessageHeader(MessageType msg_type, size_t msg_size) {
  return (msg_size << 8) | msg_type;
}

static MessageType unpackMessageHeader(MessageHeader msg_hdr,
                                       size_t *msg_size_p) {
  *msg_size_p = (msg_hdr >> 8);
  return (MessageType)(msg_hdr & 0xFF);
}

void sendMessage(int fd, const void *msg_p, MessageType msg_type,
                 size_t msg_size) {
  RTINFO << "Sending Message " << msg_type;
  ssize_t r;
  MessageHeader msg_hdr = packMessageHeader(msg_type, msg_size);

  r = write(fd, &msg_hdr, sizeof(msg_hdr));
  assert(r == sizeof(msg_hdr));

  r = write(fd, msg_p, msg_size);
  assert((size_t)r == msg_size);
}

/*
 * Returns:
 *      - NULL if peer has performed an orderly shutdown,
 *      - received message which should be freed via free().
 */
void *recvMessage(int fd, MessageType *msg_type_p) {
  ssize_t r;
  MessageHeader msg_hdr;

  r = read(fd, &msg_hdr, sizeof(msg_hdr));
  if (r == 0) {
    return NULL;
  }
  assert(r == sizeof(msg_hdr));

  size_t msg_size;
  *msg_type_p = unpackMessageHeader(msg_hdr, &msg_size);

  void *msg_p = malloc(msg_size);
  assert(msg_p);

  r = read(fd, msg_p, msg_size);
  assert((size_t)r == msg_size);

  return msg_p;
}

static int initRemoteClient(const char *host, const char *port) {
  struct addrinfo hints;
  struct addrinfo *result, *rp;
  int sfd = -1, s;

  /* Obtain address(es) matching host/port */

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC; /* Allow IPv4 or IPv6 */
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = 0;
  hints.ai_protocol = 0; /* Any protocol */

  s = getaddrinfo(host, port, &hints, &result);
  if (s != 0) {
    fprintf(stderr, "getaddrinfo: %s (%s)\n", gai_strerror(s), host);
    exit(EXIT_FAILURE);
  }

  /* getaddrinfo() returns a list of address structures.
    Try each address until we successfully connect(2).
    If socket(2) (or connect(2)) fails, we (close the socket
    and) try the next address. */

  for (rp = result; rp != NULL; rp = rp->ai_next) {
    sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (sfd == -1) {
      continue;
    }

    if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1) {
      break; /* Success */
    }

    close(sfd);
  }

  if (rp == NULL) /* No address succeeded */
  {
    fprintf(stderr, "Could not connect (%s)\n", host);
    exit(EXIT_FAILURE);
  }

  freeaddrinfo(result); /* No longer needed */

  return sfd;
}

static int initRemoteServer(const char *port) {
  struct addrinfo hints;
  struct addrinfo *result, *rp;
  int sfd = -1, s;

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC; /* Allow IPv4 or IPv6 */
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE; /* For wildcard IP address */
  hints.ai_protocol = 0;       /* Any protocol */
  hints.ai_canonname = NULL;
  hints.ai_addr = NULL;
  hints.ai_next = NULL;

  s = getaddrinfo(NULL, port, &hints, &result);
  if (s != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
    exit(EXIT_FAILURE);
  }

  /**
   * getaddrinfo() returns a list of address structures.
   * Try each address until we successfully bind(2).
   * If socket(2) (or bind(2)) fails, we (close the socket
   * and) try the next address.
   */

  for (rp = result; rp != NULL; rp = rp->ai_next) {
    sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (sfd == -1) {
      continue;
    }

    int enable = 1;
    if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
      perror("setsockopt(SO_REUSEADDR) failed");
    }

    if (bind(sfd, rp->ai_addr, rp->ai_addrlen) == 0) {
      break; /* Success */
    }

    close(sfd);
  }

  if (rp == NULL) /* No address succeeded */
  {
    fprintf(stderr, "Could not bind\n");
    exit(EXIT_FAILURE);
  }

  freeaddrinfo(result); /* No longer needed */

  if (listen(sfd, 1) == -1) {
    perror("listen");
    exit(EXIT_FAILURE);
  }

  int cfd = accept(sfd, NULL, NULL);
  if (cfd == -1) {
    perror("accept");
    exit(EXIT_FAILURE);
  }

  close(sfd);

  return cfd;
}

static int initLocalClient(const char *path) {
  struct sockaddr_un addr;

  memset(&addr, 0, sizeof(struct sockaddr_un));
  addr.sun_family = AF_UNIX;
  strcpy(addr.sun_path, path);

  const int sock = socket(AF_UNIX, SOCK_STREAM, 0);
  if (sock == -1) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  const int retries = 3;
  int conn_result = 0;

  for (int i = 0; i < retries; i++) {
    conn_result = connect(sock, (struct sockaddr *)&addr, sizeof(addr));

    if (conn_result == -1) {
      sleep(1);
    } else {
      break;
    }
  }

  if (conn_result == -1) {
    perror("connect");
    exit(EXIT_FAILURE);
  }

  return sock;
}

static int initLocalServer(const char *path) {
  struct sockaddr_un addr;

  memset(&addr, 0, sizeof(struct sockaddr_un));
  addr.sun_family = AF_UNIX;
  strcpy(addr.sun_path, path);

  const int sock = socket(AF_UNIX, SOCK_STREAM, 0);
  if (sock == -1) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  int enable = 1;
  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
    perror("setsockopt(SO_REUSEADDR) failed");
  }

  if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
    perror("bind");
    exit(EXIT_FAILURE);
  }

  if (listen(sock, 1) == -1) {
    perror("listen");
    exit(EXIT_FAILURE);
  }

  int cfd = accept(sock, NULL, NULL);
  if (cfd == -1) {
    perror("accept");
    exit(EXIT_FAILURE);
  }

  close(sock);
  unlink(path);

  return cfd;
}

#include <linux/limits.h>

int initSocket(bool is_server) {
  const char *host = "localhost"; /* default remote host */
  ;
  const char *port = "12003"; /* default remote port */
  const char *local_socket;
  const char *addr;
  char sock_path[PATH_MAX];

  if (is_server) {
    sprintf(sock_path, "%s%d", "/tmp/card_emu_socket_", getpid());
    addr = sock_path;
  } else
    addr = etrtGetClientSocket();

  printf("[ET-RPC] socket address (%s): %s\n", is_server ? "server" : "client",
         addr);
  printf("[ET-RPC] pid            (%s): %d\n", is_server ? "server" : "client",
         getpid());

  bool is_local_socket = false;

  if (addr && (addr[0] != '\0')) {
    const char *colon_pos;

    /*
     * /<some_absolute_path> - use this path as path to local unix domain socket
     * <string> - use <string> as remote host, port is 12003
     * :<string> - use "localhost" as remote host, port is <string>
     * <string1>:<string2> - use <string1> as remote host, port is <string2>
     */
    if (addr[0] == '/') {
      local_socket = addr;
      is_local_socket = true;
    } else if (addr[0] == ':') {
      port = addr + 1;
    } else if ((colon_pos = strchr(addr, ':')) != NULL) {
      int colon_idx = colon_pos - addr;
      static char host_buf[128];
      strncpy(host_buf, addr, 128);
      if (colon_idx >= 128) {
        fprintf(stderr, "host name is too large\n");
        exit(EXIT_FAILURE);
      }
      host_buf[colon_idx] = '\0';
      host = host_buf;
      port = colon_pos + 1;
    } else {
      host = addr;
    }
  }

  if (is_local_socket) {
    if (is_server) {
      return initLocalServer(local_socket);
    } else {
      return initLocalClient(local_socket);
    }
  } else {
    if (is_server) {
      return initRemoteServer(port);
    } else {
      return initRemoteClient(host, port);
    }
  }
}
