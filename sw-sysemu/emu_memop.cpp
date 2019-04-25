/* vim: set ts=8 sw=4 et sta cin cino=\:0s,l1,g0,N-s,E-s,i0,+2s,(0,W2s : */

#include "emu_memop.h"


static uint8_t host_memread8(uint64_t addr)
{ return * ((uint8_t *) addr); }


static uint16_t host_memread16(uint64_t addr)
{ return * ((uint16_t *) addr); }


static uint32_t host_memread32(uint64_t addr)
{ return * ((uint32_t *) addr); }


static uint64_t host_memread64(uint64_t addr)
{ return * ((uint64_t *) addr); }


static void host_memwrite8(uint64_t addr, uint8_t data)
{ * ((uint8_t *) addr) = data; }


static void host_memwrite16(uint64_t addr, uint16_t data)
{ * ((uint16_t *) addr) = data; }


static void host_memwrite32(uint64_t addr, uint32_t data)
{ * ((uint32_t *) addr) = data; }


static void host_memwrite64(uint64_t addr, uint64_t data)
{ * ((uint64_t *) addr) = data; }


uint8_t  (*pmemread8)  (uint64_t addr) = host_memread8;
uint16_t (*pmemread16) (uint64_t addr) = host_memread16;
uint32_t (*pmemread32) (uint64_t addr) = host_memread32;
uint64_t (*pmemread64) (uint64_t addr) = host_memread64;

void (*pmemwrite8)  (uint64_t addr, uint8_t  data) = host_memwrite8;
void (*pmemwrite16) (uint64_t addr, uint16_t data) = host_memwrite16;
void (*pmemwrite32) (uint64_t addr, uint32_t data) = host_memwrite32;
void (*pmemwrite64) (uint64_t addr, uint64_t data) = host_memwrite64;


void set_memory_funcs(uint8_t  (*func_memread8_ ) (uint64_t),
                      uint16_t (*func_memread16_) (uint64_t),
                      uint32_t (*func_memread32_) (uint64_t),
                      uint64_t (*func_memread64_) (uint64_t),
                      void (*func_memwrite8_ ) (uint64_t, uint8_t ),
                      void (*func_memwrite16_) (uint64_t, uint16_t),
                      void (*func_memwrite32_) (uint64_t, uint32_t),
                      void (*func_memwrite64_) (uint64_t, uint64_t))
{
    pmemread8   = func_memread8_;
    pmemread16  = func_memread16_;
    pmemread32  = func_memread32_;
    pmemread64  = func_memread64_;
    pmemwrite8  = func_memwrite8_;
    pmemwrite16 = func_memwrite16_;
    pmemwrite32 = func_memwrite32_;
    pmemwrite64 = func_memwrite64_;
}
