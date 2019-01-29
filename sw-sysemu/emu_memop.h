#ifndef _EMU_MEMOP_H
#define _EMU_MEMOP_H

#include <cstdint>

// virtual to physical address translation
extern uint64_t (*vmemtranslate) (uint64_t addr, mem_access_type acc);

// virtual (if enabled) memory read accesses
extern uint8_t  vmemread8  (uint64_t addr);
extern uint16_t vmemread16 (uint64_t addr);
extern uint32_t vmemread32 (uint64_t addr);
extern uint64_t vmemread64 (uint64_t addr);

// virtual (if enabled) memory write accesses
extern void vmemwrite8  (uint64_t addr, uint8_t  data);
extern void vmemwrite16 (uint64_t addr, uint16_t data);
extern void vmemwrite32 (uint64_t addr, uint32_t data);
extern void vmemwrite64 (uint64_t addr, uint64_t data);

// physical memory instruction fetch accesses
extern uint16_t pmemfetch16(uint64_t paddr);

// physical memory read accesses
// extern uint64_t pmemread64(uint64_t addr);

// physical memory write accesses
extern void pmemwrite64 (uint64_t paddr, uint64_t data);

#endif // _EMU_MEMOP_H
