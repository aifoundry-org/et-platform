#ifndef ETRPC_CARD_PROXY_H
#define ETRPC_CARD_PROXY_H

#include <cstddef>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct CardProxy {
  int fd;
  bool is_use_dev;
};
typedef struct CardProxy CardProxy;

void cpOpen(CardProxy *card_proxy, bool is_use_dev);
void cpClose(CardProxy *card_proxy);
void cpDefineDevMem(CardProxy *card_proxy, uintptr_t dev_addr, size_t size,
                    bool is_exec);
void cpReadDevMem(CardProxy *card_proxy, uintptr_t dev_addr, size_t size,
                  void *buf);
void cpWriteDevMem(CardProxy *card_proxy, uintptr_t dev_addr, size_t size,
                   const void *buf);
void cpLaunch(CardProxy *card_proxy, uintptr_t launch_pc);
void cpBoot(CardProxy *card_proxy, uintptr_t init_pc, uintptr_t trap_pc);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // ETRPC_CARD_PROXY_H
