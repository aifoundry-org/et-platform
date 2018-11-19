#ifndef __RISCV_DISASM_H__
#define __RISCV_DIASAM_H__

#include <cstdint>
#include <cstddef>

// disassemble a single RISCV instruction into str
void riscv_disasm(char* str, size_t size, uint32_t bits);


#endif // __RISCV_DISASM_H__
