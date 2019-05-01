#ifndef ETRPC_CARD_PROXY_H
#define ETRPC_CARD_PROXY_H

#include <cassert>
#include <cstddef>
#include <exception>
#include <memory>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string>
#include <unistd.h>

class CardEmu;

struct CardProxy {
public:
  CardProxy(const std::string &path);

  ~CardProxy();

  CardEmu &card_emu() { return *card_emu_; }

private:
  std::string connection_;
  // HACK shoud get rid of this #$@!#$!4 eventually;
  std::unique_ptr<CardEmu> card_emu_;
};

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


#endif // ETRPC_CARD_PROXY_H
