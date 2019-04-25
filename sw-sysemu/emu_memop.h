#ifndef _EMU_MEMOP_H
#define _EMU_MEMOP_H

#include <cstdint>

// low-level physical memory read accesses
extern uint8_t  (*pmemread8 ) (uint64_t paddr);
extern uint16_t (*pmemread16) (uint64_t paddr);
extern uint32_t (*pmemread32) (uint64_t paddr);
extern uint64_t (*pmemread64) (uint64_t paddr);

// low-level memory write accesses
extern void (*pmemwrite8 ) (uint64_t paddr, uint8_t  data);
extern void (*pmemwrite16) (uint64_t paddr, uint16_t data);
extern void (*pmemwrite32) (uint64_t paddr, uint32_t data);
extern void (*pmemwrite64) (uint64_t paddr, uint64_t data);

void set_memory_funcs(uint8_t  (*func_memread8_ ) (uint64_t),
                      uint16_t (*func_memread16_) (uint64_t),
                      uint32_t (*func_memread32_) (uint64_t),
                      uint64_t (*func_memread64_) (uint64_t),
                      void (*func_memwrite8_ ) (uint64_t, uint8_t ),
                      void (*func_memwrite16_) (uint64_t, uint16_t),
                      void (*func_memwrite32_) (uint64_t, uint32_t),
                      void (*func_memwrite64_) (uint64_t, uint64_t));

#endif // _EMU_MEMOP_H
