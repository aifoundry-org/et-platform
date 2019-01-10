#ifndef _EMU_MEMOP_H
#define _EMU_MEMOP_H

#if defined(__cplusplus) && (__cplusplus >= 201103L)
#include <cinttypes>
#else
#include <inttypes.h>
#endif

extern uint8_t  (*vmemread8 ) (uint64_t addr);
extern uint16_t (*vmemread16) (uint64_t addr);
extern uint32_t (*vmemread32) (uint64_t addr);
extern uint64_t (*vmemread64) (uint64_t addr);

extern void (*vmemwrite8 ) (uint64_t addr, uint8_t  data);
extern void (*vmemwrite16) (uint64_t addr, uint16_t data);
extern void (*vmemwrite32) (uint64_t addr, uint32_t data);
extern void (*vmemwrite64) (uint64_t addr, uint64_t data);

// extern uint64_t pmemread64(uint64_t addr);
extern void (*pmemwrite64) (uint64_t addr, uint64_t data);

#endif // _EMU_MEMOP_H
