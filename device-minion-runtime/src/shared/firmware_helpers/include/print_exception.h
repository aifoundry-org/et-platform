#ifndef PRINT_EXCEPTION_H
#define PRINT_EXCEPTION_H

#include <stdint.h>

void print_exception(uint64_t mcause, uint64_t mepc, uint64_t mtval, uint64_t mstatus,
                     uint64_t hart_id);

#endif
