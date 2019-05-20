#ifndef EXCEPTION_H
#define EXCEPTION_H

#include <stdint.h>

void exception_handler(uint64_t mcause, uint64_t mepc, uint64_t mtval, uint64_t* const reg);

#endif
