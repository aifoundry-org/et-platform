#ifndef _EMU_MEMOP_H
#define _EMU_MEMOP_H

#include <cstdint>

// virtual to physical address translation
extern uint64_t (*vmemtranslate) (uint64_t addr, mem_access_type acc);

// physical memory instruction fetch accesses
extern uint16_t pmemfetch16(uint64_t paddr);

// physical memory read accesses
extern uint8_t  pmemread8  (uint64_t paddr);
extern uint16_t pmemread16 (uint64_t paddr);
extern uint32_t pmemread32 (uint64_t paddr);
extern uint64_t pmemread64 (uint64_t paddr);

// physical memory write accesses
extern void pmemwrite8  (uint64_t paddr, uint32_t data);
extern void pmemwrite16 (uint64_t paddr, uint32_t data);
extern void pmemwrite32 (uint64_t paddr, uint32_t data);
extern void pmemwrite64 (uint64_t paddr, uint64_t data);

#endif // _EMU_MEMOP_H
